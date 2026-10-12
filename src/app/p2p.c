#include "p2p.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <fcntl.h>
#include "../transfer/transfer.h"

#include "../common/protocol.h"
#include "../storage/file_splitter.h"
#include "../network/peer.h"

static int send_file_manifest(P2PNode *node, int socket_fd);

static int receive_file_manifest(
    P2PNode *node,
    int socket_fd,
    uint32_t remote_peer_id);
static void *p2p_upload_thread(void *arg)
{
    TrackedUploadTask *task = arg;

    if (task == NULL)
    {
        return NULL;
    }

    upload_worker_tracked(task);
    pthread_mutex_lock(task->fairness_mutex);

    int save_result = fairness_save(
        task->fairness,
        task->fairness_path);

    pthread_mutex_unlock(task->fairness_mutex);

    if (save_result != 0)
    {
        fprintf(stderr, "Warning: failed to save upload fairness history.\n");
    }

    close(task->upload.socket_fd);
    close(task->upload.file_fd);

    pthread_mutex_lock(task->upload_mutex);

    (*task->active_uploads)--;

    if (*task->active_uploads == 0)
    {
        pthread_cond_signal(task->upload_cond);
    }

    pthread_mutex_unlock(task->upload_mutex);

    free(task);

    return NULL;
}
static int receive_manifest_update(
    P2PNode *node,
    int socket_fd)
{
    unsigned char buffer[4];
    MessageType type;
    uint32_t payload_size;
    uint32_t network_peer_id;
    uint32_t remote_peer_id;

    if (node == NULL || socket_fd < 0)
    {
        return -1;
    }

    if (receive_message(
            socket_fd,
            &type,
            buffer,
            sizeof(buffer),
            &payload_size) != 0)
    {
        return -1;
    }

    if (type != MSG_MANIFEST_UPDATE ||
        payload_size != sizeof(uint32_t))
    {
        return -1;
    }

    memcpy(&network_peer_id, buffer, sizeof(network_peer_id));
    remote_peer_id = ntohl(network_peer_id);

    if (remote_peer_id == 0 ||
        remote_peer_id == node->peer_id)
    {
        return -1;
    }

    pthread_mutex_lock(&node->peers_mutex);

    PeerInfo *known_peer =
        peer_table_find(&node->peers, remote_peer_id);

    int peer_known = known_peer != NULL;

    pthread_mutex_unlock(&node->peers_mutex);

    if (!peer_known)
    {
        fprintf(stderr,
                "Manifest update from unknown peer %u rejected.\n",
                remote_peer_id);
        return -1;
    }

    if (receive_file_manifest(
            node,
            socket_fd,
            remote_peer_id) != 0)
    {
        fprintf(stderr,
                "Manifest update from peer %u failed.\n",
                remote_peer_id);
        return -1;
    }

    printf("Manifest updated from peer %u.\n", remote_peer_id);
    return 0;
}

