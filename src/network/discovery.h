#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <stddef.h>
#include <stdint.h>

#include "../common/types.h"

typedef struct {
    PeerInfo *peers;
    size_t count;
    size_t capacity;
} PeerTable;

void peer_table_init(PeerTable *table);
void peer_table_free(PeerTable *table);

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