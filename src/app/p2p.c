#include "p2p.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "../common/protocol.h"
#include "../storage/file_splitter.h"
#include "../network/peer.h"

static void *p2p_listener_thread(void *arg)
{
    P2PNode *node = arg;

    while (node->running) {

        int peer_fd = accept_peer(node->listener_fd);

        if (peer_fd < 0) {
            if (!node->running) {
                break;
            }

            perror("accept");
            continue;
        }

        uint32_t remote_peer_id;
        uint16_t remote_port;

        if (p2p_receive_hello(
                peer_fd,
                &remote_peer_id,
                &remote_port
            ) != 0) {

            printf("Invalid HELLO received.\n");
            close(peer_fd);
            continue;
        }

        PeerInfo peer;

        peer.peer_id = remote_peer_id;
        peer.port = remote_port;

        snprintf(
            peer.ip,
            sizeof(peer.ip),
            "127.0.0.1"
        );

        if (peer_table_add(&node->peers, peer) != 0) {
            printf(
                "Failed to add Peer %u to discovery table.\n",
                remote_peer_id
            );
        } else {
            printf(
                "Peer %u connected on port %u.\n",
                remote_peer_id,
                remote_port
            );
        }

        if (p2p_send_hello(
                peer_fd,
                node->peer_id,
                node->port
            ) != 0) {

            printf(
                "Failed to send HELLO to Peer %u.\n",
                remote_peer_id
            );
        }

        close(peer_fd);
    }

    return NULL;
}

int p2p_start_listener(
    P2PNode *node
)
{
    if (node == NULL) {
        return -1;
    }

    if (node->running) {
        return -1;
    }

    node->listener_fd = start_listener(node->port);

    if (node->listener_fd < 0) {
        return -1;
    }

    node->running = 1;

    if (pthread_create(
            &node->listener_thread,
            NULL,
            p2p_listener_thread,
            node
        ) != 0) {

        close(node->listener_fd);
        node->listener_fd = -1;
        node->running = 0;

        return -1;
    }

    return 0;
}

void p2p_stop_listener(
    P2PNode *node
)
{
    if (node == NULL || !node->running) {
        return;
    }

    node->running = 0;

    shutdown(
        node->listener_fd,
        SHUT_RDWR
    );

    close(node->listener_fd);
    node->listener_fd = -1;

    pthread_join(
        node->listener_thread,
        NULL
    );
}

int p2p_node_init(
    P2PNode *node,
    uint32_t peer_id,
    uint16_t port
)
{
    if (node == NULL) {
        return -1;
    }

    memset(node, 0, sizeof(P2PNode));

    node->peer_id = peer_id;
    node->port = port;
    node->listener_fd = -1;
    node->running = 0;

    peer_table_init(&node->peers);
    availability_init(&node->availability);

    node->pieces = NULL;
    node->piece_count = 0;

    snprintf(
        node->pieces_dir,
        sizeof(node->pieces_dir),
        "pieces/peer_%u",
        peer_id
    );

    return 0;
}

void p2p_node_free(
    P2PNode *node
)
{
    if (node == NULL) {
        return;
    }

    p2p_stop_listener(node);
    peer_table_free(&node->peers);
    availability_free(&node->availability);

    free_pieces(node->pieces);

    node->pieces = NULL;
    node->piece_count = 0;
}

int p2p_send_hello(
    int socket_fd,
    uint32_t peer_id,
    uint16_t port
)
{
    unsigned char buffer[6];

    uint32_t network_peer_id = htonl(peer_id);
    uint16_t network_port = htons(port);

    memcpy(
        buffer,
        &network_peer_id,
        sizeof(network_peer_id)
    );

    memcpy(
        buffer + sizeof(network_peer_id),
        &network_port,
        sizeof(network_port)
    );

    return send_message(
        socket_fd,
        MSG_HELLO,
        buffer,
        sizeof(buffer)
    );
}

int p2p_connect_to_peer(
    P2PNode *node,
    const char *ip,
    uint16_t port
)
{
    if (node == NULL || ip == NULL) {
        return -1;
    }

    int socket_fd = connect_to_peer(ip, port);

    if (socket_fd < 0) {
        return -1;
    }

    if (p2p_send_hello(
            socket_fd,
            node->peer_id,
            node->port
        ) != 0) {

        close(socket_fd);
        return -1;
    }

    uint32_t remote_peer_id;
    uint16_t remote_port;

    if (p2p_receive_hello(
            socket_fd,
            &remote_peer_id,
            &remote_port
        ) != 0) {

        close(socket_fd);
        return -1;
    }

    PeerInfo peer;

    peer.peer_id = remote_peer_id;
    peer.port = remote_port;

    snprintf(
        peer.ip,
        sizeof(peer.ip),
        "%s",
        ip
    );

    if (peer_table_add(&node->peers, peer) != 0) {
        close(socket_fd);
        return -1;
    }

    printf(
        "Connected to Peer %u at %s:%u.\n",
        remote_peer_id,
        ip,
        remote_port
    );

    close(socket_fd);

    return 0;
}

int p2p_receive_hello(
    int socket_fd,
    uint32_t *peer_id,
    uint16_t *port
)
{
    unsigned char buffer[6];
    MessageType message_type;
    uint32_t payload_size;

    if (peer_id == NULL || port == NULL) {
        return -1;
    }

    if (receive_message(
            socket_fd,
            &message_type,
            buffer,
            sizeof(buffer),
            &payload_size
        ) != 0) {
        return -1;
    }

    if (message_type != MSG_HELLO) {
        return -1;
    }

    if (payload_size != sizeof(buffer)) {
        return -1;
    }

    uint32_t network_peer_id;
    uint16_t network_port;

    memcpy(
        &network_peer_id,
        buffer,
        sizeof(network_peer_id)
    );

    memcpy(
        &network_port,
        buffer + sizeof(network_peer_id),
        sizeof(network_port)
    );

    *peer_id = ntohl(network_peer_id);
    *port = ntohs(network_port);

    return 0;
}