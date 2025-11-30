#include "iso7816.h"
#include "flash_emu.h"
#include <string.h>

typedef struct {
	DF_Record  parent_dir;
	DF_Record  current_dir;
	DF_Record  current_file;
	SuperBlock sblk;
	uint32_t   sblk_addr;
} ValidityArea;

static ValidityArea va;

static void
va_set_parent_df(uint16_t fid, uint16_t node)
{
	va.parent_dir.fid   = fid;
	va.parent_dir.iNode = node;
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

#define advance(expr) \
	(expr += len)

static uint16_t
get_short(uint8_t* buff)
{
	return (((uint16_t)buff[0] << 8) | ((uint16_t)buff[1] & 0xFF));
}
	
/** TODO: implement parameters consistency check */
static emu_Result
parse_params(INode* inode, uint8_t* data, uint32_t data_len)
{
	emu_Result result = fmr_Ok;
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

/** pointer to the beginning of DF_Payload field. */
#define df_data(expr) (uint32_t)&((DF_Payload*)expr->data)
/** pointer to the DF_Payload::entries array */
#define df_entries(expr) ((DF_Payload*)expr->data)->entries

/** Adds to the DF_Payload::entries array an entry about newly created file */
static emu_Result
add_record_to_parent(INode* current)
{
	emu_Result result = fmr_Ok;

	// choose the parent depending on file type to be created:
	// If we're about to create an EF, then we should start updating the 'size' field from current folder.
	// Otherwise we need to proceed from parent folder.
	uint16_t idx       = (current->desc[0] == ft_DF) ? va.parent_dir.iNode : va.current_dir.iNode;
	INode* inode_array = (INode*)va.sblk.inodes_start;
	INode* currentDf   = (INode*)&inode_array[idx];
	
	do {
		uint32_t count = 0;
		
		// get the 'count' of parent folder. It will be used as an index into 'entries' array
		if ((result = emu_read(df_data(currentDf)->count, (uint8_t*)&count,  sizeof(uint32_t))) != fmr_Ok) {
			break;
		}
		
		DF_Record newRecord = {.iNode = va.sblk.inodes_count, .fid = current->fid};
		// write the record about a new child of current folder.
		if ((result = emu_write((uint32_t)&df_entries(currentDf)[count++], (uint8_t*)&newRecord, sizeof(DF_Record))) != fmr_Ok) {
			break;
		}
		
		// update the 'count' of parent folder.
		if ((result = emu_write(df_data(currentDf)->count, (uint8_t*)&count,  sizeof(uint32_t))) != fmr_Ok) {
			break;
		}

	} while (0);

	return result;
}

/** traverse the path from this DF right to the MF,
 * increasing the inode->size value of each parent folder. */
static emu_Result
update_parent_size(INode* current)
{
	emu_Result result = fmr_Ok;
	INode* inode_array = (INode*)va.sblk.inodes_start;
	uint16_t idx = va.parent_dir.iNode;
	uint16_t fid = va.parent_dir.fid;

	do {
		INode* parent = (INode*)&inode_array[idx];
		if (parent->desc[0] != ft_DF) {
			result = fmr_Err;
			break;
		}
		// add the size of newly create folder to the size of subsequent parent folder.
		uint16_t updated_size = parent->size + current->size;
		if ((result = emu_write((uint32_t)&inode_array[idx].size, (uint8_t*)&updated_size, sizeof(uint16_t))) != fmr_Ok) {
			result = fmr_writeErr;
			break;
		}

		// get the inode index of subsequent parent.
		idx = df_entries(parent)[1].iNode;
		fid = df_entries(parent)[1].fid;

	} while (fid != 0); // fid=0000 means we've reached the end of file hierarchy

	return result;
}

static emu_Result
data_block_for_df(INode* new_df_node)
{
	emu_Result result = fmr_Ok;

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
		new_df_node->size = sizeof(DF_Payload);
		// allocate a data block for the newly created DF
		if ((new_df_node->data = emu_allocate(new_df_node->size)) == 0x00) {
			result = fmr_writeErr;
			break;
		}

		// MIND THE SEQUENCE! firstable update the parent, and only after current.
		va_set_parent_df(va.current_dir.fid, va.current_dir.iNode); // Current dir becomes 'parent'
		va_set_current_df(new_df_node->fid, va.sblk.inodes_count);  // New one becomes 'current'
		
		// In each newly created dir, the first two records goes for itself and its parent
		uint32_t count = 0;
		emu_write((uint32_t)&df_entries(new_df_node)[count++], (uint8_t*)&va.current_dir, sizeof(DF_Record));
		emu_write((uint32_t)&df_entries(new_df_node)[count++], (uint8_t*)&va.parent_dir,  sizeof(DF_Record));
		emu_write(df_data(new_df_node)->count, (uint8_t*)&count,  sizeof(uint32_t));
		
		// If this DF is the MF, then there is no parent dir at all. Bail out.
		if (new_df_node->fid == FID_MASTER_FILE) {
			break;
		}
		// Otherwise, update DF_Record array of its parent
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

static emu_Result
data_block_for_ef(INode* new_ef_node)
{
	emu_Result result = fmr_Ok;

	do {
		
		// allocate a data block for the newly created DF
		if ((new_ef_node->data = emu_allocate(new_ef_node->size)) == 0x00) {
			result = fmr_writeErr;
			break;
		}

		va_set_current_ef(new_ef_node->fid, va.sblk.inodes_count);

		// update DF_Record array of its parent
		if ((result = add_record_to_parent(new_ef_node)) != fmr_Ok) {
			break;
		}
		// and update the 'size' field of its parent dirs.
		if ((result = update_parent_size(new_ef_node)) != fmr_Ok) {
			break;
		}

	} while (0);

	return result;
}

static emu_Result
allocate_data_block(INode* inode)
{
	emu_Result result = fmr_Ok;
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

static emu_Result
store_inode(INode* inode)
{
	emu_Result result = fmr_Err;

	do {
		INode* inode_array = (INode*)va.sblk.inodes_start;
		if ((result = emu_write((uint32_t)&inode_array[va.sblk.inodes_count], (uint8_t*)inode, sizeof(INode))) != fmr_Ok) {
			break;
		}

		// update the number of Inodes in INode table
		va.sblk.inodes_count++;

		// store the updated state of SuperBlock
		if ((result = emu_write(va.sblk_addr, (uint8_t*)&va.sblk, sizeof(SuperBlock))) != fmr_Ok) {
			break;
		}

	} while (0);

	return result;
}

ISO_SW
iso_initialize(void)
{
	ISO_SW result = SW_MEMORY_FAILURE;
	
	(void)sizeof(DF_Record);
	(void)sizeof(DF_Payload);
	(void)sizeof(INode);
	(void)sizeof(SuperBlock);
	(void)sizeof(ValidityArea);
	do {
		memset((uint8_t*)&va, 0x00, sizeof(ValidityArea));

		va.sblk_addr = emu_get_start_address() + sizeof(block_t);

		// Open (or create) persistent storage for a flash memory
		if (emu_open_flash() != fmr_Ok) {
			break;
		}
		// Read the 'Super Block' and find out has the file system been initialized previously or not
		if (emu_read(va.sblk_addr, (uint8_t*)&va.sblk, sizeof(SuperBlock)) != fmr_Ok) {
			break;
		}

		// IF: the file system is already initialized, then just set the MF as the current folder and bail out.
		if (va.sblk.magic == 0xCAFEBABE) {
			va_set_current_df(FID_MASTER_FILE, 0x00);          // Set the MF as the current dir
			va_set_parent_df(0x00,0x00); // The master file hasn't parent dir.
			va_set_current_ef(FID_NONE, FID_NONE);             // At startup there is no
			result = SW_OK;
			break;
		}

		// OTHERWISE: this is the very first run: no MF, no file system, no nothing.
		if ((va.sblk_addr = emu_allocate(sizeof(SuperBlock))) == 0) { // allocate space for the SuperBlock
			break;
		}

		uint32_t size = PAGE_SIZE * 6;	// allocate space for the Inodes table.
		if ((va.sblk.inodes_start = emu_allocate(size)) == 0) {
			break;
		}

		va.sblk.magic           = 0xCAFEBABE;
		va.sblk.inodes_count    = 0x00;
		va.sblk.inodes_capacity = size / sizeof(INode);	// 1024 / 64 = 96
		if (emu_write(va.sblk_addr, (uint8_t*)&va.sblk, sizeof(SuperBlock)) != fmr_Ok) { // store the state of SuperBlock
			break;
		}

		result = SW_OK;
	} while (0);

	return result;
}

ISO_SW
iso_create_file(uint8_t* data, uint32_t data_len)
{
	ISO_SW result = SW_UNKNOWN;
	INode inode;

	do {
		// we cant't create more than 'inodes_capacity' files.
		if (va.sblk.inodes_count >= va.sblk.inodes_capacity) {
			break;
		}

		memset((uint8_t*)&inode, 0x00, sizeof(INode));
		if (parse_params(&inode, data, data_len) != fmr_Ok) {
			break;
		}
		if (allocate_data_block(&inode) != fmr_Ok) {
			break;
		}
		if (store_inode(&inode) != fmr_Ok) {
			break;
		}

		result = SW_OK;
	} while (0);
	return result;
}

ISO_SW
iso_select_by_name(const uint16_t fid)
{
	ISO_SW result = SW_UNKNOWN;

	uint16_t idx       = va.current_dir.iNode;
	INode* inode_array = (INode*)va.sblk.inodes_start;
	INode* currentDf   = (INode*)&inode_array[idx];

	do {
		// special case: MF always present and is always accessible
		if (fid == FID_MASTER_FILE) {
			result = SW_OK;
			va_set_current_df(FID_MASTER_FILE, 0x00);
			va_set_parent_df(0x00, 0x00);
			va_set_current_ef(FID_NONE, FID_NONE);
			break;
		}

		uint32_t count = 0;
		// get the 'count' of files in current folder.
		if (emu_read(df_data(currentDf)->count, (uint8_t*)&count,  sizeof(uint32_t)) != fmr_Ok) {
			break;
		}

		DF_Record next;
		uint32_t i;
		for (i = 0; i < count; ++i) {
			// read from current DF's payload region an info about subsequent child. 
			if (emu_read((uint32_t)&df_entries(currentDf)[i], (uint8_t*)&next, sizeof(DF_Record)) != fmr_Ok) {
				break;
			}

			if (fid == next.fid) {
				currentDf = (INode*)&inode_array[next.iNode];
				
				// If the file we have found isn't of type DF, then just update the 'current file' field
				// of VA and return.
				if (currentDf->desc[0] != ft_DF) {
					va_set_current_ef(next.fid, next.iNode);
					break;
				}

				// Otherwise, update the 'parent DF/child DF' values.
				uint16_t parent_idx = df_entries(currentDf)[1].iNode;
				uint16_t parent_fid = df_entries(currentDf)[1].fid;

				va_set_parent_df(parent_fid, parent_idx); // Current dir becomes 'parent'
				va_set_current_df(next.fid, next.iNode);  // the one we're looking for becomes 'current'
				break;
			}
		}

		if (i >= count) {
			break;
		}
		result = SW_OK;
	} while (0);

	return result;
}

ISO_SW
iso_select_by_path(uint8_t* data, uint32_t data_len)
{
	ISO_SW result = SW_UNKNOWN;
#if (0)
	uint8_t* ptr    = data;

	uint16_t idx       = va.current_dir.iNode;
	INode* inode_array = (INode*)va.sblk.inodes_start;
	INode* currentDf   = (INode*)&inode_array[idx];
	DF_Record nextFile;
	uint16_t fid;

	// This machinery expects that the data length is multiple of 'uint16_t'.
	uint32_t count = 0;
	do {
		fid = get_short(ptr);
		(void)fid;
		
		// TODO
		
		ptr += 2;
	} while (data_len -= 2);
#endif /* 0 */

	return result;
}