static void *p2p_listener_thread(void *arg)
{
    P2PNode *node = arg;

    while (node->running)
    {

        int peer_fd = accept_peer(node->listener_fd);

        if (peer_fd < 0)
        {
            if (!node->running)
            {
                break;
            }

            perror("accept");
            continue;
        }
        uint32_t network_type;

        ssize_t peeked = recv(
            peer_fd,
            &network_type,
            sizeof(network_type),
            MSG_PEEK | MSG_WAITALL);

        if (peeked != (ssize_t)sizeof(network_type))
        {
            close(peer_fd);
            continue;
        }

        uint32_t message_type = ntohl(network_type);
        if (message_type == MSG_MANIFEST_UPDATE)
        {
            if (receive_manifest_update(node, peer_fd) != 0)
            {
                fprintf(stderr, "Failed to process manifest update.\n");
            }

            close(peer_fd);
            continue;
        }

        if (message_type == MSG_PIECE_REQUEST)
        {
            if (node->shared_file_path[0] == '\0')
            {
                fprintf(stderr,
                        "Upload request received, but no file is shared.\n");
                close(peer_fd);
                continue;
            }

            int file_fd = open(
                node->shared_file_path,
                O_RDONLY);

            if (file_fd < 0)
            {
                perror("open shared file");
                close(peer_fd);
                continue;
            }

            TrackedUploadTask *task = malloc(sizeof(*task));

            if (task == NULL)
            {
                perror("malloc upload task");
                close(file_fd);
                close(peer_fd);
                continue;
            }

            task->upload.socket_fd = peer_fd;
            task->upload.file_fd = file_fd;
            task->fairness = &node->fairness;
            task->fairness_mutex = &node->fairness_mutex;
            task->upload_mutex = &node->upload_mutex;
            task->upload_cond = &node->upload_cond;
            task->active_uploads = &node->active_uploads;
            snprintf(
                task->fairness_path,
                sizeof(task->fairness_path),
                "%s/fairness.csv",
                node->pieces_dir);

            pthread_t upload_thread;
            pthread_mutex_lock(&node->upload_mutex);
            node->active_uploads++;
            pthread_mutex_unlock(&node->upload_mutex);

            if (pthread_create(
                    &upload_thread,
                    NULL,
                    p2p_upload_thread,
                    task) != 0)
            {
                fprintf(stderr, "Failed to start upload thread.\n");

                pthread_mutex_lock(&node->upload_mutex);
                node->active_uploads--;

                if (node->active_uploads == 0)
                {
                    pthread_cond_signal(&node->upload_cond);
                }

                pthread_mutex_unlock(&node->upload_mutex);

                close(file_fd);
                close(peer_fd);
                free(task);
                continue;
            }

            pthread_detach(upload_thread);
            continue;
        }
        uint32_t remote_peer_id;
        uint16_t remote_port;

        if (p2p_receive_hello(
                peer_fd,
                &remote_peer_id,
                &remote_port) != 0)
        {

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
            "127.0.0.1");

        pthread_mutex_lock(&node->peers_mutex);
        if (peer.peer_id == node->peer_id)
        {
            fprintf(stderr, "Ignoring connection from this node itself.\n");
            close(peer_fd);
            continue;
        }
        int add_result = peer_table_add(&node->peers, peer);

        pthread_mutex_unlock(&node->peers_mutex);

        if (add_result != 0)
        {
            printf(
                "Failed to add Peer %u to discovery table.\n",
                remote_peer_id);
        }
        else
        {
            printf(
                "Peer %u connected on port %u.\n",
                remote_peer_id,
                remote_port);
        }

        if (p2p_send_hello(
                peer_fd,
                node->peer_id,
                node->port) != 0)
        {

            printf(
                "Failed to send HELLO to Peer %u.\n",
                remote_peer_id);

            close(peer_fd);
            continue;
        }

        if (p2p_exchange_peer_list(node, peer_fd, remote_peer_id) != 0)
        {
            fprintf(stderr, "Peer-list exchange failed.\n");
        }

        close(peer_fd);
    }
    return NULL;
}

int p2p_start_listener(
    P2PNode *node)
{
    if (node == NULL)
    {
        return -1;
    }

    if (node->running)
    {
        return -1;
    }

    node->listener_fd = start_listener(node->port);

    if (node->listener_fd < 0)
    {
        return -1;
    }

    node->running = 1;

    if (pthread_create(
            &node->listener_thread,
            NULL,
            p2p_listener_thread,
            node) != 0)
    {

        close(node->listener_fd);
        node->listener_fd = -1;
        node->running = 0;

        return -1;
    }

    return 0;
}

void p2p_stop_listener(
    P2PNode *node)
{
    if (node == NULL || !node->running)
    {
        return;
    }

    node->running = 0;

    shutdown(
        node->listener_fd,
        SHUT_RDWR);

    close(node->listener_fd);
    node->listener_fd = -1;

    pthread_join(
        node->listener_thread,
        NULL);
}

int p2p_node_init(
    P2PNode *node,
    uint32_t peer_id,
    uint16_t port)
{
    if (node == NULL)
    {
        return -1;
    }

    memset(node, 0, sizeof(P2PNode));

    node->peer_id = peer_id;
    node->port = port;
    node->listener_fd = -1;
    node->running = 0;

    peer_table_init(&node->peers);
    availability_init(&node->availability);
    fairness_init(&node->fairness);

    if (pthread_mutex_init(&node->fairness_mutex, NULL) != 0)
    {
        return -1;
    }

    if (pthread_mutex_init(&node->availability_mutex, NULL) != 0)
    {
        pthread_mutex_destroy(&node->fairness_mutex);
        return -1;
    }

    if (pthread_mutex_init(&node->peers_mutex, NULL) != 0)
    {
        pthread_mutex_destroy(&node->availability_mutex);
        pthread_mutex_destroy(&node->fairness_mutex);
        return -1;
    }
    if (pthread_mutex_init(&node->upload_mutex, NULL) != 0)
    {
        pthread_mutex_destroy(&node->peers_mutex);
        pthread_mutex_destroy(&node->availability_mutex);
        pthread_mutex_destroy(&node->fairness_mutex);
        return -1;
    }

    if (pthread_cond_init(&node->upload_cond, NULL) != 0)
    {
        pthread_mutex_destroy(&node->upload_mutex);
        pthread_mutex_destroy(&node->peers_mutex);
        pthread_mutex_destroy(&node->availability_mutex);
        pthread_mutex_destroy(&node->fairness_mutex);
        return -1;
    }

    node->active_uploads = 0;

    node->pieces = NULL;
    node->piece_count = 0;
    node->remote_pieces = NULL;
    node->remote_piece_count = 0;
    node->remote_peer_id = 0;

    snprintf(
        node->pieces_dir,
        sizeof(node->pieces_dir),
        "pieces/peer_%u",
        peer_id);
    node->shared_file_path[0] = '\0';
    return 0;
}

