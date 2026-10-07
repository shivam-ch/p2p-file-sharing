#ifndef P2P_H
#define P2P_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#include "../network/discovery.h"
#include "../storage/availability.h"
#include "../common/types.h"

typedef struct {
    uint32_t peer_id;
    uint16_t port;
    int listener_fd;

    pthread_t listener_thread;
    int running;

    PeerTable peers;
    AvailabilityTable availability;

    PieceInfo *pieces;
    size_t piece_count;

    char pieces_dir[256];

} P2PNode;

int p2p_node_init(
    P2PNode *node,
    uint32_t peer_id,
    uint16_t port
);

void p2p_node_free(
    P2PNode *node
);

int p2p_send_hello(
    int socket_fd,
    uint32_t peer_id,
    uint16_t port
);

int p2p_connect_to_peer(
    P2PNode *node,
    const char *ip,
    uint16_t port
);

int p2p_receive_hello(
    int socket_fd,
    uint32_t *peer_id,
    uint16_t *port
);

int p2p_start_listener(
    P2PNode *node
);

void p2p_stop_listener(
    P2PNode *node
);

#endif