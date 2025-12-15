#ifndef FUNFS_ISO7816_CMD_H
#define FUNFS_ISO7816_CMD_H

#include <stdint.h>
#include "iso7816.h"
#include "iso7816_apdu.h"

ISO_SW iso_initialize(void);

ISO_SW iso_create_file   (Apdu* apdu);
ISO_SW iso_select_by_path(Apdu* apdu);
ISO_SW iso_select_by_name(const uint16_t fid);

ISO_SW iso_activate    (Apdu* apdu);
ISO_SW iso_read_binary (Apdu* apdu);
ISO_SW iso_write_binary(Apdu* apdu);

#endif /* FUNFS_ISO7816_CMD_H */