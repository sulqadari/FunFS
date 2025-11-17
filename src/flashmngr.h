#ifndef ISO_FLASHMNGR_H
#define ISO_FLASHMNGR_H

#include <stdint.h>

#define PAGE_SIZE (1024)
#define PAGES_TOTAL (64)
#define FLASH_SIZE_TOTAL (PAGES_TOTAL * PAGE_SIZE) // 64 KBytes
#define DATABLOCKS_TOTAL (flashAvailable / PAGE_SIZE)

uint32_t _flashStartAddr  = 0x08003C24; // this value is taken as an example
uint32_t flashBaseAddr    = ((_flashStartAddr + 0x3FF) & 0xFFFFFC00);
uint32_t SupBlkStartAddr  = flashBaseAddr;
uint32_t iNodeStartAddr   = SupBlkStartAddr + PAGE_SIZE;
uint32_t dataBlkStartAddr = iNodeStartAddr + (PAGE_SIZE * 3); // 0x8004C00
uint32_t flashUpperBound  = 0x08000000 + FLASH_SIZE_TOTAL;
uint32_t flashAvailable   = flashUpperBound - dataBlkStartAddr; // 0xB400 (46 Kbytes)

typedef enum {
	fmr_Ok,
	fmr_Err
} FMResult;

FMResult mmg_init(uint32_t offset);
FMResult mmg_allocate(uint32_t offset, uint16_t size);
FMResult mmg_deallocate(uint32_t offset);
FMResult mmg_defragmentate(void);
FMResult mmg_write(uint32_t offset, uint8_t* data, uint16_t data_len);
FMResult mmg_read(uint32_t offset, uint8_t* data, uint16_t data_len);

#endif /* ISO_FLASHMNGR_H */