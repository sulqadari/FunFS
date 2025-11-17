#ifndef FUNFS_FILESYSTEM_H
#define FUNFS_FILESYSTEM_H

#include <stdint.h>
#include "flash_emu.h"

typedef enum {
	ft_DF, // Dedicated file
	ft_EF, // Elementary file
	ft_LF, // Linear Fixed file
	ft_LV  // Linear Variable file
} FileType;

typedef enum {
	cmd_Ok,
	cmd_Error
} FsCmdStatus;

/** Keeps information about each file on the file system */
typedef struct {
	FileType type;
	uint16_t id;
	uint16_t size;			// how many bytes are occupied
	uint32_t capacity;		// the total size of this file
	// uint32_t blkCount;		// how many blocks have been allocated to this file?
	uint32_t* blkPointer;	// points to the associated data block
} Inode;

typedef struct {
	
} InodeBmp;

/** User data */
typedef struct {
	uint32_t length;
	uint32_t start;
} DataBlk;

typedef struct {
	
} DataBlkBmp;

typedef struct {
	uint32_t magic;
	uint32_t iNodesCount;
	uint32_t iNodesStartAddress;
	uint32_t dataBlkCount;
	uint32_t dataBlksStartAddress;
} SuperBlock;

/**
 * The length of iNodeTable is calculated as follow:
 * 1. four pages are dedicated for this array: "PAGE_SIZE * 4"
 * 2. the size of Inode block is sizeof(Inode) bytes length.
 * 3. each page can accomodate up to "PAGE_SIZE / sizeof(Inode)" entries
 * 4. thus "(PAGE_SIZE * 4) / sizeof(Inode)" is the number of INodes we can have in four pages
 */
typedef struct {
	SuperBlock super;
	InodeBmp   inBmp;
	DataBlkBmp dbBmp;
	Inode      iNodeTable;
	DataBlk    dataBlks;
} FileSystem;

/** Used as entry in dedicated file's data block.
 * When we create a DF, the system will allocate 1 Kbyte flash storage for it.
 * In that storage will be kept an information about all direct children of that folder.
 * This means that the number of direct descendants of a DF is limited to 256 entries.
 */
typedef struct {
	uint16_t iNode;
	uint16_t fid;
} IdTuple;

FsCmdStatus create_file(uint8_t* data, uint32_t len);
FsCmdStatus select_file(uint32_t fid);

#endif /* FUNFS_FILESYSTEM_H */