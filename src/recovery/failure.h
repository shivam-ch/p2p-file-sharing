#ifndef FAILURE_H
#define FAILURE_H

#include <stdint.h>
#include <stddef.h>

#include "../common/types.h"

#define MAX_FAILED_PEERS 100

typedef enum {
    PEER_AVAILABLE = 0,
    PEER_FAILED,
    PEER_DISCONNECTED
} PeerStatus;

typedef struct {
    PeerInfo peer;
    PeerStatus status;
    uint32_t failed_attempts;
} PeerFailureState;

typedef struct {
    PeerFailureState peers[MAX_FAILED_PEERS];
    size_t peer_count;
} FailureTable;

void failure_table_init(FailureTable *table);

int failure_add_peer(
    FailureTable *table,
    PeerInfo peer
);

PeerFailureState *failure_find_peer(
    FailureTable *table,
    uint32_t peer_id
);

int failure_mark_failed(
    FailureTable *table,
    uint32_t peer_id
);

int failure_mark_disconnected(
    FailureTable *table,
    uint32_t peer_id
);

int failure_mark_available(
    FailureTable *table,
    uint32_t peer_id
);

int failure_is_available(
    FailureTable *table,
    uint32_t peer_id
);

#endif
