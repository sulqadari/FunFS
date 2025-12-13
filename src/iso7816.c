#include "iso7816.h"
#include "flash_emu.h"
#include "debug.h"
#include "iso7816_helpers.h"
#include <string.h>

static ValidityArea va;

ISO_SW
iso_initialize(void)
{
	DBG_PRINT_VARG("\ncall: %s\n\n", "iso_initialize")
	DBG_PRINT_VARG(
		"VA                     size: %d\n"
		"SuperBlock             size: %d\n"
		"Max number of entries in DF: %d\n"
		"Inode                  size: %d\n",
		sizeof(ValidityArea),
		sizeof(SuperBlock),
		sizeof(DF_Payload) / sizeof(DF_Record) - 1,
		sizeof(INode)
	)
	
	ISO_SW result = SW_MEMORY_FAILURE;
	do {
		memset((uint8_t*)&va, 0x00, sizeof(ValidityArea));

		va.spr_blk_addr = mm_get_start_address();

		// Open (or create) persistent storage for a flash memory
		if (mm_open_image() != mm_Ok) {
			break;
		}
		// Read the 'Super Block' and find out has the file system been initialized previously or not
		if (hlp_read_data(va.spr_blk_addr, (uint8_t*)&va.spr_blk, sizeof(SuperBlock)) != SW_OK) {
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

		DBG_SET_AVAIL_MEMORY(sizeof(SuperBlock))

		uint32_t inode_table_size = INODE_TABLE_SIZE;	// allocate 10% of available memory for the Inodes table.
		if ((va.spr_blk.inodes_start = mm_allocate(inode_table_size)) == 0) {
			break;
		}

		DBG_SET_AVAIL_MEMORY(inode_table_size)

		va.spr_blk.magic           = 0xCAFEBABE;
		va.spr_blk.inodes_count    = 0x00;
		va.spr_blk.inodes_capacity = inode_table_size / sizeof(INode);	// 1024 / 64 = 96
		if (hlp_write_data(va.spr_blk_addr, (uint8_t*)&va.spr_blk, sizeof(SuperBlock)) != SW_OK) { // store the state of SuperBlock
			break;
		}

		result = SW_OK;
	} while (0);

	DBG_PRINT_VARG(
		"\nSuperBlock\n"
		"super block start:     %04X\n"
		"inodes_start:          %04X\n"
		"inodes table capacity: %d\n"
		"available memory:      %d\n\n",
		va.spr_blk_addr,
		va.spr_blk.inodes_start,
		va.spr_blk.inodes_capacity,
		DBG_GET_AVAIL_MEMORY()
		
	)

	return result;
}

ISO_SW
iso_create_file(uint8_t* data, uint32_t data_len)
{
	DBG_PRINT_VARG("\ncall: %s", "iso_create_file")

	ISO_SW result = SW_MEMORY_FULL;
	INode inode;

	do {
		// we cant't create more than 'inodes_capacity' files.
		if (va.spr_blk.inodes_count >= va.spr_blk.inodes_capacity) {
			break;
		}

		memset((uint8_t*)&inode, 0x00, sizeof(INode));
		if ((result = hlp_parse_params(&inode, data, data_len)) != SW_OK) {
			break;
		}
		
		DBG_PRINT_VARG("(%04X)\n", inode.fid)

		if ((result = hlp_allocate_data_block(&va, &inode)) != SW_OK) {
			break;
		}
		if ((result = hlp_store_inode(&va, &inode)) != SW_OK) {
			break;
		}

		DBG_PRINT_SUPERBLOCK(&va)
		DBG_PRINT_INODE(&inode)
		result = SW_OK;
	} while (0);

	
	DBG_PRINT_VARG("\navailable memory: %d\n", DBG_GET_AVAIL_MEMORY());
	
	return result;
}

ISO_SW
iso_select_by_name(const uint16_t fid)
{
	DBG_PRINT_VARG("\ncall: %s(%04X)\n\n", "iso_select_by_name", fid)
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
		if (hlp_read_data(df_children_count(currentDf), (uint8_t*)&count,  sizeof(uint32_t)) != SW_OK) {
			break;
		}

		DF_Record next;
		uint32_t i;

		for (i = 0; i < count; ++i) {
			// read from current DF's payload region an info about subsequent child. 
			if (hlp_read_data((uint32_t)&df_children_list(currentDf)[i], (uint8_t*)&next, sizeof(DF_Record)) != SW_OK) {
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
				uint16_t parent_idx = df_children_list(currentDf)[1].iNode;
				uint16_t parent_fid = df_children_list(currentDf)[1].fid;

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

	DBG_PRINT_VA(&va);

	return result;
}

ISO_SW
iso_select_by_path(uint8_t* data, uint32_t data_len)
{
	DBG_PRINT_VARG("call: %s\n", "iso_select_by_path")

	ISO_SW result = SW_FILE_NOT_FOUND;
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