#ifndef FUNFS_FLASH_EMU_H
#define FUNFS_FLASH_EMU_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE   1024
#define PAGES_TOTAL 64
#define FLASH_SIZE_TOTAL (PAGES_TOTAL * PAGE_SIZE)

#define WORD_ALIGNED(expr) (((expr) +  0x03) & 0xFFFFFFFC)  // word = 4 bytes
#define PAGE_ALIGNED(expr) (((expr) + 0x3FF) & 0xFFFFFC00)  // page = 1024 bytes

typedef enum {
	fmr_Ok,
	fmr_readErr,
	fmr_writeErr,
	fmr_fopenErr,
	fmr_InodeTableFull,
	fmr_Err
} FmResult;

/** Flash Memory block */
typedef struct {
	uint32_t len;
	uint32_t prev;
} FmBlock;

// extern uint8_t flash_emu[];
// extern const uint32_t fs_start_addr;

uint32_t femu_get_start_address(void);

uint32_t femu_allocate(uint16_t size);
// FmResult femu_deallocate(uint32_t offset);
// FmResult femu_defragmentate(void);
FmResult femu_write(uint32_t offset, uint8_t* data, uint16_t data_len);
FmResult femu_read(uint32_t offset, uint8_t* data, uint16_t data_len);

FmResult femu_open_flash(void);
FmResult femu_close_flash(void);

#endif /* FUNFS_FLASH_EMU_H */