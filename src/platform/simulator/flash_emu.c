#include <string.h>
#include <signal.h>

#include "flash_emu.h"

static const char* FLASH_BINARY = "FunFS.bin";
static FILE*       aFile        = NULL;

static uint16_t page_ram [PAGE_SIZE / 2];
static uint16_t flash_emu[FLASH_SIZE_TOTAL / 2];

/**Page-aligned first address.
 * This solution resembles a real hardware: usually we use a first available page after
 * the main sections (.text, .data, .bss, etc.). */
static uint32_t fs_start_addr    = 0x00;

/** Upper bound of the NVM. */
static uint32_t fs_upper_addr    = 0x00;

/** Available NVM memory after each allocation */
static uint32_t available_memory = 0x00;

uint16_t*
mm_get_flash_emu(void)
{
	return flash_emu;
}

void
mm_set_available_memory(uint32_t size)
{
	available_memory -= HEX_CEIL(size + 1) + sizeof(block_t);
}

uint32_t
mm_get_available_memory(void)
{
	return available_memory;
}

uint32_t
mm_get_start_address(void)
{
	fs_start_addr = PAGE_CEIL((uint32_t)flash_emu);
	fs_upper_addr = (uint32_t)flash_emu + FLASH_SIZE_TOTAL;

	// initially available memory
	available_memory = FLASH_SIZE_TOTAL - (fs_start_addr - (uint32_t)flash_emu);

	DBG_PRINT_VARG(
		"\n"
		"flash size:               %d bytes\n"
		"page  size:               %d bytes\n"
		"pages total:              %d bytes\n"
		"transaction buffer size:  %d bytes\n"

		"flash   start address:    0x%08x\n"
		"program start address:    0x%08x\n"
		"flash   upper bound:      0x%08x\n\n",
		FLASH_SIZE_TOTAL,
		PAGE_SIZE,
		PAGES_TOTAL,
		sizeof(page_ram),
		
		(uint32_t)flash_emu,
		fs_start_addr,
		fs_upper_addr
	)
	
	return fs_start_addr + sizeof(block_t);
}

uint32_t
mm_allocate(uint16_t size)
{
	size = WORD_CEIL(size);
	block_t* current  = (block_t*)fs_start_addr;
	block_t* previous = (block_t*)fs_start_addr;
	uint32_t address  = 0;
	do {
		if (current->len == 0xFFFFFFFF) { // the block is empty
			
			uint32_t offset = (uint32_t)&current->len;
			uint16_t value = size;
			
			// allocate this block
			mm_write(offset, value);
			mm_write(offset + 2, 0);

			offset = (uint32_t)&current->prev;
			value  = (uint16_t)(((uint32_t)previous + sizeof(block_t)) & 0x0000FFFF);
			mm_write(offset, value);

			offset += 2;
			value   = (uint16_t)(((uint32_t)previous + sizeof(block_t)) >> 16);
			mm_write(offset, value);
			
			address = (uint32_t)current  + sizeof(block_t);

			break;
		} else { // look for an another block
			previous = current;
			// shift to the next block
			current = (block_t*)((uint8_t*)current + current->len + sizeof(block_t));
			current = (block_t*)HEX_CEIL((uint32_t)current);

			// check if the current address doesn't exceed flash boundary
			if ((uint32_t)current + size >= fs_upper_addr) {
				break;
			}
		}
	} while (1);

	return address;
}

mm_Result
mm_write(uint32_t offset, uint16_t half_word)
{
	uint32_t temp = offset;
	uint16_t* ptr = (uint16_t*)fs_start_addr;

	if (offset & 0x00000001) {
		printf("\n\t\t\t****HardFault****\n"
			"Attempt to write at address '%08X' which isn't half-word aligned\n\n", offset
		);

		raise(SIGINT);
	}

	if (offset > fs_upper_addr) {
		return mm_writeErr;
	}

	// address to index conversion
	offset = (offset - fs_start_addr) / 2;

	if (ptr[offset] != 0xFFFF) {
		printf("\n\t\t\t****HardFault****\n"
			"Attempt to write at address '%08X' (first_page[%d])\nwhich isn't blank and contains '%04X' value\n\n", temp, offset, ptr[offset]
		);

		raise(SIGINT);
	}

	ptr[offset] = half_word;

	return mm_Ok;
}

