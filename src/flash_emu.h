#ifndef FUNFS_FLASH_EMU_H
#define FUNFS_FLASH_EMU_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE 1024
#define PAGES_TOTAL 64
#define FLASH_SIZE_TOTAL (PAGES_TOTAL * PAGE_SIZE)

typedef enum {
	fmr_Ok,
	fmr_readErr,
	fmr_writeErr,
	fmr_fopenErr,
	fmr_Err
} FMResult;

/** User data */
typedef struct {
	uint32_t len;
	uint32_t next;
	uint32_t prev;
} DataBlk;

extern uint8_t flash_emu[];
extern uint32_t fs_start_addr;

uint32_t mmg_allocate(uint32_t offset, uint16_t size);
// FMResult mmg_deallocate(uint32_t offset);
// FMResult mmg_defragmentate(void);
FMResult mmg_write(uint32_t offset, uint8_t* data, uint16_t data_len);
FMResult mmg_read(uint32_t offset, uint8_t* data, uint16_t data_len);

FMResult mmg_open_flash(void);
FMResult mmg_close_flash(void);

#endif /* FUNFS_FLASH_EMU_H */