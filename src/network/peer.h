#ifndef PEER_H
#define PEER_H

#include <stdint.h>

int start_listener(uint16_t port);
int connect_to_peer(const char *ip, uint16_t port);
int accept_peer(int server_fd);

#endif