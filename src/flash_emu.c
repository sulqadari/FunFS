#include <string.h>
#include "flash_emu.h"

static const char* FLASH_BINARY = "FunFS.bin";
static FILE*       aFile        = NULL;

uint32_t fs_start_addr = 0x00;
uint8_t flash_emu[FLASH_SIZE_TOTAL];

uint32_t
mmg_allocate(uint32_t offset, uint16_t size)
{
	uint32_t address = 0;

	do {

	} while (0);

	return address;
}

FMResult
mmg_write(uint32_t offset, uint8_t* data, uint16_t data_len)
{
	uint32_t bound = offset + data_len;
	if (bound > FLASH_SIZE_TOTAL) {
		return fmr_writeErr;
	}

	memcpy(&flash_emu[offset], data, data_len);
	return fmr_Ok;
}

FMResult
mmg_read(uint32_t offset, uint8_t* data, uint16_t data_len)
{
	uint32_t bound = offset + data_len;
	if (bound > FLASH_SIZE_TOTAL) {
		return fmr_readErr;
	}

	memcpy(data, &flash_emu[offset], data_len);
	return fmr_Ok;
}

/**
 * Creates an 64 Kbyte flash memory simulation space.
 * If file already exists, it will be reused.
 */
FMResult
mmg_open_flash(void)
{
	size_t bytesRead = 0;
	FMResult result = fmr_Ok;

	do {
		if (aFile != NULL) {
			break;
		}
		memset(flash_emu, 0xFF, FLASH_SIZE_TOTAL);

		result = fmr_Err;
		aFile = fopen(FLASH_BINARY, "r+");
		if (aFile != NULL) {
			bytesRead = fread(flash_emu, 1, FLASH_SIZE_TOTAL, aFile);
		} else {
			aFile = fopen(FLASH_BINARY, "w");
		}

		if (bytesRead != FLASH_SIZE_TOTAL) {
			result = fmr_readErr;
			break;
		}
		if (aFile == NULL) {
			result = fmr_fopenErr;
			break;
		}

		result = fmr_Ok;
	} while (0);

	return result;
}

static FMResult
update_flash(void)
{
	FMResult result = fmr_Ok;

	do {
		if (aFile == NULL) {
			fprintf(stderr, "ERROR: can't update %s binary because file isn't openned yet.\n", FLASH_BINARY);
			result = fmr_Err;
			break;
		}
	
		size_t written = fwrite(flash_emu, 1, FLASH_SIZE_TOTAL, aFile);
		if (written < FLASH_SIZE_TOTAL) {
			fprintf(stderr, "ERROR: couldn't update %s binary file.\n", FLASH_BINARY);
			fclose(aFile);
			result = fmr_writeErr;
			break;
		}
	
		rewind(aFile);
	} while (0);

	return result;
}

FMResult
mmg_close_flash(void)
{
	FMResult result = fmr_Ok;

	if (aFile != NULL) {
		result = update_flash();
		fclose(aFile);
	}

	return result;
}