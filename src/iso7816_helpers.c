#include "iso7816_helpers.h"
#include <string.h>

#define assert_eq(curr, expt) \
	if (curr != expt) {       \
		result = mm_Err;     \
		break;                \
	} 

#define assert_leq(curr, expt) \
	if (curr > expt) {         \
		result = mm_Err;      \
		break;                 \
	}

#define advance(expr) \
	(expr += len)


mm_Result
read_data(uint32_t offset, uint8_t* data, uint32_t len)
{
	mm_Result result   = mm_Ok;
	uint16_t half_word = 0;

	for (uint32_t i = 0; i < len; i += 2) {

		if ((result = mm_read(offset + i, &half_word)) != mm_Ok) {
			break;
		}

		data[i + 1] = (uint8_t)((half_word >> 8) & 0xFF);
		data[i]     = (uint8_t)( half_word       & 0xFF);
		half_word   = 0;
	}

	return result;
}

mm_Result
write_data(uint32_t offset, uint8_t* data, uint32_t len)
{
	mm_Result result   = mm_Ok;
	uint16_t half_word = 0;

	for (uint32_t i = 0; i < len; i += 2) {

		half_word |= (uint16_t)data[i + 1] << 8;
		half_word |= (uint16_t)data[i] & 0x00FF;

		if ((result = mm_write(offset + i, half_word)) != mm_Ok) {
			break;
		}

		half_word = 0;
	}

	return result;
}

/** Adds to the DF_Payload::entries array an entry about newly created file */
static mm_Result
add_record_to_parent(ValidityArea* va, INode* current)
{
	mm_Result result = mm_Ok;

	// choose the parent depending on file type to be created:
	// If we're about to create an EF, then we should start updating the 'size' field from current folder.
	// Otherwise we need to proceed from parent folder.
	uint16_t idx       = (current->desc[0] == ft_DF) ? va->parent_dir.iNode : va->current_dir.iNode;
	INode* inode_array = (INode*)va->spr_blk.inodes_start;
	INode* currentDf   = (INode*)&inode_array[idx];
	
	do {
		uint32_t count = 0;
		
		// get the 'count' of parent folder. It will be used as an index into 'entries' array
		if ((result = read_data(df_data(currentDf)->count, (uint8_t*)&count,  sizeof(uint32_t))) != mm_Ok) {
			break;
		}
		
		DF_Record newRecord = {.iNode = va->spr_blk.inodes_count, .fid = current->fid};
		// write the record about a new child of current folder.
		if ((result = write_data((uint32_t)&df_entries(currentDf)[count++], (uint8_t*)&newRecord, sizeof(DF_Record))) != mm_Ok) {
			break;
		}
		
		// update the 'count' of parent folder.
		if ((result = write_data(df_data(currentDf)->count, (uint8_t*)&count,  sizeof(uint32_t))) != mm_Ok) {
			break;
		}

	} while (0);

	return result;
}

/** traverse the path from this DF right to the MF,
 * increasing the inode->size value of each parent folder. */
static mm_Result
update_parent_size(ValidityArea* va, INode* current)
{
	mm_Result result = mm_Ok;
	INode* inode_array = (INode*)va->spr_blk.inodes_start;
	uint16_t idx = va->parent_dir.iNode;
	uint16_t fid = va->parent_dir.fid;

	do {
		INode* parent = (INode*)&inode_array[idx];
		if (parent->desc[0] != ft_DF) {
			result = mm_Err;
			break;
		}
		// add the size of newly create folder to the size of subsequent parent folder.
		uint16_t updated_size = parent->size + current->size;
		if ((result = write_data((uint32_t)&inode_array[idx].size, (uint8_t*)&updated_size, sizeof(uint16_t))) != mm_Ok) {
			result = mm_writeErr;
			break;
		}

		// get the inode index of subsequent parent.
		idx = df_entries(parent)[1].iNode;
		fid = df_entries(parent)[1].fid;

	} while (fid != 0x00); // fid=FFFF means we've reached the end of file hierarchy

	return result;
}

static mm_Result
data_block_for_df(ValidityArea* va, INode* new_df_node)
{
	mm_Result result = mm_Ok;

	do {
		// Assigning 3F00 to some another file is forbidden.
		if ((va->spr_blk.inodes_count != 0x00) && (new_df_node->fid == FID_MASTER_FILE)) {
			result = mm_Err;
			break;
		}

		// The size of DF is always 256 bytes.
		new_df_node->size = sizeof(DF_Payload);
		// allocate a data block for the newly created DF
		if ((new_df_node->data = mm_allocate(new_df_node->size)) == 0x00) {
			result = mm_writeErr;
			break;
		}

		// MIND THE SEQUENCE! firstable update the parent, and only after current.
		hlp_va_set_parent_df(va, va->current_dir.fid, va->current_dir.iNode); // Current dir becomes 'parent'
		hlp_va_set_current_df(va, new_df_node->fid, va->spr_blk.inodes_count);  // New one becomes 'current'
		
		// In each newly created dir, the first two records goes for itself and its parent
		uint32_t count = 0;
		write_data((uint32_t)&df_entries(new_df_node)[count++], (uint8_t*)&va->current_dir, sizeof(DF_Record));
		write_data((uint32_t)&df_entries(new_df_node)[count++], (uint8_t*)&va->parent_dir,  sizeof(DF_Record));
		write_data(df_data(new_df_node)->count, (uint8_t*)&count,  sizeof(uint32_t));
	} while (0);

	return result;
}

