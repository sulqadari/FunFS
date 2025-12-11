#ifndef FUNFS_DEBUG_H
#define FUNFS_DEBUG_H

#include "iso7816.h"
#include <string.h>
#include <stdio.h>
// #include <stdlib.h>
#include <stdint.h>

void dbg_print_inode(INode* node);
void dbg_print_va(ValidityArea* va);
void dbg_print_super_block(ValidityArea* va);
void dbg_print_hex(uint8_t* data, uint32_t len);

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

# else
#define DBG_PRINT_VARG(format, ...)
#define DBG_PRINT_INODE(inode)
#define DBG_PRINT_VA(va)
#define DBG_PRINT_SUPERBLOCK(super)
#define DBG_PRINT_VALUE(value, len)
#define DBG_PRINT_HEX(array, length)

#endif /* FUNFS_DBG_MODE */

#endif /* FUNFS_DEBUG_H */