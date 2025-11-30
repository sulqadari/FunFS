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
} emu_Result;

/** Flash Memory block */
typedef struct {
	uint32_t len;
	uint32_t prev;
} block_t;

uint32_t emu_get_start_address(void);

uint32_t emu_allocate(uint16_t size);
// emu_Result emu_deallocate(uint32_t offset);
// emu_Result emu_defragmentate(void);
emu_Result emu_write(uint32_t offset, uint8_t* data, uint16_t data_len);
emu_Result emu_read(uint32_t offset, uint8_t* data, uint16_t data_len);

emu_Result emu_open_flash(void);
emu_Result emu_close_flash(void);

#endif /* FUNFS_FLASH_EMU_H */