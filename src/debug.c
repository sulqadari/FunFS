#include "debug.h"

static void
print_array(uint8_t* data, uint16_t len, const char* name)
{
	printf("%s: ", name);
	for (uint16_t i = 0; i < len; ++i) {
		printf("%02X ", data[i]);
	}
	printf("\n");
}

void
dbg_print_inode(INode* node)
{
	printf(
		"INode\n"
		"size:     %d\n"
		"desc:     %02X\n"
		"fid:      %02X\n"
		"sfi:      %02X\n"
		"lcs:      %02X\n"
		"se:       %02X\n",
		node->size, node->desc[0], node->fid, node->sfi,
		node->lcs, node->se
	);
	print_array(node->aid, 16,      "AID         ");
	print_array(node->compact, 7,   "compact SA  ");
	print_array(node->expanded, 20, "expanded SA ");
	printf("\n");
}

void
dbg_print_super_block(ValidityArea* va)
{
	printf(
		"Super Block\n"
		"    magic: %02X\n"
		"    inodes count: %d\n"
		"    inodes capacity: %d\n\n",
		va->spr_blk.magic,va->spr_blk.inodes_count,va->spr_blk.inodes_capacity
	);
}

void
dbg_print_va(ValidityArea* va)
{
	printf(
		"Validity Area\n"
		"parent dir\n"
		"    node: %02X\n"
		"    fid:  %02X\n"
		"current dir\n"
		"    node: %02X\n"
		"    fid:  %02X\n"
		"current file\n"
		"    node: %02X\n"
		"    fid:  %02X\n\n",
		va->parent_dir.iNode, va->parent_dir.fid,
		va->current_dir.iNode, va->current_dir.fid,
		va->current_file.iNode, va->current_file.fid
	);
}

void
dbg_print_hex(uint8_t* data, uint32_t len)
{
	for (uint32_t i = 0; i < len; ++i) {
		if ((i != 0) && ((i % 16) == 0)) {
			printf("\n");
		}
	
		printf("%02x ", data[i]);
	}
	printf("\n");
}