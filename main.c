#include "simulator/simu_udp.h"
#include "iso7816_cmd.h"
#include "simulator/debug.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

static void
interrupt_handler(int num)
{
	udp_server_close();
	mm_save_image();
	DBG_PRINT_VARG("\n%s\n", "Done.");
	exit(0);
}

int
main(int argc, char* argv[])
{
	ISO_SW sw = SW_UNKNOWN;
	Apdu apdu;

	udp_server_init();
	if (signal(SIGINT, interrupt_handler) == SIG_ERR) {
		DBG_PRINT_VARG("%s", "ERROR: can't catch SIGINT\n");
	}

	if ((sw = iso_initialize()) != SW_OK) {
		DBG_PRINT_VARG("ERROR: 'iso_initialize()' function failed with SW %04X (%s)\n", sw, DBG_SW_TO_STRING(sw));
		exit(1);
	}

	while (1) {
		// accept next incoming APDU
		apdu_receive_cdata(&apdu);

		// process incoming command
		sw = apdu_process(&apdu);

		// return data (if any) and SW
		apdu_send_rdata(apdu.buffer, apdu.resp_len, sw);
	}

	while (1) { volatile int i = 0; (void)i++; }
}