mm_Result
mm_read(uint32_t offset, uint8_t* byte)
{
	uint8_t* ptr = (uint8_t*)fs_start_addr;
	*byte = 0;

	if (offset > fs_upper_addr) {
		return mm_readErr;
	}

	// address to index conversion
	offset = (offset - fs_start_addr);

	*byte = ptr[offset];

	return mm_Ok;
}

static mm_Result
clear_page(uint32_t address)
{
	uint16_t* ptr = (uint16_t*)fs_start_addr;

	if (address > fs_upper_addr) {
		return mm_writeErr;
	}

	// address to index conversion
	address = (address - fs_start_addr) / 2;

	for (uint16_t i = 0; i < PAGE_SIZE / 2; ++i) {
		ptr[address + i] = 0xFFFF;
	}

	return mm_Ok;
}

/**
 * Updates a data field in a given page.
 * Because we can only write into 'cleared' memory, and clearing encompases
 * the whole page, this function copies the given page into RAM buffer, makes
 * updates at the given offset and then writes updated page into the flash. 
 * Targeted page in flash is cleared beforehand, natch.
 */
static mm_Result
rewrite_next_page(const uint32_t page_addr, const uint32_t data_addr, uint8_t* data, uint32_t len)
{
	mm_Result result  = mm_Ok;
	uint8_t   byte    = 0;
	uint8_t*  page_u8 = (uint8_t*)page_ram;

	memset((uint8_t*)page_ram, 0xFF, PAGE_SIZE);

	do {
		// 1. copy all data from flash to ram
		for (uint32_t i = 0; i < PAGE_SIZE; ++i) {
			if ((result = mm_read(page_addr + i, &byte)) != mm_Ok) {
				break;
			}

			page_u8[i] = byte;
		}
		
		if (result != mm_Ok) {
			break;
		}
		
		// 2. update fields in RAM
		for (uint32_t src = 0, dst = (data_addr - page_addr); src < len; src++, dst++) {
			page_u8[dst] = data[src];
		}

		// 3. clear page
		if ((result = clear_page(page_addr)) != mm_Ok) {
			break;
		}

		// 4. update entire page
		for (uint32_t src = 0, dst = 0; dst < PAGE_SIZE; ++src, dst += 2) {
			if ((result = mm_write(page_addr + dst, page_ram[src])) != mm_Ok) {
				break;
			}
		}
	} while (0);

	return result;
}

/**
 * Rewrites an entire page.
 * This function is called whenever the 'hlp_write_data()' function encounters a non-FF value.
 * It also handles cases when data spans two or more pages.
 */
mm_Result
mm_rewrite_page(uint32_t start_page, uint32_t data_start_addr, uint8_t* data, uint32_t len)
{
	mm_Result result  = mm_Ok;
	uint8_t* data_ptr = data;
	uint32_t portion  = 0;

	do {
		portion = PAGE_CEIL(start_page + 1) - data_start_addr;
		if (len < portion) { // remaining bytes are less than page size
			portion = len;
		}

		rewrite_next_page(start_page, data_start_addr, data_ptr, portion);
		start_page       = PAGE_CEIL(start_page + 1);
		data_start_addr += portion;
		data_ptr        += portion;
		
	} while (len -= portion);

	return result;
}


/**
 * Creates an 64 Kbyte flash memory simulation space.
 * If file already exists, it will be reused.
 */
mm_Result
mm_open_image(void)
{
	// size_t bytesRead = 0;
	mm_Result result = mm_Ok;

	do {
		if (aFile != NULL) { // the 'FunFS.bin' is already openned. Bail out.
			break;
		}

		memset((uint8_t*)flash_emu, 0xFF, FLASH_SIZE_TOTAL);
  		aFile = fopen(FLASH_BINARY, "w");

		if (aFile == NULL) {
			result = mm_fopenErr;
			break;
		}

	} while (0);

	return result;
}

mm_Result
mm_save_image(void)
{
	mm_Result result = mm_Ok;

	do {
		if (aFile == NULL) {
			break;
		}

		rewind(aFile);
		
		size_t written = fwrite(flash_emu, 2, FLASH_SIZE_TOTAL / 2, aFile);
		if (written < FLASH_SIZE_TOTAL / 2) {
			fprintf(stderr, "ERROR: couldn't update %s binary file.\n", FLASH_BINARY);
			fclose(aFile);
			result = mm_writeErr;
			break;
		}
		
		fclose(aFile);
		aFile = NULL;
	} while (0);

	return result;
}
