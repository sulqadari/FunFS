#include "iso7816_helpers.h"
#include <string.h>

#define assert_eq(curr, expt, sw) \
	if (curr != expt) {           \
		result = sw;              \
		break;                    \
	} 

#define assert_leq(curr, expt, sw) \
	if (curr > expt) {             \
		result = sw;               \
		break;                     \
	}

#define advance(expr) \
	(expr += len)


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
	va->curr_dir.fid   = fid;
	va->curr_dir.iNode = node;
}

void
hlp_va_set_current_ef(ValidityArea* va, uint16_t fid, uint16_t node)
{
	va->curr_file.fid   = fid;
	va->curr_file.iNode = node;
}

ISO_SW
hlp_read_data(uint32_t offset, uint8_t* data, uint32_t len)
{
	// DBG_PRINT_VARG("hlp_read_data(offset: %04X)\n", offset)

	ISO_SW result = SW_OK;
	uint8_t byte  = 0;

	for (uint32_t i = 0; i < len; ++i) {

		if (mm_read(offset + i, &byte) != mm_Ok) {
			result = SW_MEMORY_FAILURE;
			break;
		}

		data[i] = byte;

	}

	return result;
}

ISO_SW
hlp_write_data(uint32_t offset, uint8_t* data, uint32_t len)
{
	// DBG_PRINT_VARG("hlp_write_data(offset: %04X)\n", offset)
	ISO_SW result      = SW_OK;
	uint16_t half_word = 0;
	uint8_t len_tail   = len & 0x01;

	do {
		// 1. check if an area we're about to write into is clear or not
		uint32_t i = 0;
		for (; i < len; i += 2) {
			if ((result = hlp_read_data(offset + i, (uint8_t*)&half_word, 2)) != SW_OK) {
				break;
			}
			if (half_word != 0xFFFF) {
				break;
			}
		}

		if (result != SW_OK){
			break;
		}

		// the number of bytes to be read is odd. That's mean that we've read
		// one excess byte and it should be cleaned up.
		// Also this assertion is intended to prevent erasing a half word which
		// is isn't the last one, i.e. we have encountered a non-zeroed bytes in
		// the middle of area we're about to update.
		if (i >= len && len_tail) {
			half_word |= 0x00FF;
		}

		// 2. If even a word isn't clear, then call mm_rewrite_page()
		if (half_word != 0xFFFF) {
			uint32_t start_page = PAGE_ALIGN(offset);
			if (mm_rewrite_page(start_page, offset, data, len) != mm_Ok) {
				result = SW_MEMORY_FAILURE;
				break;
			}

		  // 3. else - just write data starting from the given offset.
		} else {
			len &= 0xFE; // only even number is allowed because writing is performed on a half-word base
			half_word = 0;
			for (uint32_t i = 0; i < len; i += 2) {
				half_word |= (uint16_t)data[i + 0] & 0x00FF;
				half_word |= (uint16_t)data[i + 1] << 8;

				if (mm_write(offset + i, half_word) != mm_Ok) {
					result = SW_MEMORY_FAILURE;
					break;
				}

				half_word = 0;
			}

			// we have one byte to be written
			if (len_tail) {
				half_word = 0xFF00;
				half_word |= data[len];

				if (mm_write(offset + len, half_word) != mm_Ok) {
					result = SW_MEMORY_FAILURE;
				}
			}
		}
	} while (0);

	return result;
}

static ISO_SW
check_df(INode* inode)
{
	ISO_SW result = SW_INCORRECT_PARAMS_IN_CDATA;

	do {
		if (inode->size != 0x00) {
			break;
		}

		if (inode->desc[5] != 1) { // the length of descriptor string must be 2 bytes
			break;
		}

		if (inode->sfi != 0x00) {
			break;
		}

		result = SW_OK;
	} while (0);

	return result;
}

static ISO_SW
check_ef(INode* inode)
{
	ISO_SW result = SW_INCORRECT_PARAMS_IN_CDATA;

	do {
		if (inode->size == 0x00) {
			break;
		}

		if (inode->desc[5] != 2) { // the length of descriptor string must be 2 bytes
			break;
		}

		if (inode->sfi == 0x00) {
			inode->sfi = inode->fid & 0x01F;
		}

		result = SW_OK;
	} while (0);

	return result;
}

