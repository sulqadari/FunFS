#include <string.h>
#include "flash_emu.h"

static const char* FLASH_BINARY = "FunFS.bin";
static FILE*       aFile        = NULL;

uint8_t flash_emu[FLASH_SIZE_TOTAL];

/* forward declaration */
static uint8_t open_flash(void);
static uint8_t update_flash(void);

void
mmg_init(void)
{
	uint8_t heckVal[128];
	memset(heckVal, 0xFF, sizeof(heckVal));

	if (memcmp(flash_emu, heckVal, sizeof(heckVal))) {
		memset(flash_emu, 0xFF, FLASH_SIZE_TOTAL);
	}

	open_flash();
}

FMResult
mmg_write(uint32_t offset, uint8_t* data, uint16_t data_len)
{
	uint32_t bound = offset + data_len;
	if (bound > FLASH_SIZE_TOTAL) {
		return fmr_Err;
	}

	memcpy(&flash_emu[offset], data, data_len);

	update_flash();
	return fmr_Ok;
}

FMResult
mmg_read(uint32_t offset, uint8_t* data, uint16_t data_len)
{
	uint32_t bound = offset + data_len;
	if (bound > FLASH_SIZE_TOTAL) {
		return fmr_Err;
	}

	memcpy(data, &flash_emu[offset], data_len);
	return fmr_Ok;
}

/**
 * Creates an 64 Kbyte flash memory simulation space.
 * If file already exists, it will be reused.
 */
static uint8_t
open_flash(void)
{
	aFile = fopen(FLASH_BINARY, "r+");
	if (aFile != NULL) {
		fread(flash_emu, 1, FLASH_SIZE_TOTAL, aFile);
	} else {
		aFile = fopen(FLASH_BINARY, "w");
		if (aFile == NULL) {
			return 1;
		}
	}

	return 0;
}

static uint8_t
update_flash(void)
{
	size_t written = fwrite(flash_emu, 1, FLASH_SIZE_TOTAL, aFile);
	if (written < FLASH_SIZE_TOTAL) {
		fprintf(stderr, "ERROR: couldn't update %s binary file.\n", FLASH_BINARY);
		fclose(aFile);
		return 1;
	}

	rewind(aFile);
	return 0;
}

void
close_flash(void)
{
	if (aFile != NULL) {
		fclose(aFile);
	}
}