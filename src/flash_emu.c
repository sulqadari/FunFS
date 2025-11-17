#include <string.h>
#include "flash_emu.h"

static const char* FLASH_BINARY = "FunFS.bin";
static FILE*       aFile        = NULL;

uint8_t flash_emu[FLASH_SIZE_TOTAL];
uint32_t magic = 0xCAFEBABE;

/* forward declaration */
static uint8_t open_flash(void);
static uint8_t update_flash(void);

void
mmg_init(void)
{
	// if (memcmp(flash_emu, &magic, sizeof(magic))) {
	// 	memset(flash_emu, 0xFF, FLASH_SIZE_TOTAL);
	// }

	// Zero-out memory
	memset(flash_emu, 0xFF, FLASH_SIZE_TOTAL);
	// Open or create persistent storage for a flash memory
	open_flash();
	// Write the magic
	mmg_write(0, (uint8_t*)&magic, sizeof(magic));
}

FMResult
mmg_write(uint32_t offset, uint8_t* data, uint16_t data_len)
{
	uint32_t bound = offset + data_len;
	if (bound > FLASH_SIZE_TOTAL) {
		return fmr_Err;
	}

	memcpy(&flash_emu[offset], data, data_len);
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
	size_t bytesRead = 0;
	uint8_t result = 0;

	if (aFile != NULL) {
		return result;
	}

	aFile = fopen(FLASH_BINARY, "r+");
	if (aFile != NULL) {
		bytesRead = fread(flash_emu, 1, FLASH_SIZE_TOTAL, aFile);
	} else {
		aFile = fopen(FLASH_BINARY, "w");
	}

	if ((bytesRead != FLASH_SIZE_TOTAL) || (aFile == NULL)) {
		result = 1;
	}

	return result;
}

static uint8_t
update_flash(void)
{
	if (aFile == NULL) {
		fprintf(stderr, "ERROR: can't update %s binary because file isn't openned yet.\n", FLASH_BINARY);
		return 1;
	}

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
		update_flash();
		fclose(aFile);
	}
}