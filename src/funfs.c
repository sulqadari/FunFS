#include "funfs.h"
#include "flash_emu.h"
#include <string.h>

static uint32_t super_blk_addr;

typedef struct {
	uint16_t parent_df;
	uint16_t curr_df;
	uint16_t curr_ef;
} ValidityArea;

ValidityArea va;

FMResult
ffs_initialize(void)
{
	FMResult result = fmr_Err;
	SuperBlock super;
	
	(void)sizeof(Inode);
	(void)sizeof(SuperBlock);
	do {
		
		super_blk_addr = femu_get_start_address() + sizeof(DataBlk);

		// Open (or create) persistent storage for a flash memory
		if ((result = femu_open_flash()) != fmr_Ok)
			break;
		
		// Read the 'Super Block' and find out has the file system been initialized previously or not
		if ((result = femu_read(super_blk_addr, (uint8_t*)&super, sizeof(SuperBlock))) != fmr_Ok)
			break;
		
		// The file system is already initialized. Bail out.
		if (super.magic == 0xCAFEBABE) {
			va.curr_df   = FID_MASTER_FILE;
			va.parent_df = FID_MASTER_FILE;
			break;
		}

		// allocate space on the flash for the SuperBlock
		if ((super_blk_addr = femu_allocate(sizeof(SuperBlock))) == 0)
			break;

		uint32_t size = PAGE_SIZE * 6;
		if ((super.inodes_start = femu_allocate(size)) == 0)
			break;

		super.magic           = 0xCAFEBABE;
		super.inodes_count    = 0x00;
		super.inodes_capacity = size / sizeof(Inode);	// if page size equals 1024, then the length
														// of this array is 96 elements

		// store the state of SuperBlock
		if ((result = femu_write(super_blk_addr, (uint8_t*)&super, sizeof(SuperBlock))) != fmr_Ok)
			break;

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
static FMResult
parse_params(Inode* inode, uint8_t* data, uint32_t data_len)
{
	FMResult result = fmr_Ok;
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

static FMResult
allocate_data_block(Inode* inode)
{
	FMResult result = fmr_Ok;
	SuperBlock super;
	FileType type = inode->desc[0];
	uint32_t size = (type == ft_DF) ? 256 : (inode->size & 0xFFFF);

	do {
		if ((result = femu_read(super_blk_addr, (uint8_t*)&super, sizeof(SuperBlock))) != fmr_Ok){
			break;
		}

		if ((inode->data = femu_allocate(size)) == 0x00) {
			result = fmr_Err;
			break;
		}

		if (type == ft_DF) {
			DfEntry entry;
			entry.iNode = super.inodes_count;
			entry.fid = inode->fid;

			if ((result = femu_write(inode->data, (uint8_t*)&entry, sizeof(DfEntry))) != fmr_Ok)
				break;
			
			if ((result = femu_write(inode->data + sizeof(DfEntry), (uint8_t*)&entry, sizeof(DfEntry))) != fmr_Ok)
				break;
		}
	} while (0);

	return result;
}

static FMResult
store_inode(Inode* inode)
{
	FMResult result = fmr_Err;
	SuperBlock super;
	Inode* inode_array;

	do {
		if ((result = femu_read(super_blk_addr, (uint8_t*)&super, sizeof(SuperBlock))) != fmr_Ok)
			break;
		
		if (super.inodes_count >= super.inodes_capacity)
			break;
		
		inode_array = (Inode*)super.inodes_start;
		
		if ((result = femu_write((uint32_t)&inode_array[super.inodes_count], (uint8_t*)inode, sizeof(Inode))) != fmr_Ok)
			break;
		
		super.inodes_count++;

		// store the state of SuperBlock
		if ((result = femu_write(super_blk_addr, (uint8_t*)&super, sizeof(SuperBlock))) != fmr_Ok)
			break;
		
	} while (0);

	return result;
}

FMResult
ffs_create_file(uint8_t* data, uint32_t data_len)
{
	FMResult result = fmr_Ok;
	Inode inode;

	do {
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

FMResult
ffs_select_file(uint32_t fid)
{
	FMResult result = fmr_Err;
	return result;
}