void p2p_node_free(
    P2PNode *node)
{
    if (node == NULL)
    {
        return;
    }

    p2p_stop_listener(node);
    pthread_mutex_lock(&node->upload_mutex);

    while (node->active_uploads > 0)
    {
        pthread_cond_wait(
            &node->upload_cond,
            &node->upload_mutex);
    }

    pthread_mutex_unlock(&node->upload_mutex);
    pthread_mutex_lock(&node->peers_mutex);
    peer_table_free(&node->peers);
    pthread_mutex_unlock(&node->peers_mutex);

    pthread_mutex_destroy(&node->peers_mutex);
    pthread_mutex_lock(&node->availability_mutex);
    availability_free(&node->availability);
    pthread_mutex_unlock(&node->availability_mutex);

    pthread_mutex_destroy(&node->availability_mutex);
    pthread_mutex_destroy(&node->fairness_mutex);

    free_pieces(node->pieces);

    node->pieces = NULL;
    node->piece_count = 0;
    free_pieces(node->remote_pieces);
    node->remote_pieces = NULL;
    node->remote_piece_count = 0;
    pthread_cond_destroy(&node->upload_cond);
    pthread_mutex_destroy(&node->upload_mutex);
}

int p2p_send_hello(
    int socket_fd,
    uint32_t peer_id,
    uint16_t port)
{
    unsigned char buffer[6];

    uint32_t network_peer_id = htonl(peer_id);
    uint16_t network_port = htons(port);

    memcpy(
        buffer,
        &network_peer_id,
        sizeof(network_peer_id));

    memcpy(
        buffer + sizeof(network_peer_id),
        &network_port,
        sizeof(network_port));

    return send_message(
        socket_fd,
        MSG_HELLO,
        buffer,
        sizeof(buffer));
}

int p2p_connect_to_peer(
    P2PNode *node,
    const char *ip,
    uint16_t port)
{
    if (node == NULL || ip == NULL)
    {
        return -1;
    }

    int socket_fd = connect_to_peer(ip, port);

    if (socket_fd < 0)
    {
        return -1;
    }

    if (p2p_send_hello(
            socket_fd,
            node->peer_id,
            node->port) != 0)
    {

        close(socket_fd);
        return -1;
    }

    uint32_t remote_peer_id;
    uint16_t remote_port;

    if (p2p_receive_hello(
            socket_fd,
            &remote_peer_id,
            &remote_port) != 0)
    {

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
        ip);
    pthread_mutex_lock(&node->peers_mutex);
    if (remote_peer_id == node->peer_id)
    {
        fprintf(stderr, "Error: cannot connect to this node itself.\n");
        close(socket_fd);
        return -1;
    }
    int add_result = peer_table_add(&node->peers, peer);

    pthread_mutex_unlock(&node->peers_mutex);

    if (add_result != 0)
    {
        close(socket_fd);
        return -1;
    }

    printf(
        "Connected to Peer %u at %s:%u.\n",
        remote_peer_id,
        ip,
        remote_port);

    if (p2p_exchange_peer_list(node, socket_fd, remote_peer_id) != 0)
    {
        fprintf(stderr, "Peer-list exchange failed.\n");
    }

    close(socket_fd);

    return 0;
}

int p2p_receive_hello(
    int socket_fd,
    uint32_t *peer_id,
    uint16_t *port)
{
    unsigned char buffer[6];
    MessageType message_type;
    uint32_t payload_size;

    if (peer_id == NULL || port == NULL)
    {
        return -1;
    }

    if (receive_message(
            socket_fd,
            &message_type,
            buffer,
            sizeof(buffer),
            &payload_size) != 0)
    {
        return -1;
    }

    if (message_type != MSG_HELLO)
    {
        return -1;
    }

    if (payload_size != sizeof(buffer))
    {
        return -1;
    }

    uint32_t network_peer_id;
    uint16_t network_port;

    memcpy(
        &network_peer_id,
        buffer,
        sizeof(network_peer_id));

    memcpy(
        &network_port,
        buffer + sizeof(network_peer_id),
        sizeof(network_port));

    *peer_id = ntohl(network_peer_id);
    *port = ntohs(network_port);

    return 0;
}