ISO_SW
hlp_check_consistency(INode* inode)
{
	ISO_SW result = SW_INCORRECT_PARAMS_IN_CDATA;

	do {
		if (inode->fid == FID_RESERVED_1 ||
			inode->fid == FID_RESERVED_2 ||
			inode->fid == FID_RESERVED_3) {
			break;
		}
		
		if (inode->desc[0] == ft_DF) {
			result = check_df(inode);
		} else if (inode->desc[0] == ft_EF) {
			result = check_ef(inode);
		}

	} while (0);

	return result;
}

ISO_SW
hlp_parse_params(INode* inode, uint8_t* data, uint32_t data_len)
{
	ISO_SW result = SW_OK;
	uint8_t* curr = data;
	uint8_t* end  = data + data_len;

	// 1. parse input string
	do {
		uint8_t tag = *curr++;
		uint8_t len = *curr++;

		if (tag != 0x62) {
			result = SW_INCORRECT_PARAMS_IN_CDATA;
			break;
		}
		
		while (curr < end) {
			tag = *curr++;
			len = *curr++;
			switch (tag) {
				case 0x80: { // File size
					assert_eq(len, 2, SW_INCORRECT_PARAMS_IN_CDATA);
					inode->size = hlp_get_short(curr);
					advance(curr);
				} break;
				case 0x82: { // file descriptor (e.g. DF, EF, LF, etc.)
					assert_leq(len, 5, SW_INCORRECT_PARAMS_IN_CDATA);
					memcpy(&inode->desc, curr, len);
					inode->desc[5] = len;
					advance(curr);
				} break;
				case 0x83: { // file ID (e.g. 3F00, 0101, etc.)
					assert_eq(len, 2, SW_INCORRECT_PARAMS_IN_CDATA);
					inode->fid = hlp_get_short(curr);
					advance(curr);
				} break;
				case 0x84: { // application AID (for DFs only)
					assert_leq(len, 16, SW_INCORRECT_PARAMS_IN_CDATA);
					memcpy(&inode->aid, curr, len);
					advance(curr);
				} break;
				case 0x88: { // short file ID
					assert_eq(len, 1, SW_INCORRECT_PARAMS_IN_CDATA);
					inode->sfi = *curr;
					advance(curr);
				} break;
				case 0x8A: { // Life cycle stage
					assert_eq(len, 1, SW_INCORRECT_PARAMS_IN_CDATA);
					inode->lcs = *curr;
					advance(curr);
				} break;
				case 0x8C: {  // security attributes in compact format
					assert_leq(len, 7, SW_INCORRECT_PARAMS_IN_CDATA);
					memcpy(&inode->compact, curr, len);
					advance(curr);
				} break;
				case 0x8D: { // the FID of associated securiy environment
					assert_eq(len, 2, SW_INCORRECT_PARAMS_IN_CDATA);
					inode->se = hlp_get_short(curr);
					advance(curr);
				} break;
				default: {
					result = SW_INCORRECT_PARAMS_IN_CDATA;
				} break;
			}

			if (result != SW_OK)
				break;
		}

	} while (0);

	return result;
}


static ISO_SW
data_block_for_df(ValidityArea* va, INode* new_df_node)
{
	ISO_SW result = SW_OK;

	do {
		// Assigning 3F00 to some another file is forbidden.
		if ((va->spr_blk.inodes_count != 0x00) && (new_df_node->fid == FID_MASTER_FILE)) {
			result = SW_DF_NAME_ALREADY_EXISTS;
			break;
		}

		// The size of DF is defined at compilation stage
		new_df_node->size = sizeof(FolderData);
		// allocate a data block for the newly created DF
		if ((new_df_node->data = mm_allocate(new_df_node->size)) == 0x00) {
			result = SW_MEMORY_FULL;
			break;
		}

		DBG_SET_AVAIL_MEMORY(new_df_node->size)

		// MIND THE SEQUENCE! firstable update the parent, and only after current.
		hlp_va_set_parent_df(va, va->curr_dir.fid, va->curr_dir.iNode); // Current dir becomes 'parent'
		hlp_va_set_current_df(va, new_df_node->fid, va->spr_blk.inodes_count);  // New one becomes 'current'
		
		// In each newly created dir, the first two records goes for itself and its parent
		uint32_t count = 0;

		
		if ((result = hlp_write_data((uint32_t)&df_children_list(new_df_node)[count++], (uint8_t*)&va->curr_dir, sizeof(FileID))) != SW_OK) {
			break;
		}
		if ((result = hlp_write_data((uint32_t)&df_children_list(new_df_node)[count++], (uint8_t*)&va->parent_dir,  sizeof(FileID))) != SW_OK) {
			break;
		}

		result = hlp_write_data(df_children_count(new_df_node), (uint8_t*)&count,  sizeof(uint32_t));

	} while (0);

	return result;
}

