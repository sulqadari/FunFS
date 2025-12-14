#include "simu_udp.h"

static struct sockaddr_in server_addr = {0};
static struct sockaddr_in client_addr = {0};

static int socket_desc = 0;
static int socket_port = 8485;

static socklen_t socketLength = sizeof(client_addr);

uint16_t
udp_server_init(void)
{
	return 1;
}

uint16_t
udp_server_run(Apdu* apdu)
{
	return 1;
}

void
apdu_receive_cmd(Apdu* apdu)
{

}

void
apdu_receive_data(Apdu* apdu)
{
	
}

void
apdu_send_data(Apdu* apdu)
{
	
}

void
apdu_send_status(Apdu* apdu)
{
	
}