int p2p_exchange_peer_list(
    P2PNode *node,
    int socket_fd,
    uint32_t remote_peer_id)
{
    if (node == NULL || socket_fd < 0)
    {
        return -1;
    }

    pthread_mutex_lock(&node->peers_mutex);

    int send_result = send_peer_list(socket_fd, &node->peers);

    pthread_mutex_unlock(&node->peers_mutex);

    if (send_result != 0)
    {
        fprintf(stderr, "Failed to send peer list.\n");
        return -1;
    }

    pthread_mutex_lock(&node->peers_mutex);

    int receive_result = receive_peer_list(socket_fd, &node->peers);

    pthread_mutex_unlock(&node->peers_mutex);

    if (receive_result != 0)
    {
        fprintf(stderr, "Failed to receive peer list.\n");
        return -1;
    }

    if (send_file_manifest(node, socket_fd) != 0)
    {
        fprintf(stderr, "Failed to send file manifest.\n");
        return -1;
    }

    if (receive_file_manifest(
            node,
            socket_fd,
            remote_peer_id) != 0)
    {
        fprintf(stderr, "Failed to receive file manifest.\n");
        return -1;
    }

    return 0;
}
#define MANIFEST_ENTRY_SIZE 52

static uint64_t manifest_hton64(uint64_t value)
{
    uint32_t high = (uint32_t)(value >> 32);
    uint32_t low = (uint32_t)value;

    return ((uint64_t)htonl(low) << 32) | htonl(high);
}

static uint64_t manifest_ntoh64(uint64_t value)
{
    uint32_t high = (uint32_t)value;
    uint32_t low = (uint32_t)(value >> 32);

    return ((uint64_t)ntohl(high) << 32) | ntohl(low);
}

static int send_file_manifest(P2PNode *node, int socket_fd)
{
    unsigned char buffer[MAX_PAYLOAD_SIZE];
    uint32_t count;

    if (node == NULL || node->piece_count >
                            (MAX_PAYLOAD_SIZE - 4) / MANIFEST_ENTRY_SIZE)
    {
        return -1;
    }

    count = htonl((uint32_t)node->piece_count);
    memcpy(buffer, &count, 4);

    size_t position = 4;

    for (size_t i = 0; i < node->piece_count; i++)
    {
        PieceInfo *piece = &node->pieces[i];

        uint32_t id = htonl(piece->piece_id);
        uint64_t offset = manifest_hton64(piece->offset);
        uint64_t size = manifest_hton64(piece->size);

        memcpy(buffer + position, &id, 4);
        position += 4;

        memcpy(buffer + position, &offset, 8);
        position += 8;

        memcpy(buffer + position, &size, 8);
        position += 8;

        memcpy(buffer + position, piece->hash, SHA256_HASH_SIZE);
        position += SHA256_HASH_SIZE;
    }

    return send_message(
        socket_fd,
        MSG_FILE_MANIFEST,
        buffer,
        (uint32_t)position);
}
int p2p_announce_manifest(P2PNode *node)
{
    if (node == NULL)
    {
        return -1;
    }

    PeerInfo *peer_snapshot = NULL;
    size_t peer_count = 0;

    pthread_mutex_lock(&node->peers_mutex);

    peer_count = node->peers.count;

    if (peer_count > 0)
    {
        peer_snapshot = malloc(peer_count * sizeof(*peer_snapshot));

        if (peer_snapshot != NULL)
        {
            memcpy(
                peer_snapshot,
                node->peers.peers,
                peer_count * sizeof(*peer_snapshot));
        }
    }

    pthread_mutex_unlock(&node->peers_mutex);

    if (peer_count > 0 && peer_snapshot == NULL)
    {
        return -1;
    }

    int failures = 0;

    for (size_t i = 0; i < peer_count; i++)
    {
        PeerInfo *peer = &peer_snapshot[i];

        if (peer->peer_id == node->peer_id ||
            peer->port == 0 ||
            peer->ip[0] == '\0')
        {
            continue;
        }

        int socket_fd = connect_to_peer(peer->ip, peer->port);

        if (socket_fd < 0)
        {
            fprintf(
                stderr,
                "Could not notify peer %u.\n",
                peer->peer_id);
            failures++;
            continue;
        }

        uint32_t network_peer_id = htonl(node->peer_id);

        int result = send_message(
            socket_fd,
            MSG_MANIFEST_UPDATE,
            &network_peer_id,
            sizeof(network_peer_id));

        if (result == 0)
        {
            result = send_file_manifest(node, socket_fd);
        }

        close(socket_fd);

        if (result != 0)
        {
            fprintf(
                stderr,
                "Manifest update to peer %u failed.\n",
                peer->peer_id);
            failures++;
        }
        else
        {
            printf(
                "Manifest update sent to peer %u.\n",
                peer->peer_id);
        }
    }

    free(peer_snapshot);

    return failures == 0 ? 0 : -1;
}

