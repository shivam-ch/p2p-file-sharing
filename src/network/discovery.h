#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <stddef.h>
#include <stdint.h>

#include "../common/types.h"

#define MAX_PEERS 10

typedef struct {
    PeerInfo peers[MAX_PEERS];
    size_t count;
} PeerTable;

void peer_table_init(PeerTable *table);

int peer_table_add(
    PeerTable *table,
    PeerInfo peer
);

int peer_table_remove(
    PeerTable *table,
    uint32_t peer_id
);

PeerInfo *peer_table_find(
    PeerTable *table,
    uint32_t peer_id
);

int serialize_peer_list(
    const PeerTable *table,
    unsigned char *buffer,
    uint32_t buffer_size,
    uint32_t *output_size
);

int deserialize_peer_list(
    PeerTable *table,
    const unsigned char *buffer,
    uint32_t buffer_size
);

int send_peer_list(
    int socket_fd,
    const PeerTable *table
);

int receive_peer_list(
    int socket_fd,
    PeerTable *table
);

#endif