static ISO_SW
data_block_for_ef(ValidityArea* va, INode* new_ef_node)
{
	ISO_SW result = SW_OK;
	do {
		// allocate a data block for the newly created DF
		if ((new_ef_node->data = mm_allocate(new_ef_node->size)) == 0x00) {
			result = SW_MEMORY_FULL;
			break;
		}
		
		DBG_SET_AVAIL_MEMORY(new_ef_node->size)

		hlp_va_set_current_ef(va, new_ef_node->fid, va->spr_blk.inodes_count);
	} while (0);

	return result;
}


/** Adds to the FolderData::entries array an entry about newly created file */
static ISO_SW
add_record_to_parent(ValidityArea* va, INode* current)
{
	ISO_SW result = SW_OK;

	// choose the parent depending on file type to be created:
	// If we're about to create an EF, then we should start updating the 'size' field from current folder.
	// Otherwise we need to proceed from parent folder.
	uint16_t idx       = (current->desc[0] == ft_DF) ? va->parent_dir.iNode : va->curr_dir.iNode;
	INode* inode_array = (INode*)va->spr_blk.inodes_start;
	INode* currentDf   = (INode*)&inode_array[idx];
	
	do {
		uint32_t count = 0;
		
		// get the 'count' of parent folder. It will be used as an index into 'entries' array
		if ((result = hlp_read_data(df_children_count(currentDf), (uint8_t*)&count,  sizeof(uint32_t))) != SW_OK) {
			break;
		}
		
		FileID newRecord = {.iNode = va->spr_blk.inodes_count, .fid = current->fid};
		// write the record about a new child of current folder.
		if ((result = hlp_write_data((uint32_t)&df_children_list(currentDf)[count++], (uint8_t*)&newRecord, sizeof(FileID))) != SW_OK) {
			break;
		}
		
		// update the 'count' of parent folder.
		if ((result = hlp_write_data(df_children_count(currentDf), (uint8_t*)&count,  sizeof(uint32_t))) != SW_OK) {
			break;
		}

	} while (0);

	return result;
}

/** traverse the path from this DF right to the MF,
 * increasing the inode->size value of each parent folder. */
static ISO_SW
update_parent_size(ValidityArea* va, INode* current)
{
	ISO_SW result = SW_OK;
	INode* inode_array = (INode*)va->spr_blk.inodes_start;
	uint16_t idx = va->parent_dir.iNode;
	uint16_t fid = va->parent_dir.fid;

	do {
		INode* parent = (INode*)&inode_array[idx];
		if (parent->desc[0] != ft_DF) {
			break;
		}
		// add the size of newly create folder to the size of subsequent parent folder.
		uint16_t updated_size = parent->size + current->size;
		if ((result = hlp_write_data((uint32_t)&inode_array[idx].size, (uint8_t*)&updated_size, sizeof(uint16_t))) != SW_OK) {
			break;
		}

		// get the inode index of subsequent parent.
		idx = df_children_list(parent)[1].iNode;
		fid = df_children_list(parent)[1].fid;

	} while (fid != 0x00); // fid=FFFF means we've reached the end of file hierarchy

	return result;
}


ISO_SW
hlp_allocate_data_block(ValidityArea* va, INode* inode)
{
	ISO_SW result = SW_OK;
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

		if (result != SW_OK)
			break;
		
		// If this is the MF, then there is no parent dir at all. Bail out.
		if (inode->fid == FID_MASTER_FILE) {
			break;
		}
		// update FileID array of its parent
		if ((result = add_record_to_parent(va, inode)) != SW_OK) {
			break;
		}
		// and update the 'size' field of its parent dirs.
		if ((result = update_parent_size(va, inode)) != SW_OK) {
			break;
		}
		result = SW_OK;
	} while (0);

	return result;
}

ISO_SW
hlp_store_inode(ValidityArea* va, INode* inode)
{
	ISO_SW result = mm_Err;

	do {
		INode* inode_array = (INode*)va->spr_blk.inodes_start;
		if ((result = hlp_write_data((uint32_t)&inode_array[va->spr_blk.inodes_count], (uint8_t*)inode, sizeof(INode))) != SW_OK) {
			break;
		}

		// update the number of Inodes in INode table
		va->spr_blk.inodes_count++;

		// store the updated state of SuperBlock
		if ((result = hlp_write_data(va->spr_blk_addr, (uint8_t*)&va->spr_blk, sizeof(SuperBlock))) != SW_OK) {
			break;
		}

	} while (0);

	return result;
}