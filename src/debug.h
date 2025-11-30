#ifndef FUNFS_DEBUG_H
#define FUNFS_DEBUG_H

#include "iso7816.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

void dbg_print_inode(INode* node);
void dbg_print_va(ValidityArea* va);
void dbg_print_super_block(ValidityArea* va);
void dbg_print_cmd_name(const char* name);

#endif /* FUNFS_DEBUG_H */