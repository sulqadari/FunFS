#include "funfs.h"
#include "flash_emu.h"
#include <string.h>

typedef struct {
	DfEntry    parent_dir;
	DfEntry    current_dir;
	DfEntry    current_file;
	SuperBlock sblk;
	uint32_t   sblk_addr;
} ValidityArea;

static ValidityArea va;

FmResult
ffs_initialize(void)
{
	FmResult result = fmr_Err;
	
	(void)sizeof(Inode);
	(void)sizeof(SuperBlock);
	(void)sizeof(ValidityArea);
	do {
		memset((uint8_t*)&va, 0x00, sizeof(ValidityArea));

		va.sblk_addr = femu_get_start_address() + sizeof(block_t);

		// Open (or create) persistent storage for a flash memory
		if ((result = femu_open_flash()) != fmr_Ok) {
			break;
		}
		// Read the 'Super Block' and find out has the file system been initialized previously or not
		if ((result = femu_read(va.sblk_addr, (uint8_t*)&va.sblk, sizeof(SuperBlock))) != fmr_Ok) {
			break;
		}

		// IF: the file system is already initialized, then just set the MF as the current folder and bail out.
		if (va.sblk.magic == 0xCAFEBABE) {
			// set the MF as the current dir
			va.parent_dir.fid  = FID_MASTER_FILE;
			va.parent_dir.iNode = 0x00; // Master file has zeroth index in iNode table

			// The master file hasn't parent dir. Thus this field closures to master file it self.
			va.current_dir.fid  = FID_MASTER_FILE;
			va.current_dir.iNode = 0x00; // Master file has zeroth index in iNode table
			
			// At startup there is no
			va.current_file.fid   = FID_NONE;
			va.current_file.iNode = FID_NONE;
			break;
		}

		// OTHERWISE: this is the very first run: no MF, no file system, no nothing.
		// allocate space on the flash for the SuperBlock
		if ((va.sblk_addr = femu_allocate(sizeof(SuperBlock))) == 0) {
			break;
		}

		// allocate  space on the flash for the Inodes table.
		uint32_t size = PAGE_SIZE * 6;
		if ((va.sblk.inodes_start = femu_allocate(size)) == 0) {
			break;
		}

		va.sblk.magic           = 0xCAFEBABE;
		va.sblk.inodes_count    = 0x00;
		va.sblk.inodes_capacity = size / sizeof(Inode);	// if page size equals 1024, then the length
														// of this array is 96 elements
		// store the state of SuperBlock
		result = femu_write(va.sblk_addr, (uint8_t*)&va.sblk, sizeof(SuperBlock));

	} while (0);

	return result;
}

#define assert_eq(curr, expt) \
	if (curr != expt) {       \
		result = fmr_Err;     \
		break;                \
	} 

#define assert_leq(curr, expt) \
	if (curr > expt) {         \
		result = fmr_Err;      \
		break;                 \
	}

#define advance() (curr += len)

static uint16_t
from_short(uint8_t* buff)
{
	return (((uint16_t)buff[0] << 8) | ((uint16_t)buff[1] & 0xFF));
}

/** TODO: implement parameters consistency check */
static FmResult
parse_params(Inode* inode, uint8_t* data, uint32_t data_len)
{
	FmResult result = fmr_Ok;
	uint8_t* curr = data;
	uint8_t* end  = data + data_len;

	// 1. parse input string
	do {
		uint8_t tag = *curr++;
		uint8_t len = *curr++;

		if (tag != 0x62) {
			result = fmr_Err;
			break;
		}
		
		while (curr < end) {
			tag = *curr++;
			len = *curr++;
			switch (tag) {
				case 0x80: { // File size
					assert_eq(len, 2);
					inode->size = from_short(curr);
					advance();
				} break;
				case 0x82: { // file descriptor (e.g. DF, EF, LF, etc.)
					assert_leq(len, 5);
					memcpy(&inode->desc, curr, len);
					advance();
				} break;
				case 0x83: { // file ID (e.g. 3F00, 0101, etc.)
					assert_eq(len, 2);
					inode->fid = from_short(curr);
					advance();
				} break;
				case 0x84: { // application AID (for DFs only)
					assert_leq(len, 16);
					memcpy(&inode->aid, curr, len);
					advance();
				} break;
				case 0x88: { // short file ID
					assert_eq(len, 1);
					inode->sfi = *curr;
					advance();
				} break;
				case 0x8A: { // Life cycle stage
					assert_eq(len, 1);
					inode->lcs = *curr;
					advance();
				} break;
				case 0x8C: {  // security attributes in compact format
					assert_leq(len, 7);
					memcpy(&inode->compact, curr, len);
					advance();
				} break;
				case 0x8D: { // the FID of associated securiy environment
					assert_eq(len, 2);
					inode->se = from_short(curr);
					advance();
				} break;
				case 0xAB: {
					assert_leq(len, 20);
					memcpy(&inode->expanded, curr, len);
					advance();
				} break;
				default: {
					result = fmr_Err;
				} break;
			}

			if (result != fmr_Ok)
				break;
		}

	} while (0);

	return result;
}

