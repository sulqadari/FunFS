#include "iso7816_apdu.h"
#include "iso7816_cmd.h"
#include "simulator/simu_udp.h"

ISO_SW
apdu_process(Apdu* apdu)
{
	ISO_SW  sw  = SW_CLA_NOT_SUPPORTED;

	do {
		if (apdu->header.cla != 0x00) {
			break;
		}
		
		switch (apdu->header.ins) {
			case APDU_INS_CREATE_FILE : {
				sw = iso_create_file(apdu);
			} break;
			case APDU_INS_SELECT      : {
				sw = iso_select(apdu);
			} break;
			case APDU_INS_ACTIVATE    : {
				sw = iso_activate(apdu);
			} break;
			case APDU_INS_READ_BINARY : {
				sw = iso_read_binary(apdu);
			} break;
			case APDU_INS_WRITE_BINARY: {
				sw = iso_write_binary(apdu);
			} break;
			default: {
				sw = SW_INS_NOT_SUPPORTED;
			} break;
		}
	} while (0);

	return sw;
}

void
apdu_receive_cdata(Apdu* apdu)
{
	memset(apdu->buffer, 0x00, APDU_BUFFER_LEN);
	udp_receive_cdata(apdu->buffer);
	apdu->resp_len = 0;
}

void
apdu_send_rdata(uint8_t* rdata, uint16_t len, ISO_SW sw)
{
	rdata[len]     = sw >> 8;
	rdata[len + 1] = sw & 0xFF;
	len += 2;

	udp_send_cdata(rdata, len);
}