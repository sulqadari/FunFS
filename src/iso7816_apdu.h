#ifndef FUNFS_APDU_H
#define FUNFS_APDU_H

#include <stdint.h>

#define APDU_OFFSET_CDATA 5

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
		uint8_t buffer[263];
	};
} Apdu;

void apdu_receive_cmd (Apdu* apdu);
void apdu_receive_data(Apdu* apdu);
void apdu_send_data   (Apdu* apdu);
void apdu_send_status (Apdu* apdu);

#endif /* FUNFS_APDU_H */