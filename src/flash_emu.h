#ifndef FUNFS_FLASH_EMU_H
#define FUNFS_FLASH_EMU_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE   1024
#define PAGES_TOTAL 4
#define FLASH_SIZE_TOTAL (PAGES_TOTAL * PAGE_SIZE)

#define WORD_ALIGNED(expr) (((expr) +  0x03) & 0xFFFFFFFC)  // word = 4 bytes
#define HEX_ALIGNED(expr)  (((expr) +  0x0F) & 0xFFFFFFF0)  // hex = 16 bytes
#define PAGE_ALIGNED(expr) (((expr) + 0x3FF) & 0xFFFFFC00)  // page = 1024 bytes

typedef enum {
	mm_Ok,
	mm_readErr,
	mm_writeErr,
	mm_fopenErr,
	mm_InodeTableFull,
	mm_Err
} mm_Result;

/** Flash Memory block */
typedef struct {
	uint32_t len;
	uint32_t prev;
} block_t;

uint32_t mm_get_start_address(void);

uint32_t mm_allocate(uint16_t size);
// mm_Result mm_deallocate(uint32_t offset);
// mm_Result mm_defragmentate(void);
mm_Result mm_write(uint32_t offset, uint16_t half_word);
mm_Result mm_read (uint32_t offset, uint16_t* half_word);

mm_Result mm_open_image(void);
mm_Result mm_save_image(void);

#endif /* FUNFS_FLASH_EMU_H */