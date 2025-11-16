#ifndef ISO_FLASHMNGR_H
#define ISO_FLASHMNGR_H

#include <stdint.h>

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