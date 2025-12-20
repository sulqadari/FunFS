#ifndef FUNFS_FLASH_EMU_H
#define FUNFS_FLASH_EMU_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"

#ifndef PAGE_SIZE
#	define PAGE_SIZE   1024
#endif
#ifndef PAGES_TOTAL
#	define PAGES_TOTAL 64
#endif

#ifndef ITABLE_MEMORY_OCCUPATION_IN_PERCENTS
#	define ITABLE_MEMORY_OCCUPATION_IN_PERCENTS 10
#endif

#define FLASH_SIZE_TOTAL (PAGES_TOTAL * PAGE_SIZE)
#define INODE_TABLE_SIZE ((FLASH_SIZE_TOTAL * ITABLE_MEMORY_OCCUPATION_IN_PERCENTS) / 100)

#define WORD_CEIL(expr) (((expr) +  0x03) & 0xFFFFFFFC)  // word = 4 bytes
#define HEX_CEIL(expr)  (((expr) +  0x0F) & 0xFFFFFFF0)  // hex = 16 bytes
#define PAGE_CEIL(expr) (((expr) + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1))  // page = 1024 bytes

#define PAGE_ALIGN(expr) ((expr) & ~(PAGE_SIZE - 1))

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

uint32_t  mm_get_start_address(void);
uint32_t  mm_allocate(uint16_t size);
// mm_Result mm_deallocate(uint32_t offset);
// mm_Result mm_defragmentate(void);
mm_Result mm_write(uint32_t offset, uint16_t half_word);
mm_Result mm_read (uint32_t offset, uint8_t* byte);

mm_Result mm_open_image(void);
mm_Result mm_save_image(void);

mm_Result mm_rewrite_page(uint32_t start_page, uint32_t data_start_addr, uint8_t* data, uint32_t len);

void      set_available_memory(uint32_t size);
uint32_t  get_available_memory(void);

#endif /* FUNFS_FLASH_EMU_H */