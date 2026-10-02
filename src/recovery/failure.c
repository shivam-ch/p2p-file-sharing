#include "failure.h"

#include <stddef.h>

void failure_table_init(FailureTable *table)
{
    if (table == NULL) {
        return;
    }

    table->peer_count = 0;

    for (size_t i = 0; i < MAX_FAILED_PEERS; i++) {
        table->peers[i].peer.peer_id = 0;
        table->peers[i].peer.ip[0] = '\0';
        table->peers[i].peer.port = 0;
        table->peers[i].status = PEER_AVAILABLE;
        table->peers[i].failed_attempts = 0;
    }
}

int failure_add_peer(
    FailureTable *table,
    PeerInfo peer
)
{
    if (table == NULL ||
        table->peer_count >= MAX_FAILED_PEERS) {
        return -1;
    }

    if (failure_find_peer(table, peer.peer_id) != NULL) {
        return -1;
    }

    PeerFailureState *state =
        &table->peers[table->peer_count];

    state->peer = peer;
    state->status = PEER_AVAILABLE;
    state->failed_attempts = 0;

    table->peer_count++;

    return 0;
}

PeerFailureState *failure_find_peer(
    FailureTable *table,
    uint32_t peer_id
)
{
    if (table == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < table->peer_count; i++) {
        if (table->peers[i].peer.peer_id == peer_id) {
            return &table->peers[i];
        }
    }

    return NULL;
}

int failure_mark_failed(
    FailureTable *table,
    uint32_t peer_id
)
{
    PeerFailureState *state =
        failure_find_peer(table, peer_id);

    if (state == NULL) {
        return -1;
    }

    state->status = PEER_FAILED;
    state->failed_attempts++;

    return 0;
}

int failure_mark_disconnected(
    FailureTable *table,
    uint32_t peer_id
)
{
    PeerFailureState *state =
        failure_find_peer(table, peer_id);

    if (state == NULL) {
        return -1;
    }

    state->status = PEER_DISCONNECTED;
    state->failed_attempts++;

    return 0;
}

int failure_mark_available(
    FailureTable *table,
    uint32_t peer_id
)
{
    PeerFailureState *state =
        failure_find_peer(table, peer_id);

    if (state == NULL) {
        return -1;
    }

    state->status = PEER_AVAILABLE;

    return 0;
}

int failure_is_available(
    FailureTable *table,
    uint32_t peer_id
)
{
    PeerFailureState *state =
        failure_find_peer(table, peer_id);

    if (state == NULL) {
        return 0;
    }

    return state->status == PEER_AVAILABLE;
}
