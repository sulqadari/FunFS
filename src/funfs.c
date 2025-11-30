#include "funfs.h"
#include "flash_emu.h"
#include <string.h>

typedef struct {
	DfEntry     parent_dir;
	DfEntry     current_dir;
	DfEntry     current_file;
	SuperBlock  sblk;
	uint32_t    sblk_addr;
} ValidityArea;

static ValidityArea va;

/** pointer to the beginning of DfPayload field. */
#define df_data(expr) (uint32_t)&((DfPayload*)expr->data)
/** pointer to the DfPayload::entries array */
#define df_entries(expr) ((DfPayload*)expr->data)->entries

static void
va_set_parent_df(uint16_t fid, uint16_t node)
{
	va.parent_dir.iNode = node;
	va.parent_dir.fid = fid;
}

static void
va_set_current_df(uint16_t fid, uint16_t node)
{
	va.current_dir.fid   = fid;
	va.current_dir.iNode = node;
}

static void
va_set_current_ef(uint16_t fid, uint16_t node)
{
	va.current_file.fid   = fid;
	va.current_file.iNode = node;
}

FmResult
ffs_initialize(void)
{
	FmResult result = fmr_Err;
	
	(void)sizeof(DfEntry);
	(void)sizeof(DfPayload);
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
			va_set_current_df(FID_MASTER_FILE, 0x00);          // Set the MF as the current dir
			va_set_parent_df(0x00,0x00); // The master file hasn't parent dir.
			va_set_current_ef(FID_NONE, FID_NONE);             // At startup there is no
			break;
		}

		// OTHERWISE: this is the very first run: no MF, no file system, no nothing.
		if ((va.sblk_addr = femu_allocate(sizeof(SuperBlock))) == 0) { // allocate space for the SuperBlock
			result = fmr_Err;
			break;
		}

		uint32_t size = PAGE_SIZE * 6;	// allocate space for the Inodes table.
		if ((va.sblk.inodes_start = femu_allocate(size)) == 0) {
			result = fmr_Err;
			break;
		}

		va.sblk.magic           = 0xCAFEBABE;
		va.sblk.inodes_count    = 0x00;
		va.sblk.inodes_capacity = size / sizeof(Inode);	// 1024 / 64 = 96
		result = femu_write(va.sblk_addr, (uint8_t*)&va.sblk, sizeof(SuperBlock)); // store the state of SuperBlock

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

#define advance(expr) (expr += len)

