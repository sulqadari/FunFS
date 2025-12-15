#ifndef FUNFS_SIMU_UDP_H
#define FUNFS_SIMU_UDP_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#include "iso7816_apdu.h"

uint16_t udp_server_init(void);

#endif /* FUNFS_SIMU_UDP_H */