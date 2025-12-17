#ifndef FUNFS_APDU_H
#define FUNFS_APDU_H

#include <stdint.h>
#include "iso7816.h"

#define APDU_OFFSET_CDATA 5
#define APDU_BUFFER_LEN   5 + 256 + 1 + 1	// header + data + sw1 + sw2

#define APDU_INS_CREATE_FILE  0xE0
#define APDU_INS_SELECT       0xA4
#define APDU_INS_ACTIVATE     0x44
#define APDU_INS_READ_BINARY  0xB0
#define APDU_INS_WRITE_BINARY 0xD0

typedef struct {
	uint8_t cla;
	uint8_t ins;
	uint8_t p1;
	uint8_t p2;
	uint8_t len;
} Header;

typedef struct {
	union
	{
		Header header;
		uint8_t buffer[APDU_BUFFER_LEN];
	};
} Apdu;

ISO_SW apdu_process    (Apdu* apdu);
void apdu_receive_cdata(Apdu* apdu);
void apdu_send_rdata   (Apdu* apdu, ISO_SW sw);

#endif /* FUNFS_APDU_H */