#include "iso7816.h"
#include "flash_emu.h"
#include "debug.h"
#include "iso7816_helpers.h"
#include <string.h>

static ValidityArea va;

ISO_SW
iso_initialize(void)
{
	dbg_print_cmd_name("INITIALIZE");
	ISO_SW result = SW_MEMORY_FAILURE;
	
	dbg_print_value(sizeof(ValidityArea), "ValidityArea");
	dbg_print_value(sizeof(SuperBlock),   "SuperBlock  ");
	dbg_print_value(sizeof(DF_Record),    "DF_Record   ");
	dbg_print_value(sizeof(DF_Payload),   "DF_Payload  ");
	dbg_print_value(sizeof(INode),        "INode       ");
	
	do {
		memset((uint8_t*)&va, 0x00, sizeof(ValidityArea));

		va.spr_blk_addr = mm_get_start_address();

		// Open (or create) persistent storage for a flash memory
		if (mm_open_image() != mm_Ok) {
			break;
		}
		// Read the 'Super Block' and find out has the file system been initialized previously or not
		if (read_data(va.spr_blk_addr, (uint8_t*)&va.spr_blk, sizeof(SuperBlock)) != mm_Ok) {
			break;
		}

		// IF: the file system is already initialized, then just set the MF as the current folder and bail out.
		if (va.spr_blk.magic == 0xCAFEBABE) {
			hlp_va_set_current_df(&va, FID_MASTER_FILE, 0x00); // Set the MF as the current dir
			hlp_va_set_parent_df( &va, 0x00,0x00); // The master file hasn't parent dir.
			hlp_va_set_current_ef(&va, FID_NONE, FID_NONE); // At startup there is no
			result = SW_OK;
			break;
		}

		// OTHERWISE: this is the very first run: no MF, no file system, no nothing.
		if ((va.spr_blk_addr = mm_allocate(sizeof(SuperBlock))) == 0) { // allocate space for the SuperBlock
			break;
		}

		uint32_t size = PAGE_SIZE * 1;	// allocate space for the Inodes table.
		if ((va.spr_blk.inodes_start = mm_allocate(size)) == 0) {
			break;
		}

		va.spr_blk.magic           = 0xCAFEBABE;
		va.spr_blk.inodes_count    = 0x00;
		va.spr_blk.inodes_capacity = size / sizeof(INode);	// 1024 / 64 = 96
		if (write_data(va.spr_blk_addr, (uint8_t*)&va.spr_blk, sizeof(SuperBlock)) != mm_Ok) { // store the state of SuperBlock
			break;
		}

		result = SW_OK;
	} while (0);

	return result;
}

ISO_SW
iso_create_file(uint8_t* data, uint32_t data_len)
{
	dbg_print_cmd_name("CREATE FILE");

	ISO_SW result = SW_UNKNOWN;
	INode inode;

	do {
		// we cant't create more than 'inodes_capacity' files.
		if (va.spr_blk.inodes_count >= va.spr_blk.inodes_capacity) {
			break;
		}

		memset((uint8_t*)&inode, 0x00, sizeof(INode));
		if (hlp_parse_params(&inode, data, data_len) != mm_Ok) {
			break;
		}
		if (hlp_allocate_data_block(&va, &inode) != mm_Ok) {
			break;
		}
		if (hlp_store_inode(&va, &inode) != mm_Ok) {
			break;
		}

		result = SW_OK;
	} while (0);

	dbg_print_super_block(&va);
	dbg_print_inode(&inode);
	
	return result;
}

ISO_SW
iso_select_by_name(const uint16_t fid)
{
	dbg_print_cmd_name("SELECT BY NAME");
	ISO_SW result = SW_UNKNOWN;

	uint16_t idx       = va.current_dir.iNode;
	INode* inode_array = (INode*)va.spr_blk.inodes_start;
	INode* currentDf   = (INode*)&inode_array[idx];

	do {
		hlp_va_set_current_ef(&va, FID_NONE, FID_NONE);
		// special case: MF always present and is always accessible
		if (fid == FID_MASTER_FILE) {
			result = SW_OK;
			hlp_va_set_current_df(&va, FID_MASTER_FILE, 0x00);
			hlp_va_set_parent_df(&va, 0x00, 0x00);
			break;
		}

		uint32_t count = 0;
		// get the 'count' of files in current folder.
		if (read_data(df_data(currentDf)->count, (uint8_t*)&count,  sizeof(uint32_t)) != mm_Ok) {
			break;
		}

		DF_Record next;
		uint32_t i;

		for (i = 0; i < count; ++i) {
			// read from current DF's payload region an info about subsequent child. 
			if (read_data((uint32_t)&df_entries(currentDf)[i], (uint8_t*)&next, sizeof(DF_Record)) != mm_Ok) {
				break;
			}

			if (fid == next.fid) {
				currentDf = (INode*)&inode_array[next.iNode];

				// If the file we have found isn't of type DF, then just update the 'current file' field
				// of VA and return.
				if (currentDf->desc[0] != ft_DF) {
					hlp_va_set_current_ef(&va, next.fid, next.iNode);
					break;
				}

				// Otherwise, update the 'parent DF/child DF' values.
				uint16_t parent_idx = df_entries(currentDf)[1].iNode;
				uint16_t parent_fid = df_entries(currentDf)[1].fid;

				hlp_va_set_parent_df(&va, parent_fid, parent_idx); // Current dir becomes 'parent'
				hlp_va_set_current_df(&va, next.fid, next.iNode);  // the one we're looking for becomes 'current'
				break;
			}
		}

		if (i >= count) {
			result = SW_FILE_NOT_FOUND;
			break;
		}
		result = SW_OK;
	} while (0);

	dbg_print_va(&va);

	return result;
}

ISO_SW
iso_select_by_path(uint8_t* data, uint32_t data_len)
{
	dbg_print_cmd_name("SELECT BY PATH");
	ISO_SW result = SW_UNKNOWN;
	uint8_t* ptr = data;
	uint16_t fid;

	do {
		fid = hlp_get_short(ptr);
		if ((result = iso_select_by_name(fid)) != SW_OK)
			break; 

		ptr += 2;
	} while (data_len -= 2);
	
	return result;
}