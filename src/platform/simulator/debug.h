#ifndef FUNFS_DEBUG_H
#define FUNFS_DEBUG_H

#include "iso7816_cmd.h"
#include <string.h>
#include <stdio.h>
#include "flash_emu.h"
#include <stdint.h>

void dbg_print_inode(INode* node);
void dbg_print_va(ValidityArea* va);
void dbg_print_super_block(ValidityArea* va);
void dbg_print_hex(uint8_t* data, uint32_t len);
char* dbg_sw_to_string(ISO_SW sw);

#if defined (FUNFS_DBG_MODE)
#define DBG_PRINT_VARG(format, ...) \
	printf(format, __VA_ARGS__);

#define DBG_PRINT_INODE(inode)      \
	dbg_print_inode(inode);

#define DBG_PRINT_VA(va)            \
	dbg_print_va(va);

#define DBG_PRINT_SUPERBLOCK(super) \
	dbg_print_super_block(super);

#define DBG_PRINT_VALUE(value, len) \
	dbg_print_value(value, len);

#define DBG_PRINT_HEX(array, length) \
	dbg_print_hex(array, length);

#define DBG_SW_TO_STRING(sw) \
	dbg_sw_to_string(sw)

#define DBG_GET_AVAIL_MEMORY() \
	get_available_memory()            // Notice the absence of semicolon

# else
#define DBG_PRINT_VARG(format, ...)
#define DBG_PRINT_INODE(inode)
#define DBG_PRINT_VA(va)
#define DBG_PRINT_SUPERBLOCK(super)
#define DBG_PRINT_VALUE(value, len)
#define DBG_PRINT_HEX(array, length)
#define DBG_SW_TO_STRING(sw)
#define DBG_GET_AVAIL_MEMORY()
#endif /* FUNFS_DBG_MODE */

#endif /* FUNFS_DEBUG_H */