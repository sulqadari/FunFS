#include <string.h>
#include "flash_emu.h"

static const char* FLASH_BINARY = "FunFS.bin";
static FILE*       aFile        = NULL;

static uint8_t flash_emu[FLASH_SIZE_TOTAL];
static uint32_t fs_start_addr = 0x00;
static uint32_t fs_upper_addr = 0x00;

static void
mm_set_bounds(void)
{
	(void)sizeof(flash_emu);
	fs_start_addr = (uint32_t)&flash_emu;
	fs_upper_addr = fs_start_addr + FLASH_SIZE_TOTAL;
}

uint32_t
mm_get_start_address(void)
{
	mm_set_bounds();
	return fs_start_addr;
}

#if defined (FEMU_USE_INDICES)
#define calculate_index(addr) (uint32_t)((uint32_t)addr + sizeof(block_t)) - (uint32_t)flash_emu
#else
#define calculate_index(addr) (uint32_t)((uint32_t)addr + sizeof(block_t))
#endif

uint32_t
mm_allocate(uint16_t size)
{
	size = WORD_ALIGNED(size);
	block_t* current = (block_t*)flash_emu;
	block_t* previous = (block_t*)flash_emu;
	uint32_t address = 0;
	do {
		if (current->len == 0xFFFFFFFF) { // the block is empty
			// allocate this block
			current->len = size;

			// set address->prev
			current->prev = calculate_index(previous);
			// compute actual index within flash_emu array
			address = calculate_index(current);
			break;
		} else { // seek another block
			previous = current;

			// shift to the next block
			current = (block_t*)((uint8_t*)current + current->len + sizeof(block_t));
			
			// check if the current address doesn't exceed flash boundary
			if ((uint32_t)current + size >= fs_upper_addr) {
				break;
			}
		}
	} while (1);

	return address;
}

mm_Result
mm_write(uint32_t offset, uint8_t* data, uint16_t data_len)
{
	if (offset + data_len > fs_upper_addr) {
		return mm_writeErr;
	}

	// address to index convertation
	offset = offset - fs_start_addr;
	memcpy(&flash_emu[offset], data, data_len);
	return mm_Ok;
}

mm_Result
mm_read(uint32_t offset, uint8_t* data, uint16_t data_len)
{
	if (offset + data_len > fs_upper_addr) {
		return mm_readErr;
	}

	// address to index convertation
	offset = offset - fs_start_addr;
	memcpy(data, &flash_emu[offset], data_len);
	return mm_Ok;
}

/**
 * Creates an 64 Kbyte flash memory simulation space.
 * If file already exists, it will be reused.
 */
mm_Result
mm_open_flash(void)
{
	// size_t bytesRead = 0;
	mm_Result result = mm_Ok;

	do {
		if (aFile != NULL) { // the 'FunFS.bin' is already openned. Bail out.
			break;
		}

		memset(flash_emu, 0xFF, FLASH_SIZE_TOTAL);
  		aFile = fopen(FLASH_BINARY, "w");

		if (aFile == NULL) {
			result = mm_fopenErr;
			break;
		}

	} while (0);

	return result;
}

static mm_Result
update_flash(void)
{
	mm_Result result = mm_Ok;

	do {
		if (aFile == NULL) {
			fprintf(stderr, "ERROR: can't update %s binary because file isn't openned yet.\n", FLASH_BINARY);
			result = mm_fopenErr;
			break;
		}
		
		rewind(aFile);
		
		size_t written = fwrite(flash_emu, 1, FLASH_SIZE_TOTAL, aFile);
		if (written < FLASH_SIZE_TOTAL) {
			fprintf(stderr, "ERROR: couldn't update %s binary file.\n", FLASH_BINARY);
			fclose(aFile);
			result = mm_writeErr;
			break;
		}
	
	} while (0);

	return result;
}

mm_Result
mm_close_flash(void)
{
	mm_Result result = mm_Ok;

	if (aFile != NULL) {
		result = update_flash();
		fclose(aFile);
		aFile = NULL;
	}

	return result;
}