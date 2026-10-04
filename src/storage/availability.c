#include "availability.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "../common/protocol.h"

void availability_init(
    AvailabilityTable *table
)
{
    if (table == NULL) {
        return;
    }

    table->pieces = NULL;
    table->piece_count = 0;
}

int availability_add_piece(
    AvailabilityTable *table,
    uint32_t piece_id
)
{
    PieceAvailability *new_pieces;

    if (table == NULL) {
        return -1;
    }

    if (availability_find_piece(table, piece_id) != NULL) {
        return -1;
    }

    new_pieces = realloc(
        table->pieces,
        (table->piece_count + 1) * sizeof(PieceAvailability)
    );

    if (new_pieces == NULL) {
        return -1;
    }

    table->pieces = new_pieces;

    table->pieces[table->piece_count].piece_id = piece_id;
    table->pieces[table->piece_count].peer_count = 0;

    table->piece_count++;

    return 0;
}

int availability_add_peer(
    AvailabilityTable *table,
    uint32_t piece_id,
    uint32_t peer_id
)
{
    PieceAvailability *piece;

    if (table == NULL) {
        return -1;
    }

    piece = availability_find_piece(table, piece_id);

    if (piece == NULL) {
        return -1;
    }

    if (piece->peer_count >= MAX_PIECE_PEERS) {
        return -1;
    }

    for (size_t i = 0; i < piece->peer_count; i++) {

        if (piece->peer_ids[i] == peer_id) {
            return -1;
        }
    }

    piece->peer_ids[piece->peer_count] = peer_id;
    piece->peer_count++;

    return 0;
}

int availability_remove_peer(
    AvailabilityTable *table,
    uint32_t piece_id,
    uint32_t peer_id
)
{
    PieceAvailability *piece;

    if (table == NULL) {
        return -1;
    }

    piece = availability_find_piece(table, piece_id);

    if (piece == NULL) {
        return -1;
    }

    for (size_t i = 0; i < piece->peer_count; i++) {

        if (piece->peer_ids[i] == peer_id) {

            for (; i + 1 < piece->peer_count; i++) {
                piece->peer_ids[i] = piece->peer_ids[i + 1];
            }

            piece->peer_count--;

            return 0;
        }
    }

    return -1;
}

PieceAvailability *availability_find_piece(
    AvailabilityTable *table,
    uint32_t piece_id
)
{
    if (table == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < table->piece_count; i++) {

        if (table->pieces[i].piece_id == piece_id) {
            return &table->pieces[i];
        }
    }

    return NULL;
}

int availability_select_peer(
    AvailabilityTable *table,
    uint32_t piece_id,
    uint32_t *peer_id
)
{
    PieceAvailability *piece;

    if (table == NULL || peer_id == NULL) {
        return -1;
    }

    piece = availability_find_piece(table, piece_id);

    if (piece == NULL || piece->peer_count == 0) {
        return -1;
    }

    *peer_id = piece->peer_ids[0];

    return 0;
}

int serialize_piece_info(
    uint32_t piece_id,
    uint32_t peer_id,
    unsigned char *buffer,
    uint32_t buffer_size,
    uint32_t *output_size
)
{
    uint32_t network_piece_id;
    uint32_t network_peer_id;

    if (buffer == NULL || output_size == NULL) {
        return -1;
    }

    if (buffer_size < sizeof(uint32_t) * 2) {
        return -1;
    }

    network_piece_id = htonl(piece_id);
    network_peer_id = htonl(peer_id);

    memcpy(
        buffer,
        &network_piece_id,
        sizeof(network_piece_id)
    );

    memcpy(
        buffer + sizeof(network_piece_id),
        &network_peer_id,
        sizeof(network_peer_id)
    );

    *output_size = sizeof(uint32_t) * 2;

    return 0;
}

int deserialize_piece_info(
    const unsigned char *buffer,
    uint32_t buffer_size,
    uint32_t *piece_id,
    uint32_t *peer_id
)
{
    uint32_t network_piece_id;
    uint32_t network_peer_id;

    if (buffer == NULL ||
        piece_id == NULL ||
        peer_id == NULL) {
        return -1;
    }

    if (buffer_size != sizeof(uint32_t) * 2) {
        return -1;
    }

    memcpy(
        &network_piece_id,
        buffer,
        sizeof(network_piece_id)
    );

    memcpy(
        &network_peer_id,
        buffer + sizeof(network_piece_id),
        sizeof(network_peer_id)
    );

    *piece_id = ntohl(network_piece_id);
    *peer_id = ntohl(network_peer_id);

    return 0;
}

int send_piece_info(
    int socket_fd,
    uint32_t piece_id,
    uint32_t peer_id
)
{
    unsigned char buffer[8];
    uint32_t payload_size;

    if (serialize_piece_info(
            piece_id,
            peer_id,
            buffer,
            sizeof(buffer),
            &payload_size
        ) != 0) {
        return -1;
    }

    return send_message(
        socket_fd,
        MSG_PIECE_INFO,
        buffer,
        payload_size
    );
}

int receive_piece_info(
    int socket_fd,
    uint32_t *piece_id,
    uint32_t *peer_id
)
{
    unsigned char buffer[8];
    MessageType message_type;
    uint32_t payload_size;

    if (piece_id == NULL || peer_id == NULL) {
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

    if (message_type != MSG_PIECE_INFO) {
        return -1;
    }

    return deserialize_piece_info(
        buffer,
        payload_size,
        piece_id,
        peer_id
    );
}