/** traverse the path from this DF right to the MF,
 * increasing the inode->size value of each parent folder. */
static FmResult
update_parent_size(Inode* current)
{
	FmResult result = fmr_Ok;
	Inode* inode_array = (Inode*)va.sblk.inodes_start;
	uint16_t idx = va.parent_dir.iNode;
	uint16_t fid = va.parent_dir.fid;
	
	do {
		Inode* parent = (Inode*)&inode_array[idx];
		if (parent->desc[0] != ft_DF) {
			result = fmr_Err;
			break;
		}
		// add the size of newly create folder to the size of subsequent parent folder.
		uint16_t updated_size = parent->size + current->size;
		if ((result = femu_write((uint32_t)&inode_array[idx].size, (uint8_t*)&updated_size, sizeof(uint16_t))) != fmr_Ok) {
			result = fmr_writeErr;
			break;
		}

		// get the inode index of subsequent parent. Note that the '0' points to MF.
		idx = ((DfEntry*)(parent->data + sizeof(DfEntry)))->iNode;
		fid = ((DfEntry*)(parent->data + sizeof(DfEntry)))->fid;

	} while (fid != 0);

	return result;
}

static FmResult
data_block_for_df(Inode* current)
{
	FmResult result = fmr_Ok;

	do {
		// The MF must be the first one and the only in the file system.
		if ((va.sblk.inodes_count != 0x00) && (current->fid == FID_MASTER_FILE)) {
			result = fmr_Err;
			break;
		}

		// The size of DF is always 256 bytes.
		current->size = 256;
		// allocate a data block for the newly created DF
		if ((current->data = femu_allocate(current->size)) == 0x00) {
			result = fmr_writeErr;
			break;
		}

		// the current dir now becomes 'parent' of the newly created one.
		va.parent_dir.fid  = va.current_dir.fid;
		va.parent_dir.iNode = va.current_dir.iNode;

		// the newly created folder becomes the current one.
		va.current_dir.fid = current->fid;
		va.current_dir.iNode = va.sblk.inodes_count;

		femu_write(current->data,                   (uint8_t*)&va.current_dir, sizeof(DfEntry));
		femu_write(current->data + sizeof(DfEntry), (uint8_t*)&va.parent_dir,  sizeof(DfEntry));
		// the MF is the first element in the file system. There is nothing to do.
		if (current->fid == FID_MASTER_FILE) {
			break;
		}

		if ((result = update_parent_size(current)) != fmr_Ok) {
			break;
		}

	} while (0);

	return result;
}

static FmResult
data_block_for_ef(Inode* inode)
{
	FmResult result = fmr_Ok;

	do {

	} while (0);

	return result;
}

static FmResult
allocate_data_block(Inode* inode)
{
	FmResult result = fmr_Ok;
	FileType type = inode->desc[0];
	
	do {
		if (type == ft_DF) {
			result = data_block_for_df(inode);
		} else if (type == ft_EF) {
			result = data_block_for_ef(inode);
		} else {
			result = fmr_Err;
		}

	} while (0);
	return result;
}

static FmResult
store_inode(Inode* inode)
{
	FmResult result = fmr_Err;

	do {
		Inode* inode_array = (Inode*)va.sblk.inodes_start;
		if ((result = femu_write((uint32_t)&inode_array[va.sblk.inodes_count], (uint8_t*)inode, sizeof(Inode))) != fmr_Ok) {
			break;
		}

		// update the number of Inodes in Inode table
		va.sblk.inodes_count++;

		// store the updated state of SuperBlock
		if ((result = femu_write(va.sblk_addr, (uint8_t*)&va.sblk, sizeof(SuperBlock))) != fmr_Ok) {
			break;
		}

	} while (0);

	return result;
}

FmResult
ffs_create_file(uint8_t* data, uint32_t data_len)
{
	FmResult result = fmr_InodeTableFull;
	Inode inode;

	do {
		// we cant't create more than 'inodes_capacity' files.
		if (va.sblk.inodes_count >= va.sblk.inodes_capacity) {
			break;
		}

		memset((uint8_t*)&inode,0x00, sizeof(Inode));
		if ((result = parse_params(&inode, data, data_len)) != fmr_Ok)
			break;

		if ((result = allocate_data_block(&inode)) != fmr_Ok)
			break;
		
		if ((result = store_inode(&inode)) != fmr_Ok)
			break;

	} while (0);
	return result;
}

FmResult
ffs_select_file(uint32_t fid)
{
	FmResult result = fmr_Err;
	return result;
}