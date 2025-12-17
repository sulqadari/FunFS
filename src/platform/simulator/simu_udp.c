#include "simu_udp.h"
#include "flash_emu.h"
#include "debug.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>

static struct sockaddr_in server_addr = {0};
static struct sockaddr_in client_addr = {0};

static int udp_socket = 0;
static int udp_port   = 8485;

static socklen_t socket_len = sizeof(client_addr);

uint16_t
udp_server_init(void)
{
	if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Failed to retrieve a UDP socket descriptor.\n");
		exit(1);
	}

	memset((uint8_t*)&server_addr, 0x00, sizeof(struct sockaddr_in));
	memset((uint8_t*)&client_addr, 0x00, sizeof(struct sockaddr_in));

	server_addr.sin_family      = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port        = htons(udp_port);

	if (bind(udp_socket, (const struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) < 0) {
		printf("Failed to assign the server address to the upd socket.\n");
		close(udp_socket);
		exit(1);
	}


	return 0;
}

void
udp_server_close(void)
{
	close(udp_socket);
}

void
udp_receive_cdata(uint8_t* cdata)
{
	ssize_t bytes_read = recvfrom(
							udp_socket,
							cdata,
							APDU_BUFFER_LEN,
							MSG_WAITALL,
							(struct sockaddr*)&client_addr,
							&socket_len
						);
	
	if (bytes_read < 0) {
		DBG_PRINT_VARG("%s\n", "ERROR: UDP failed to retrieve a data from the client")
		raise(SIGINT);
	}

	DBG_PRINT_VARG("%s", "<< ")
	DBG_PRINT_HEX(cdata, bytes_read)
}

void
udp_send_cdata(uint8_t* rdata, uint16_t len)
{
	sendto(
		udp_socket,
		rdata,
		len,
		MSG_CONFIRM,
		(struct sockaddr*)&client_addr,
		socket_len
	);

	DBG_PRINT_VARG("%s", ">> ")
	DBG_PRINT_HEX(rdata, len)
}