static mm_Result
data_block_for_ef(ValidityArea* va, INode* new_ef_node)
{
	mm_Result result = mm_Ok;
	do {
		// allocate a data block for the newly created DF
		if ((new_ef_node->data = mm_allocate(new_ef_node->size)) == 0x00) {
			result = mm_writeErr;
			break;
		}
		hlp_va_set_current_ef(va, new_ef_node->fid, va->spr_blk.inodes_count);
	} while (0);

	return result;
}

uint16_t
hlp_get_short(uint8_t* buff)
{
	return (((uint16_t)buff[0] << 8) | ((uint16_t)buff[1] & 0xFF));
}

void
hlp_va_set_parent_df(ValidityArea* va, uint16_t fid, uint16_t node)
{
	va->parent_dir.fid   = fid;
	va->parent_dir.iNode = node;
}

void
hlp_va_set_current_df(ValidityArea* va, uint16_t fid, uint16_t node)
{
	va->current_dir.fid   = fid;
	va->current_dir.iNode = node;
}

void
hlp_va_set_current_ef(ValidityArea* va, uint16_t fid, uint16_t node)
{
	va->current_file.fid   = fid;
	va->current_file.iNode = node;
}

/** TODO: implement parameters consistency check */
mm_Result
hlp_parse_params(INode* inode, uint8_t* data, uint32_t data_len)
{
	mm_Result result = mm_Ok;
	uint8_t* curr = data;
	uint8_t* end  = data + data_len;

	// 1. parse input string
	do {
		uint8_t tag = *curr++;
		uint8_t len = *curr++;

		if (tag != 0x62) {
			result = mm_Err;
			break;
		}
		
		while (curr < end) {
			tag = *curr++;
			len = *curr++;
			switch (tag) {
				case 0x80: { // File size
					assert_eq(len, 2);
					inode->size = hlp_get_short(curr);
					advance(curr);
				} break;
				case 0x82: { // file descriptor (e.g. DF, EF, LF, etc.)
					assert_leq(len, 5);
					memcpy(&inode->desc, curr, len);
					advance(curr);
				} break;
				case 0x83: { // file ID (e.g. 3F00, 0101, etc.)
					assert_eq(len, 2);
					inode->fid = hlp_get_short(curr);
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
					inode->se = hlp_get_short(curr);
					advance(curr);
				} break;
				case 0xAB: {
					assert_leq(len, 20);
					memcpy(&inode->expanded, curr, len);
					advance(curr);
				} break;
				default: {
					result = mm_Err;
				} break;
			}

			if (result != mm_Ok)
				break;
		}

	} while (0);

	return result;
}

mm_Result
hlp_allocate_data_block(ValidityArea* va, INode* inode)
{
	mm_Result result = mm_Err;
	FileType type = inode->desc[0];
	
	do {
		// the first file must be of type MF.
		if ((va->spr_blk.inodes_count == 0x00) && (inode->fid != FID_MASTER_FILE)) {
			break;
		}
		if (type == ft_DF) {
			result = data_block_for_df(va, inode);
		} else if (type == ft_EF) {
			result = data_block_for_ef(va, inode);
		} else {
			break;
		}
		// If this is the MF, then there is no parent dir at all. Bail out.
		if (inode->fid == FID_MASTER_FILE) {
			break;
		}
		// update DF_Record array of its parent
		if ((result = add_record_to_parent(va, inode)) != mm_Ok) {
			break;
		}
		// and update the 'size' field of its parent dirs.
		if ((result = update_parent_size(va, inode)) != mm_Ok) {
			break;
		}
		result = mm_Ok;
	} while (0);

	return result;
}

mm_Result
hlp_store_inode(ValidityArea* va, INode* inode)
{
	mm_Result result = mm_Err;

	do {
		INode* inode_array = (INode*)va->spr_blk.inodes_start;
		if ((result = write_data((uint32_t)&inode_array[va->spr_blk.inodes_count], (uint8_t*)inode, sizeof(INode))) != mm_Ok) {
			break;
		}

		// update the number of Inodes in INode table
		va->spr_blk.inodes_count++;

		// store the updated state of SuperBlock
		if ((result = write_data(va->spr_blk_addr, (uint8_t*)&va->spr_blk, sizeof(SuperBlock))) != mm_Ok) {
			break;
		}

	} while (0);

	return result;
}