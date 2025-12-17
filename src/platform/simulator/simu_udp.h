#ifndef FUNFS_SIMU_UDP_H
#define FUNFS_SIMU_UDP_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "iso7816_apdu.h"

uint16_t udp_server_init(void);
void     udp_server_close(void);

void udp_receive_cdata(Apdu* apdu);
void udp_send_cdata(Apdu* apdu);

#endif /* FUNFS_SIMU_UDP_H */