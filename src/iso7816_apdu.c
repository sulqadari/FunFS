#include "iso7816_apdu.h"
#include "iso7816_cmd.h"

ISO_SW
apdu_process(Apdu* apdu)
{
	uint8_t ins = 0x00;
	ISO_SW  sw  = SW_CLA_NOT_SUPPORTED;

	do {
		if (apdu->header.cla != 0x00) {
			break;
		}
		
		switch (ins) {
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
	
}

void
apdu_send_rdata(Apdu* apdu, ISO_SW sw)
{
	
}