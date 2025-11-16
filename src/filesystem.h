#ifndef ISO_FILESYSTEM_H
#define ISO_FILESYSTEM_H

#include <stdint.h>

#define PAGE_SIZE (256)
#define PAGES_TOTAL (64)
#define ALLOCATED_MEMORY  (PAGES_TOTAL * PAGE_SIZE) // 16 KBytes

/** 10% of all available flash memory goes to iNodes Table */
#define INODES_TABLE_SIZE (((ALLOCATED_MEMORY / PAGE_SIZE) * 10) / 100)

/** (allocated memory - iNodes Table) - SuperBlock = a space available for user data. */
#define DATA_REGION_TOTAL ((ALLOCATED_MEMORY / PAGE_SIZE) - INODES_TABLE_SIZE)

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
	Inode      iNodeTable[INODES_TABLE_SIZE]; // 6
	DataBlk    dataRegion[DATA_REGION_TOTAL];	// 57
} FileSystem;

#endif /* ISO_FILESYSTEM_H */