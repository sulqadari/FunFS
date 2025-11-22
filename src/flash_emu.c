#include <string.h>
#include "flash_emu.h"

static const char* FLASH_BINARY = "FunFS.bin";
static FILE*       aFile        = NULL;

static uint8_t flash_emu[FLASH_SIZE_TOTAL];
static uint32_t fs_start_addr = 0x00;
static uint32_t fs_upper_addr = 0x00;

static void
femu_set_bounds(void)
{
	fs_start_addr = 0;
	fs_upper_addr = FLASH_SIZE_TOTAL;
}

uint32_t
femu_get_start_address(void)
{
	femu_set_bounds();
	return fs_start_addr;
}

#define calculate_index(addr) (uint32_t)((uint32_t)addr + sizeof(FmBlock)) - (uint32_t)flash_emu

uint32_t
femu_allocate(uint16_t size)
{
	size = WORD_ALIGNED(size);
	FmBlock* current = (FmBlock*)flash_emu;
	FmBlock* previous = (FmBlock*)flash_emu;
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
			current = (FmBlock*)((uint8_t*)current + current->len + sizeof(FmBlock));
			
			// check if the current address doesn't exceed flash boundary
			if ((uint32_t)current + size >= (uint32_t)flash_emu + fs_upper_addr) {
				break;
			}
		}
	} while (1);

	return address;
}

FmResult
femu_write(uint32_t offset, uint8_t* data, uint16_t data_len)
{
	if (offset + data_len > fs_upper_addr) {
		return fmr_writeErr;
	}

	memcpy(&flash_emu[offset], data, data_len);
	return fmr_Ok;
}

FmResult
femu_read(uint32_t offset, uint8_t* data, uint16_t data_len)
{
	if (offset + data_len > fs_upper_addr) {
		return fmr_readErr;
	}

	memcpy(data, &flash_emu[offset], data_len);
	return fmr_Ok;
}

/**
 * Creates an 64 Kbyte flash memory simulation space.
 * If file already exists, it will be reused.
 */
FmResult
femu_open_flash(void)
{
	size_t bytesRead = 0;
	FmResult result = fmr_Ok;

	do {
		if (aFile != NULL) { // the 'FunFS.bin' is already openned. Bail out.
			break;
		}

		memset(flash_emu, 0xFF, FLASH_SIZE_TOTAL);
		aFile = fopen(FLASH_BINARY, "r+"); // 'r+' means 'open existing file'. I.e. file must already present.
		if (aFile != NULL) {
			bytesRead = fread(flash_emu, 1, FLASH_SIZE_TOTAL, aFile);
			if (bytesRead != FLASH_SIZE_TOTAL) { // we expect that the fread() will fetch 'FLASH_SIZE_TOTAL' bytes of data.
				result = fmr_readErr;
				break;
			}
		} else {
			aFile = fopen(FLASH_BINARY, "w"); // If file doesn't exist, then 'w' argument will create it.
		}

		if (aFile == NULL) {
			result = fmr_fopenErr;
			break;
		}

	} while (0);

	return result;
}

static FmResult
update_flash(void)
{
	FmResult result = fmr_Ok;

	do {
		if (aFile == NULL) {
			fprintf(stderr, "ERROR: can't update %s binary because file isn't openned yet.\n", FLASH_BINARY);
			result = fmr_fopenErr;
			break;
		}
		
		rewind(aFile);
		
		size_t written = fwrite(flash_emu, 1, FLASH_SIZE_TOTAL, aFile);
		if (written < FLASH_SIZE_TOTAL) {
			fprintf(stderr, "ERROR: couldn't update %s binary file.\n", FLASH_BINARY);
			fclose(aFile);
			result = fmr_writeErr;
			break;
		}
	
	} while (0);

	return result;
}

FmResult
femu_close_flash(void)
{
	FmResult result = fmr_Ok;

	if (aFile != NULL) {
		result = update_flash();
		fclose(aFile);
	}

	return result;
}