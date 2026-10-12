#ifndef P2P_H
#define P2P_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include "../recovery/fairness.h"
#include "../network/discovery.h"
#include "../storage/availability.h"
#include "../common/types.h"

typedef struct
{
    uint32_t peer_id;
    uint16_t port;
    int listener_fd;

    pthread_t listener_thread;
    int running;

    PeerTable peers;
    pthread_mutex_t peers_mutex;
    AvailabilityTable availability;
    pthread_mutex_t availability_mutex;

    FairnessTable fairness;
    pthread_mutex_t fairness_mutex;
    pthread_mutex_t upload_mutex;
    pthread_cond_t upload_cond;
    size_t active_uploads;

    PieceInfo *pieces;
    size_t piece_count;
    PieceInfo *remote_pieces;
    size_t remote_piece_count;
    uint32_t remote_peer_id;

    char pieces_dir[256];
    char shared_file_path[512];

} P2PNode;

int p2p_node_init(
    P2PNode *node,
    uint32_t peer_id,
    uint16_t port);

void p2p_node_free(
    P2PNode *node);

int p2p_send_hello(
    int socket_fd,
    uint32_t peer_id,
    uint16_t port);

int p2p_connect_to_peer(
    P2PNode *node,
    const char *ip,
    uint16_t port);

int p2p_receive_hello(
    int socket_fd,
    uint32_t *peer_id,
    uint16_t *port);

int p2p_start_listener(
    P2PNode *node);

void p2p_stop_listener(
    P2PNode *node);

int p2p_exchange_peer_list(
    P2PNode *node,
    int socket_fd,
    uint32_t remote_peer_id);
int p2p_announce_manifest(P2PNode *node);

#endif