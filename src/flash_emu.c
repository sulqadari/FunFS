#include <string.h>
#include "flash_emu.h"

static const char* FLASH_BINARY = "FunFS.bin";
static FILE*       aFile        = NULL;

static uint16_t flash_emu[FLASH_SIZE_TOTAL / 2];
static uint32_t fs_start_addr = 0x00;
static uint32_t fs_upper_addr = 0x00;
static uint16_t* first_page = NULL;

static void
mm_set_bounds(void)
{
	(void)sizeof(flash_emu);
	fs_start_addr = PAGE_ALIGNED((uint32_t)flash_emu);
	fs_upper_addr = (uint32_t)flash_emu + FLASH_SIZE_TOTAL;

	/* To make simulator as close to hardware as possible, this function
	   sets 'fs_start_addr' to address which is aligned to page size. This in turn has
	   broken mm_write() and mm_read() functions because the expression below
	   'offset = (offset - fs_start_addr) / 2;' (see in above mentioned functions)
	   calculates offset from the beginning of the 'flash_emu' array, not from the first
	   avaiable page-aligned address.
	   To fix that, we take an address within 'flash_emu' pointed by the first page
	   available for the user data. */
	first_page = &flash_emu[(fs_start_addr - (uint32_t)flash_emu) / 2];
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
	size = WORD_ALIGNED(size);
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
			current = (block_t*)HEX_ALIGNED((uint32_t)current);

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
	if (offset > fs_upper_addr) {
		return mm_writeErr;
	}

	// address to index conversion
	offset = (offset - fs_start_addr) / 2;

	// if (flash_emu[offset] != 0xFFFF) {
	// 	return mm_writeErr;
	// }

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