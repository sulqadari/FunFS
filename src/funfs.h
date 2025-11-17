#ifndef FUNFS_FILESYSTEM_H
#define FUNFS_FILESYSTEM_H

#include <stdint.h>
#include "flash_emu.h"

extern uint8_t flash_emu[];

typedef enum {
	ft_DF, // Dedicated file
	ft_EF, // Elementary file
	ft_LF, // Linear Fixed file
	ft_LV  // Linear Variable file
} FileType;

/** Keeps information about each file on the file system */
typedef struct {
	FileType type;
	uint16_t id;
	uint16_t size;
	uint32_t capacity;
} Inode;

typedef struct {
	
} InodeBmp;

/** User data */
typedef struct {
	uint8_t page[PAGE_SIZE];
} DataBlk;

typedef struct {
	
} DataBlkBmp;

typedef struct {
	uint32_t iNodesCount;
	uint32_t iNodeStartAddress;
	uint32_t dataBlockCount;
	uint32_t dataRegionStartAddress;
} SuperBlock;

typedef struct {
	SuperBlock super;
	InodeBmp   inBmp;
	DataBlkBmp dbBmp;
	Inode      iNodeTable;
	DataBlk    dataRegion;
} FileSystem;

#endif /* FUNFS_FILESYSTEM_H */