#ifndef FUNFS_ISO7816_HELPER_H
#define FUNFS_ISO7816_HELPER_H

#include "iso7816.h"
#include "flash_emu.h"

/** pointer to the beginning of DF_Payload field. */
#define df_children_count(expr) (uint32_t)&((DF_Payload*)expr->data)->children_count
/** pointer to the DF_Payload::children_list array */
#define df_children_list(expr) ((DF_Payload*)expr->data)->children_list

void hlp_va_set_parent_df(ValidityArea* va, uint16_t fid, uint16_t node);
void hlp_va_set_current_df(ValidityArea* va, uint16_t fid, uint16_t node);
void hlp_va_set_current_ef(ValidityArea* va, uint16_t fid, uint16_t node);

ISO_SW hlp_parse_params(INode* inode, uint8_t* data, uint32_t data_len);
ISO_SW hlp_allocate_data_block(ValidityArea* va, INode* inode);
ISO_SW hlp_store_inode(ValidityArea* va, INode* inode);

ISO_SW hlp_read_data (uint32_t offset, uint8_t* data, uint32_t len);
ISO_SW hlp_write_data(uint32_t offset, uint8_t* data, uint32_t len);

uint16_t hlp_get_short(uint8_t* buff);

#endif /* FUNFS_ISO7816_HELPER_H */