static int receive_file_manifest(
    P2PNode *node,
    int socket_fd,
    uint32_t remote_peer_id)
{
    unsigned char buffer[MAX_PAYLOAD_SIZE];
    MessageType type;
    uint32_t payload_size;

    if (receive_message(
            socket_fd,
            &type,
            buffer,
            sizeof(buffer),
            &payload_size) != 0)
    {
        return -1;
    }

    if (type != MSG_FILE_MANIFEST || payload_size < 4)
    {
        return -1;
    }

    uint32_t count_network;
    memcpy(&count_network, buffer, 4);

    uint32_t count = ntohl(count_network);

    if (count > (MAX_PAYLOAD_SIZE - 4) / MANIFEST_ENTRY_SIZE ||
        payload_size != 4 + count * MANIFEST_ENTRY_SIZE)
    {
        return -1;
    }

    PieceInfo *pieces = NULL;

    if (count > 0)
    {
        pieces = calloc(count, sizeof(PieceInfo));
        if (pieces == NULL)
        {
            return -1;
        }
    }

    size_t position = 4;

    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t id;
        uint64_t offset;
        uint64_t size;

        memcpy(&id, buffer + position, 4);
        position += 4;

        memcpy(&offset, buffer + position, 8);
        position += 8;

        memcpy(&size, buffer + position, 8);
        position += 8;

        pieces[i].piece_id = ntohl(id);
        pieces[i].offset = manifest_ntoh64(offset);
        pieces[i].size = manifest_ntoh64(size);

        memcpy(
            pieces[i].hash,
            buffer + position,
            SHA256_HASH_SIZE);
        position += SHA256_HASH_SIZE;

        pieces[i].status = PIECE_AVAILABLE;

        pthread_mutex_lock(&node->availability_mutex);

        if (availability_find_piece(
                &node->availability,
                pieces[i].piece_id) == NULL)
        {

            if (availability_add_piece(
                    &node->availability,
                    pieces[i].piece_id) != 0)
            {
                pthread_mutex_unlock(&node->availability_mutex);
                free(pieces);
                return -1;
            }
        }

        if (availability_add_peer(
                &node->availability,
                pieces[i].piece_id,
                remote_peer_id) != 0)
        {

            pthread_mutex_unlock(&node->availability_mutex);
            free(pieces);
            return -1;
        }

        pthread_mutex_unlock(&node->availability_mutex);
    }

    pthread_mutex_lock(&node->availability_mutex);

    /* Remove this peer's old piece mappings. */
    for (size_t i = 0; i < node->availability.piece_count; i++)
    {
        PieceAvailability *available_piece = &node->availability.pieces[i];

        for (size_t j = 0; j < available_piece->peer_count;)
        {
            if (available_piece->peer_ids[j] == remote_peer_id)
            {
                if (availability_remove_peer(
                        &node->availability,
                        available_piece->piece_id,
                        remote_peer_id) != 0)
                {
                    pthread_mutex_unlock(&node->availability_mutex);
                    free_pieces(pieces);
                    return -1;
                }
            }
            else
            {
                j++;
            }
        }
    }

    /* Add the mappings from the new manifest. */
    for (uint32_t i = 0; i < count; i++)
    {
        if (availability_find_piece(
                &node->availability,
                pieces[i].piece_id) == NULL)
        {
            if (availability_add_piece(
                    &node->availability,
                    pieces[i].piece_id) != 0)
            {
                pthread_mutex_unlock(&node->availability_mutex);
                free_pieces(pieces);
                return -1;
            }
        }

        if (availability_add_peer(
                &node->availability,
                pieces[i].piece_id,
                remote_peer_id) != 0)
        {
            pthread_mutex_unlock(&node->availability_mutex);
            free_pieces(pieces);
            return -1;
        }
    }

    free_pieces(node->remote_pieces);
    node->remote_pieces = pieces;
    node->remote_piece_count = count;
    node->remote_peer_id = remote_peer_id;

    pthread_mutex_unlock(&node->availability_mutex);

    printf(
        "Received manifest from peer %u: %u pieces\n",
        remote_peer_id,
        count);

    return 0;
}