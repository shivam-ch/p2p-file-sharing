#ifndef AVAILABILITY_H
#define AVAILABILITY_H

#include <stddef.h>
#include <stdint.h>

#include "../common/types.h"

#define MAX_PIECE_PEERS 10

typedef struct {
    uint32_t piece_id;
    uint32_t peer_ids[MAX_PIECE_PEERS];
    size_t peer_count;
} PieceAvailability;

typedef struct {
    PieceAvailability *pieces;
    size_t piece_count;
} AvailabilityTable;

void availability_init(
    AvailabilityTable *table
);

int availability_add_piece(
    AvailabilityTable *table,
    uint32_t piece_id
);

int availability_add_peer(
    AvailabilityTable *table,
    uint32_t piece_id,
    uint32_t peer_id
);

int availability_remove_peer(
    AvailabilityTable *table,
    uint32_t piece_id,
    uint32_t peer_id
);

PieceAvailability *availability_find_piece(
    AvailabilityTable *table,
    uint32_t piece_id
);

int serialize_piece_info(
    uint32_t piece_id,
    uint32_t peer_id,
    unsigned char *buffer,
    uint32_t buffer_size,
    uint32_t *output_size
);

int deserialize_piece_info(
    const unsigned char *buffer,
    uint32_t buffer_size,
    uint32_t *piece_id,
    uint32_t *peer_id
);

int send_piece_info(
    int socket_fd,
    uint32_t piece_id,
    uint32_t peer_id
);

int receive_piece_info(
    int socket_fd,
    uint32_t *piece_id,
    uint32_t *peer_id
);

#endif