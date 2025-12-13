#include <string.h>
#include "flash_emu.h"

static const char* FLASH_BINARY = "FunFS.bin";
static FILE*       aFile        = NULL;

static uint16_t flash_emu[FLASH_SIZE_TOTAL / 2];
static uint32_t fs_start_addr = 0x00;
static uint32_t fs_upper_addr = 0x00;
static uint16_t* first_page = NULL;

static uint16_t page_ram[PAGE_SIZE / 2];
static uint32_t available_memory = 0;

static void
mm_set_bounds(void)
{
	fs_start_addr = PAGE_CEIL((uint32_t)flash_emu);
	fs_upper_addr = (uint32_t)flash_emu + FLASH_SIZE_TOTAL;

	/* To make simulator as close to hardware as possible, this function
	   sets 'fs_start_addr' to address which is aligned up to the nearest page. This in turn has
	   broken mm_write() and mm_read() functions because the expression below
	   'offset = (offset - fs_start_addr) / 2;' (see in above mentioned functions)
	   calculates offset from the beginning of the 'flash_emu' array, not from the first
	   avaiable page-aligned address.
	   To fix that, we take an address within 'flash_emu' pointed by the first page
	   available for the user data. */
	first_page = &flash_emu[(fs_start_addr - (uint32_t)flash_emu) / 2];
	available_memory = sizeof(flash_emu) - ((uint32_t)first_page - (uint32_t)flash_emu);
	DBG_PRINT_VARG(
		"\n"
		"flash size:              %d\n"
		"page size:               %d\n"
		"pages total:             %d\n"
		"transaction buffer size: %d\n"
		"flash start address:     %08X\n"
		"program start address:   %08X\n"
		"flash upper bound:       %08X\n\n",
		sizeof(flash_emu),
		PAGE_SIZE,
		PAGES_TOTAL,
		sizeof(page_ram),
		(uint32_t)flash_emu,
		fs_start_addr,
		fs_upper_addr
	)
}

void
set_available_memory(uint32_t size)
{
	available_memory -= size;
}

uint32_t
get_available_memory(void)
{
	return available_memory;
}

uint32_t
mm_get_start_address(void)
{
	mm_set_bounds();
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
			// allocate this block
			current->len = size;

			current->prev = (uint32_t)previous + sizeof(block_t);
			address       = (uint32_t)current  + sizeof(block_t);
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
	if (offset > fs_upper_addr) {
		return mm_writeErr;
	}

	// address to index conversion
	offset = (offset - fs_start_addr) / 2;

	if (flash_emu[offset] != 0xFFFF) {
		printf(
			"\n\t\t\t****HardFault****\n"
			"Attempt to write at address '%08X' (emu_flash[%d])\n"
			"which isn't blank and contains '%08X' value\n\n",
			temp, offset, flash_emu[offset]
		);

		// DBG_PRINT_HEX((uint8_t*)flash_emu, sizeof(flash_emu))

		exit(1);
		return mm_writeErr;
	}

	first_page[offset] = half_word;

	return mm_Ok;
}

mm_Result
mm_read(uint32_t offset, uint16_t* half_word)
{
	if (offset > fs_upper_addr) {
		return mm_readErr;
	}

	// address to index conversion
	offset = (offset - fs_start_addr) / 2;

	*half_word = first_page[offset];

	return mm_Ok;
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

static mm_Result
update_image(void)
{
	mm_Result result = mm_Ok;

	do {
		if (aFile == NULL) {
			fprintf(stderr, "ERROR: can't update %s binary because file isn't openned yet.\n", FLASH_BINARY);
			result = mm_fopenErr;
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
	
	} while (0);
	// DBG_PRINT_HEX((uint8_t*)flash_emu, sizeof(flash_emu))
	return result;
}

mm_Result
mm_save_image(void)
{
	mm_Result result = mm_Ok;

	if (aFile != NULL) {
		result = update_image();
		fclose(aFile);
		aFile = NULL;
	}

	return result;
}

static mm_Result
clear_page(uint32_t address)
{
	if (address > fs_upper_addr) {
		return mm_writeErr;
	}

	// address to index conversion
	address = (address - fs_start_addr) / 2;

	for (uint16_t i = 0; i < PAGE_SIZE / 2; ++i) {
		first_page[address + i] = 0xFFFF;
	}

	return mm_Ok;
}

void mm_rewrite_page(const uint32_t page_addr, const uint32_t data_addr, const uint32_t len);

/**
 * Updates a data field in a given page.
 * Because we can only write into 'cleared' memory, and clearing encompases
 * the whole page, this function copies the given page into RAM buffer, makes
 * updates at the given offset and then writes updated page into the flash. 
 * Targeted page in flash is cleared beforehand, natch.
 */
mm_Result
mm_copy_page(const uint32_t page_addr, const uint32_t data_addr, uint8_t* data, const uint32_t len)
{
	mm_Result result         = mm_Ok;
	uint16_t half_word       = 0;

	memset((uint8_t*)page_ram, 0xFF, sizeof(page_ram));
	mm_rewrite_page(page_addr, data_addr, len);

	do {
		// 1. copy all data from flash to ram
		for (uint32_t src_idx = 0, dst_idx = 0; src_idx < PAGE_SIZE; src_idx += 2, ++dst_idx) {
			if ((result = mm_read(page_addr + src_idx, &half_word)) != mm_Ok) {
				break;
			}
			page_ram[dst_idx] = half_word;
		}
		
		if (result != mm_Ok) {
			break;
		}
		
		half_word = 0;

		// 2. update fields in RAM
		for (uint16_t src_idx = 0, dst_idx = (data_addr - page_addr) / 2; src_idx < len; src_idx += 2, ++dst_idx) {
			half_word |= (uint16_t)data[src_idx + 1] << 8;
			half_word |= (uint16_t)data[src_idx] & 0x00FF;

			page_ram[dst_idx] = half_word;
			half_word = 0;
		}

		// 3. clear page
		result = clear_page(page_addr);
		if (result != mm_Ok){
			break;
		}

		// 4. update entire page
		for (uint32_t dest_idx = 0, src_idx = 0; dest_idx < PAGE_SIZE; dest_idx += 2, ++src_idx) {
			if ((result = mm_write(page_addr + dest_idx, page_ram[src_idx])) != mm_Ok) {
				break;
			}
		}
	} while (0);

	return result;
}

void
mm_rewrite_page(const uint32_t page_addr, const uint32_t data_addr, const uint32_t len)
{
	uint32_t data_upper_bound = data_addr + len;
	uint32_t belongs_to_page = PAGE_ALIGN(data_upper_bound);
	if (page_addr != belongs_to_page) {
		printf(
			"WARNING: data spawns over pages.\n"
			"PAGE 1:   %08X\n"
			"PAGE 2:   %08X\n"
			"DATA LEN: %d\n",
			page_addr, belongs_to_page, len
		);
	}
}