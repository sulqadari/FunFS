#include "debug.h"
#include "iso7816_cmd.h"

typedef struct {
	char* name;
	ISO_SW value;
} dbg_sw;

dbg_sw sw_list [] = {
	{.name = "SW_SELECTED_FILE_DEACTIVATED",         .value = SW_SELECTED_FILE_DEACTIVATED },
	{.name = "SW_MEMORY_FAILURE",                    .value = SW_MEMORY_FAILURE },
	{.name = "SW_MEMORY_FULL",                       .value = SW_MEMORY_FULL },
	{.name = "SW_CMD_INCOMPATIBLE_WITH_FILE_STRUCT", .value = SW_CMD_INCOMPATIBLE_WITH_FILE_STRUCT },
	{.name = "SW_SECURITY_STATUS_NOT_SATISFIED",     .value = SW_SECURITY_STATUS_NOT_SATISFIED },
	{.name = "SW_DATA_NOT_USABLE",                   .value = SW_DATA_NOT_USABLE },
	{.name = "SW_INCORRECT_PARAMS_IN_CDATA",         .value = SW_INCORRECT_PARAMS_IN_CDATA },
	{.name = "SW_FUNCTION_NOT_SUPPORTED",            .value = SW_FUNCTION_NOT_SUPPORTED },
	{.name = "SW_FILE_NOT_FOUND",                    .value = SW_FILE_NOT_FOUND },
	{.name = "SW_NOT_ENOUGH_MEMORY_IN_FILE",         .value = SW_NOT_ENOUGH_MEMORY_IN_FILE },
	{.name = "SW_INCORRECT_P1P2",                    .value = SW_INCORRECT_P1P2 },
	{.name = "SW_NC_INCONSISTENT_WITH_P1P2",         .value = SW_NC_INCONSISTENT_WITH_P1P2 },
	{.name = "SW_DATA_NOT_FOUND",                    .value = SW_DATA_NOT_FOUND },
	{.name = "SW_FILE_ALREADY_EXISTS",               .value = SW_FILE_ALREADY_EXISTS },
	{.name = "SW_DF_NAME_ALREADY_EXISTS",            .value = SW_DF_NAME_ALREADY_EXISTS },
	{.name = "SW_INS_NOT_SUPPORTED",                 .value = SW_INS_NOT_SUPPORTED },
	{.name = "SW_CLA_NOT_SUPPORTED",                 .value = SW_CLA_NOT_SUPPORTED },
	{.name = "SW_UNKNOWN",                           .value = SW_UNKNOWN },
	{.name = "SW_OK",                                .value = SW_OK }
};

char*
dbg_sw_to_string(ISO_SW sw)
{
	uint32_t i = 0, len = sizeof(sw_list) / sizeof(sw_list[0]);

	for (i = 0; i < len; ++i) {
		if (sw_list[i].value == sw) {
			return sw_list[i].name;
		}
	}
	return "Unknown SW";
}

static void
print_array(uint8_t* data, uint16_t len, const char* name)
{
	printf("%s: ", name);
	for (uint16_t i = 0; i < len; ++i) {
		printf("%02X ", data[i]);
	}
	printf("\n");
}

static void
print_sec_attr(SACompact* attr)
{
	uint8_t* temp = (uint8_t*)attr->scb_list;
	printf(
		"compact SA  \n"
		"\tproprietary      %d\n"
		"\tremovable        %d: %02x\n"
		"\tterminate        %d: %02x\n"
		"\tactivate         %d: %02x\n"
		"\tdeactivate       %d: %02x\n"
		"\tcreate_df/write  %d: %02x\n"
		"\tcreate_ef/update %d: %02x\n"
		"\tdel_child/read   %d: %02x\n",
		attr->amb.df.is_prop,
		attr->amb.df.removable, temp[0],
		attr->amb.df.terminate, temp[1],
		attr->amb.df.activate,  temp[2],
		attr->amb.df.deactivate,temp[3],
		attr->amb.df.create_df, temp[4],
		attr->amb.df.create_ef, temp[5],
		attr->amb.df.del_child, temp[6]
	);
	
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
	print_sec_attr(&node->sacf);
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
		va->curr_dir.iNode, va->curr_dir.fid,
		va->curr_file.iNode, va->curr_file.fid
	);
}

void
dbg_print_hex(uint8_t* data, uint32_t len)
{
	for (uint32_t i = 0; i < len; ++i) {
		// if ((i != 0) && ((i % 16) == 0)) {
		// 	printf("\n");
		// }
		printf("%02x ", data[i]);
	}
	printf("\n");
}