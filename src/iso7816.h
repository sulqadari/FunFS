#ifndef FUNFS_ISO7816_H
#define FUNFS_ISO7816_H

#include <stdint.h>

typedef enum {
	SW_SELECTED_FILE_DEACTIVATED         = 0x6283,
	SW_MEMORY_FAILURE                    = 0x6581,
	SW_MEMORY_FULL                       = 0x6582,
	SW_CMD_INCOMPATIBLE_WITH_FILE_STRUCT = 0x6981,
	SW_SECURITY_STATUS_NOT_SATISFIED     = 0x6982,
	SW_DATA_NOT_USABLE                   = 0x6984,
	SW_INCORRECT_PARAMS_IN_CDATA         = 0x6A80,
	SW_FUNCTION_NOT_SUPPORTED            = 0x6A81,
	SW_FILE_NOT_FOUND                    = 0x6A82,
	SW_NOT_ENOUGH_MEMORY_IN_FILE         = 0x6A84,
	SW_INCORRECT_P1P2                    = 0x6A86,
	SW_NC_INCONSISTENT_WITH_P1P2         = 0x6A87, // Nc - number of bytes
	SW_DATA_NOT_FOUND                    = 0x6A88,
	SW_FILE_ALREADY_EXISTS               = 0x6A89,
	SW_DF_NAME_ALREADY_EXISTS            = 0x6A8A,
	SW_INS_NOT_SUPPORTED                 = 0x6D00,
	SW_CLA_NOT_SUPPORTED                 = 0x6E00,
	SW_UNKNOWN                           = 0x6F00,
	SW_OK                                = 0x9000,
} ISO_SW;

#define FID_RESERVED_1    0xFFFF
#define FID_RESERVED_2    0x0000
#define FID_RESERVED_3    0x3FFF
#define FID_MASTER_FILE   0x3F00
#define FID_PIN_FILE      0x4001
#define FID_SYM_KEYS_FILE 0x4002
#define FID_SEC_ENV_FILE  0x4003

typedef enum {
	ft_EF = 0x01, // Elementary file
	ft_LF = 0x0C, // Linear Fixed file
	ft_DF = 0x38, // Dedicated file
} FileType;

#define LCS_INITIALIZED 0x03
#define LCS_ACTIVATED   0x05
#define LCS_DEACTIVATED 0x04
#define LCS_TERMINATED  0x0F
/** Keeps information about each file on the file system */
typedef struct {
	uint16_t size;        // 0x80; File size       (EF only)
	uint8_t  desc[6];     // 0x82; file descriptor (e.g. DF, EF, LF, etc.)
	uint16_t fid;         // 0x83; file ID         (e.g. 3F00, 0101, etc.)
	uint8_t  aid[16];     // 0x84; application AID (DF only)
	uint8_t  sfi;         // 0x88; short file ID   (EF only)
	uint8_t  lcs;         // 0x8A; Life cycle stage
	uint8_t compact[8];   // 0x8C; security attributes in compact format  
	uint16_t se;          // 0x8D; the FID of associated securiy environment (DF only)
	uint8_t expanded[20]; // 0xAB; security attribute in expanded format 
	uint32_t data;        // points to the associated data block
} INode;

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
} FileID;

typedef struct {
	FileID     parent_dir;
	FileID     curr_dir;
	FileID     curr_file;
	SuperBlock spr_blk;
	uint32_t   spr_blk_addr;
} ValidityArea;

#ifndef DF_CHILDREN_LIST_LEN
#	define DF_CHILDREN_LIST_LEN 32
#endif

typedef struct {
	uint32_t children_count;
	FileID  children_list[DF_CHILDREN_LIST_LEN];
} FolderData;

#endif /* FUNFS_ISO7816_H */