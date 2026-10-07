#include "discovery.h"
#include <stdlib.h>

#include <stddef.h>
#include "../common/protocol.h"
#include <arpa/inet.h>
#include <string.h>

#define INITIAL_PEER_CAPACITY 8

void peer_table_init(PeerTable *table)
{
    if (table == NULL) {
        return;
    }

    table->peers = NULL;
    table->count = 0;
    table->capacity = 0;
}

void peer_table_free(PeerTable *table)
{
    if (table == NULL) {
        return;
    }

    free(table->peers);

    table->peers = NULL;
    table->count = 0;
    table->capacity = 0;
}

int peer_table_add(
    PeerTable *table,
    PeerInfo peer
)
{
    PeerInfo *new_peers;

    if (table == NULL) {
        return -1;
    }

    if (peer_table_find(table, peer.peer_id) != NULL) {
        return -1;
    }

    if (table->count == table->capacity) {

        size_t new_capacity;

        if (table->capacity == 0) {
            new_capacity = INITIAL_PEER_CAPACITY;
        } else {
            new_capacity = table->capacity * 2;
        }

        new_peers = realloc(
            table->peers,
            new_capacity * sizeof(PeerInfo)
        );

        if (new_peers == NULL) {
            return -1;
        }

        table->peers = new_peers;
        table->capacity = new_capacity;
    }

    table->peers[table->count] = peer;
    table->count++;

    return 0;
}

int peer_table_remove(
    PeerTable *table,
    uint32_t peer_id
)
{
    size_t i;

    if (table == NULL) {
        return -1;
    }

    for (i = 0; i < table->count; i++) {

        if (table->peers[i].peer_id == peer_id) {

            for (; i + 1 < table->count; i++) {
                table->peers[i] = table->peers[i + 1];
            }

            table->count--;

            return 0;
        }
    }

    return -1;
}

PeerInfo *peer_table_find(
    PeerTable *table,
    uint32_t peer_id
)
{
    size_t i;

    if (table == NULL) {
        return NULL;
    }

    for (i = 0; i < table->count; i++) {

        if (table->peers[i].peer_id == peer_id) {
            return &table->peers[i];
        }
    }

    return NULL;
}
int serialize_peer_list(
    const PeerTable *table,
    unsigned char *buffer,
    uint32_t buffer_size,
    uint32_t *output_size
)
{
    uint32_t peer_count;
    uint32_t required_size;
    unsigned char *cursor;

    if (table == NULL || buffer == NULL || output_size == NULL) {
        return -1;
    }

    peer_count = (uint32_t)table->count;

    required_size = sizeof(uint32_t) +
                    peer_count * (sizeof(uint32_t) +
                                  sizeof(uint16_t) +
                                  46);

    if (buffer_size < required_size) {
        return -1;
    }

    cursor = buffer;

    uint32_t network_peer_count = htonl(peer_count);

    memcpy(
        cursor,
        &network_peer_count,
        sizeof(network_peer_count)
    );

    cursor += sizeof(network_peer_count);

    for (size_t i = 0; i < table->count; i++) {

        uint32_t network_peer_id =
            htonl(table->peers[i].peer_id);

        uint16_t network_port =
            htons(table->peers[i].port);

        memcpy(
            cursor,
            &network_peer_id,
            sizeof(network_peer_id)
        );

        cursor += sizeof(network_peer_id);

        memcpy(
            cursor,
            &network_port,
            sizeof(network_port)
        );

        cursor += sizeof(network_port);

        memcpy(
            cursor,
            table->peers[i].ip,
            46
        );

        cursor += 46;
    }

    *output_size = required_size;

    return 0;
}
int deserialize_peer_list(
    PeerTable *table,
    const unsigned char *buffer,
    uint32_t buffer_size
)
{
    uint32_t peer_count;
    uint32_t required_size;
    const unsigned char *cursor;

    if (table == NULL || buffer == NULL) {
        return -1;
    }

    if (buffer_size < sizeof(uint32_t)) {
        return -1;
    }

    memcpy(
        &peer_count,
        buffer,
        sizeof(peer_count)
    );

    peer_count = ntohl(peer_count);

    required_size = sizeof(uint32_t) +
                    peer_count * (sizeof(uint32_t) +
                                  sizeof(uint16_t) +
                                  46);

    if (buffer_size != required_size) {
        return -1;
    }

    peer_table_init(table);

    cursor = buffer + sizeof(uint32_t);

    for (uint32_t i = 0; i < peer_count; i++) {

        PeerInfo peer;

        uint32_t network_peer_id;
        uint16_t network_port;

        memcpy(
            &network_peer_id,
            cursor,
            sizeof(network_peer_id)
        );

        cursor += sizeof(network_peer_id);

        memcpy(
            &network_port,
            cursor,
            sizeof(network_port)
        );

        cursor += sizeof(network_port);

        peer.peer_id = ntohl(network_peer_id);
        peer.port = ntohs(network_port);

        memcpy(
            peer.ip,
            cursor,
            46
        );

        peer.ip[45] = '\0';

        cursor += 46;

        if (peer_table_add(table, peer) != 0) {
            peer_table_free(table);
            return -1;
        }
    }

    return 0;
}
int send_peer_list(
    int socket_fd,
    const PeerTable *table
)
{
    unsigned char buffer[MAX_PAYLOAD_SIZE];
    uint32_t payload_size;

    if (table == NULL) {
        return -1;
    }

    if (serialize_peer_list(
            table,
            buffer,
            sizeof(buffer),
            &payload_size
        ) != 0) {
        return -1;
    }

    return send_message(
        socket_fd,
        MSG_PEER_LIST,
        buffer,
        payload_size
    );
}
int receive_peer_list(
    int socket_fd,
    PeerTable *table
)
{
    unsigned char buffer[MAX_PAYLOAD_SIZE];
    MessageType message_type;
    uint32_t payload_size;

    if (table == NULL) {
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

    if (message_type != MSG_PEER_LIST) {
        return -1;
    }

    return deserialize_peer_list(
        table,
        buffer,
        payload_size
    );
}