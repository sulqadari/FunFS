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
		"size:     %d\n"
		"desc:     %02X\n"
		"fid:      %02X\n"
		"sfi:      %02X\n"
		"lcs:      %02X\n"
		"se:       %02X\n",
		node->size, node->desc[0], node->fid, node->sfi,
		node->lcs, node->se
	);
	print_array(node->aid, 16, "AID");
	print_array(node->compact, 7, "compact SA");
	print_array(node->expanded, 20, "expanded SA");
	printf("\n");
}

void
dgb_print_va(ValidityArea* va)
{

}