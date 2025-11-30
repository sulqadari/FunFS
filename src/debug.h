#ifndef FUNFS_DEBUG_H
#define FUNFS_DEBUG_H

#include "iso7816.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

void dbg_print_inode(INode* node);
void dgb_print_va(ValidityArea* va);

#endif /* FUNFS_DEBUG_H */