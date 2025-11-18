#include "funfs.h"
#include "flash_emu.h"


#define PAGE_ALIGNED(expr) (((expr) + 0x3FF) & 0xFFFFFC00)
#define WORD_ALIGNED(expr) (((expr) +  0x03) & 0xFFFFFFFC)

FMResult
ffs_initialize(void)
{
	FMResult result = fmr_Err;
	SuperBlock super;
	uint32_t blocks_start_addr;
	do {
		
		// Open (or create) persistent storage for a flash memory
		if ((result = mmg_open_flash()) != fmr_Ok)
			break;
		
		// Read the 'Super Block' and find out has the file system been initialized or not
		if ((result = mmg_read(fs_start_addr, (uint8_t*)&super, sizeof(SuperBlock))) != fmr_Ok)
			break;
		
		// The file system is already initialized. Bail out.
		if (super.magic == 0xCAFEBABE)
			break;
			
		super.magic             = 0xCAFEBABE;
		super.inodes_total      = 0x00;
		super.inodes_count      = 0x00;
		super.inodes_start      = (Inode*)WORD_ALIGNED(fs_start_addr + sizeof(SuperBlock));
		super.data_blocks_start = (uint32_t)PAGE_ALIGNED((uint32_t)super.inodes_start + PAGE_SIZE * 6);

		if ((result = mmg_write(fs_start_addr, (uint8_t*)&super, sizeof(SuperBlock))) != fmr_Ok)
			break;
		
		uint32_t initial_block_size = FLASH_SIZE_TOTAL - super.data_blocks_start;

		if ((blocks_start_addr = mmg_allocate(super.data_blocks_start, initial_block_size)) == 0) {
			result = fmr_Err;
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
	uint32_t address;

	do {
		if ((result = mmg_read(fs_start_addr, (uint8_t*)&super, sizeof(SuperBlock))) != fmr_Ok)
			break;
		
		super.inodes_start;
		
	} while (0);

	return result;
}

#define assert_eq(curr, expt) \
	if (curr != expt) {       \
		result = fmr_Err;     \
	} break

#define assert_less(curr, expt) \
	if (curr > expt) {          \
		result = fmr_Err;       \
	} break

#define advance() (curr += len)

static uint16_t
from_short(uint8_t* buff)
{
	return (((uint16_t)buff[0] << 8) | ((uint16_t)buff[1] & 0xFF));
}

static FMResult
parse_params(Inode* inode, uint8_t* data, uint32_t data_len)
{
	FMResult result = fmr_Ok;
	uint8_t* curr = data;
	uint8_t* end  = data + data_len;

	// 1. parse input string
	do {
		uint16_t tag = curr++;
		uint16_t len = curr++;

		result = fmr_Err;
		if (tag != 0x62)
			break;
		
		while (curr < end) {
			tag = curr++;
			len = curr++;
			switch (tag) {
				case 0x80: { // File size
					assert_eq(len, 2);
					inode->size = from_short(curr);
					advance();
				} break;
				case 0x82: { // file descriptor (e.g. DF, EF, LF, etc.)
					assert_less(len, 5);
					memcpy(&inode->desc, curr, len);
					advance();
				} break;
				case 0x83: { // file ID (e.g. 3F00, 0101, etc.)
					assert_eq(len, 2);
					inode->fid = from_short(curr);
					advance();
				} break;
				case 0x84: { // application AID (for DFs only)
					assert_less(len, 16);
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
					assert_less(len, 7);
					memcpy(&inode->comp_sa, curr, len);
					advance();
				} break;
				case 0x8D: { // the FID of associated securiy environment
					assert_eq(len, 2);
					inode->se = from_short(curr);
					advance();
				} break;
				case 0xAB: {
					assert_less(len, 20);
					memcpy(&inode->expsa_bytes, curr, len);
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

	do {
		// Read the 'Super Block' and find out has the file system been initialized or not
		if ((result = mmg_read(fs_start_addr, (uint8_t*)&super, sizeof(SuperBlock))) != fmr_Ok)
			break;
		
		if ((inode->data_blk_ptr = mmg_allocate(super.data_blocks_start, inode->size)) == 0x00){
			result = fmr_Err;
			break;
		}

	} while (0);

	return result;
}

FMResult
ffs_create_file(uint8_t* data, uint32_t data_len)
{
	FMResult result = fmr_Ok;
	Inode inode;

	do {
		if ((result = parse_params(&inode, data, data_len)) != fmr_Ok)
			break;

		if ((result = allocate_data_block(&inode)) != fmr_Ok)
			break;
		
		if ((result = store_inode(&inode)) != fmr_Ok)
			break;

	} while (0);
}

FMResult
ffs_select_file(uint32_t fid)
{
	FMResult result = fmr_Err;
	return result;
}