static uint16_t
get_short(uint8_t* buff)
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
					inode->size = get_short(curr);
					advance(curr);
				} break;
				case 0x82: { // file descriptor (e.g. DF, EF, LF, etc.)
					assert_leq(len, 5);
					memcpy(&inode->desc, curr, len);
					advance(curr);
				} break;
				case 0x83: { // file ID (e.g. 3F00, 0101, etc.)
					assert_eq(len, 2);
					inode->fid = get_short(curr);
					advance(curr);
				} break;
				case 0x84: { // application AID (for DFs only)
					assert_leq(len, 16);
					memcpy(&inode->aid, curr, len);
					advance(curr);
				} break;
				case 0x88: { // short file ID
					assert_eq(len, 1);
					inode->sfi = *curr;
					advance(curr);
				} break;
				case 0x8A: { // Life cycle stage
					assert_eq(len, 1);
					inode->lcs = *curr;
					advance(curr);
				} break;
				case 0x8C: {  // security attributes in compact format
					assert_leq(len, 7);
					memcpy(&inode->compact, curr, len);
					advance(curr);
				} break;
				case 0x8D: { // the FID of associated securiy environment
					assert_eq(len, 2);
					inode->se = get_short(curr);
					advance(curr);
				} break;
				case 0xAB: {
					assert_leq(len, 20);
					memcpy(&inode->expanded, curr, len);
					advance(curr);
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

/** Adds to the DfPayload::entries array an entry about newly created file */
static FmResult
add_record_to_parent(Inode* current)
{
	FmResult result    = fmr_Ok;

	// choose the parent depending on file type to be created:
	// If we're about to create an EF, then we should write data into currentDF.
	// Otherwise we need to update DfPayload::entries of the parentDF.
	uint16_t idx       = (current->desc[0] == ft_DF) ? va.parent_dir.iNode : va.current_dir.iNode;
	Inode* inode_array = (Inode*)va.sblk.inodes_start;
	Inode* currentDf   = (Inode*)&inode_array[idx];
	DfEntry newRecord  = {
		.iNode = va.sblk.inodes_count,
		.fid = current->fid
	};

	do {
		uint32_t count = 0;
		
		// get the 'count' of parent folder. It will be used as an index into 'entries' array
		if ((result = femu_read(df_data(currentDf)->count, (uint8_t*)&count,  sizeof(uint32_t))) != fmr_Ok) {
			break;
		}
		
		// write the record about a new child of current folder.
		if ((result = femu_write((uint32_t)&df_entries(currentDf)[count++], (uint8_t*)&newRecord, sizeof(DfEntry))) != fmr_Ok) {
			break;
		}
		
		// update the 'count'
		if ((result = femu_write(df_data(currentDf)->count, (uint8_t*)&count,  sizeof(uint32_t))) != fmr_Ok) {
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

		// get the inode index of subsequent parent.
		idx = df_entries(parent)[1].iNode;
		fid = df_entries(parent)[1].fid;

	} while (fid != 0); // fid=0000 means we've reached the end of file hierarchy

	return result;
}

static FmResult
data_block_for_df(Inode* new_df_node)
{
	FmResult result = fmr_Ok;

	do {
		// Assigning 3F00 to some another file is forbidden.
		if ((va.sblk.inodes_count != 0x00) && (new_df_node->fid == FID_MASTER_FILE)) {
			result = fmr_Err;
			break;
		}

		// the first file must be of type MF.
		if ((va.sblk.inodes_count == 0x00) && (new_df_node->fid != FID_MASTER_FILE)) {
			result = fmr_Err;
			break;
		}

		// The size of DF is always 256 bytes.
		new_df_node->size = sizeof(DfPayload);
		// allocate a data block for the newly created DF
		if ((new_df_node->data = femu_allocate(new_df_node->size)) == 0x00) {
			result = fmr_writeErr;
			break;
		}

		// MIND THE SEQUENCE! firstable update the parent, and only after current.
		va_set_parent_df(va.current_dir.fid, va.current_dir.iNode); // Current dir becomes 'parent'
		va_set_current_df(new_df_node->fid, va.sblk.inodes_count);  // New one becomes 'current'
		
		// In each newly created dir, the first two records goes for itself and its parent
		uint32_t count = 0;
		femu_write((uint32_t)&df_entries(new_df_node)[count++], (uint8_t*)&va.current_dir, sizeof(DfEntry));
		femu_write((uint32_t)&df_entries(new_df_node)[count++], (uint8_t*)&va.parent_dir,  sizeof(DfEntry));
		femu_write(df_data(new_df_node)->count, (uint8_t*)&count,  sizeof(uint32_t));
		
		// If this DF is the MF, then there is no parent dir at all. Bail out.
		if (new_df_node->fid == FID_MASTER_FILE) {
			break;
		}
		// Otherwise, update DfEntry array of its parent
		if ((result = add_record_to_parent(new_df_node)) != fmr_Ok) {
			break;
		}
		// and update the 'size' field of its parent dirs.
		if ((result = update_parent_size(new_df_node)) != fmr_Ok) {
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

		memset((uint8_t*)&inode, 0x00, sizeof(Inode));
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
ffs_select_by_path(uint8_t* data, uint32_t data_len)
{
	FmResult result = fmr_Err;
	uint8_t* ptr    = data;

	uint16_t idx       = va.current_dir.iNode;
	Inode* inode_array = (Inode*)va.sblk.inodes_start;
	Inode* currentDf   = (Inode*)&inode_array[idx];
	DfEntry nextFile;
	uint16_t fid;

	// This machinery expects that the data length is multiple of 'uint16_t'.
	uint32_t count = 0;
	do {
		fid = get_short(ptr);
		(void)fid;
		do {
			// get the 'count' of files in current folder.
			if ((result = femu_read(df_data(currentDf)->count, (uint8_t*)&count,  sizeof(uint32_t))) != fmr_Ok) {
				break;
			}

			// read a record of the next child
			if ((result = femu_read((uint32_t)&df_entries(currentDf)[count], (uint8_t*)&nextFile, sizeof(DfEntry))) != fmr_Ok) {
				break;
			}
		} while (0);
		
		
		ptr += 2;
	} while (data_len -= 2);

	return result;
}

FmResult
ffs_select_by_name(const uint16_t fid)
{
	FmResult result = fmr_Err;

	uint16_t idx       = va.current_dir.iNode;
	Inode* inode_array = (Inode*)va.sblk.inodes_start;
	Inode* currentDf   = (Inode*)&inode_array[idx];

	do {
		// special case: MF always present and is always accessible
		if (fid == FID_MASTER_FILE) {
			result = fmr_Ok;
			va_set_current_df(FID_MASTER_FILE, 0x00);
			va_set_parent_df(0x00, 0x00);
			va_set_current_ef(FID_NONE, FID_NONE);
			break;
		}

		uint32_t count = 0;
		// get the 'count' of files in current folder.
		if ((result = femu_read(df_data(currentDf)->count, (uint8_t*)&count,  sizeof(uint32_t))) != fmr_Ok) {
			break;
		}

		DfEntry next;
		for (uint32_t i = 0; i < count; ++i) {
			// read a record of the next child
			if ((result = femu_read((uint32_t)&df_entries(currentDf)[i], (uint8_t*)&next, sizeof(DfEntry))) != fmr_Ok) {
				break;
			}

			if (fid == next.fid) {
				currentDf   = (Inode*)&inode_array[next.iNode];
				uint16_t parent_idx = df_entries(currentDf)[1].iNode;
				uint16_t parent_fid = df_entries(currentDf)[1].fid;
				
				va_set_parent_df(parent_fid, parent_idx); // Current dir becomes 'parent'
				va_set_current_df(next.fid, next.iNode);  // New one becomes 'current'
				break;
			}
		}

	} while (0);

	return result;
}