#include "simu_udp.h"
#include "debug.h"

#include <stdio.h>
#include <string.h>

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
udp_receive_cdata(Apdu* apdu)
{
	memset(apdu->buffer, 0x00, APDU_BUFFER_LEN);
	ssize_t bytes_read = recvfrom(
							udp_socket,
							apdu->buffer,
							APDU_BUFFER_LEN,
							MSG_WAITALL,
							(struct sockaddr*)&client_addr,
							&socket_len
						);
	
	if (bytes_read < 0) {

	}

	apdu->header.len = (uint16_t)bytes_read;
	
	DBG_PRINT_HEX(apdu->buffer, apdu->header.len)
}

void
udp_send_cdata(Apdu* apdu)
{
	sendto(
		udp_socket,
		apdu->buffer,
		apdu->header.len,
		MSG_CONFIRM,
		(struct sockaddr*)&client_addr,
		socket_len
	);
}