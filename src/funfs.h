#ifndef FUNFS_FILESYSTEM_H
#define FUNFS_FILESYSTEM_H

#include <stdint.h>
#include "flash_emu.h"

// allocate PERSENT percents for Inodes
#define PERSENT 10
#define INODES_TOTAL (((FLASH_SIZE_TOTAL * PERSENT) / 100) / sizeof(Inode))

#define FID_NONE          0xFFFF
#define FID_MASTER_FILE   0x3F00
#define FID_PIN_FILE      0x4001
#define FID_SYM_KEYS_FILE 0x4002
#define FID_SEC_ENV_FILE  0x4003

typedef enum {
	ft_EF = 0x01, // Elementary file
	ft_LF = 0x0C, // Linear Fixed file
	ft_DF = 0x38, // Dedicated file
} FileType;

/** Keeps information about each file on the file system */
typedef struct {
	uint16_t size;        // 0x80; File size       (EF only)
	uint8_t  desc[5];     // 0x82; file descriptor (e.g. DF, EF, LF, etc.)
	uint16_t fid;         // 0x83; file ID         (e.g. 3F00, 0101, etc.)
	uint8_t  aid[16];     // 0x84; application AID (DF only)
	uint8_t  sfi;         // 0x88; short file ID   (EF only)
	uint8_t  lcs;         // 0x8A; Life cycle stage
	uint8_t compact[7];   // 0x8C; security attributes in compact format  
	uint16_t se;          // 0x8D; the FID of associated securiy environment (DF only)
	uint8_t expanded[20]; // 0xAB; security attribute in expanded format 
	uint32_t data;        // points to the associated data block
} Inode;

typedef struct {
	uint32_t magic;
	uint32_t inodes_count;
	uint32_t inodes_capacity;
	uint32_t inodes_start;
} SuperBlock;

/** Used as entry in dedicated file's data block.
 * When we create a DF, the system will allocate 1 Kbyte flash storage for it.
 * In that storage will be kept an information about all direct children of that folder.
 * This means that the number of direct descendants of a DF is limited to 256 entries. */
typedef struct {
	uint16_t iNode;
	uint16_t fid;
} DfEntry;

typedef struct {
	uint32_t count;
	DfEntry entries[256 / sizeof(DfEntry) - 1];
} DfPayload;

FmResult ffs_initialize(void);

FmResult ffs_create_file(uint8_t* data, uint32_t data_len);
FmResult ffs_select_file(uint8_t* data, uint32_t data_len);

#endif /* FUNFS_FILESYSTEM_H */