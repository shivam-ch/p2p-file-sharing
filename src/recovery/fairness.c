#include "fairness.h"

#include <stddef.h>

void fairness_init(FairnessTable *table)
{
    if (table == NULL) {
        return;
    }

    table->peer_count = 0;

    for (size_t i = 0; i < MAX_TRACKED_PEERS; i++) {
        table->peers[i].peer_id = 0;
        table->peers[i].uploaded_pieces = 0;
        table->peers[i].downloaded_pieces = 0;
        table->peers[i].active = 0;
    }
}

PeerContribution *fairness_find_peer(
    FairnessTable *table,
    uint32_t peer_id
)
{
    if (table == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < table->peer_count; i++) {
        if (table->peers[i].active &&
            table->peers[i].peer_id == peer_id) {
            return &table->peers[i];
        }
    }

    return NULL;
}

int fairness_add_peer(
    FairnessTable *table,
    uint32_t peer_id
)
{
    if (table == NULL ||
        table->peer_count >= MAX_TRACKED_PEERS) {
        return -1;
    }

    if (fairness_find_peer(table, peer_id) != NULL) {
        return 0;
    }

    PeerContribution *peer =
        &table->peers[table->peer_count];

    peer->peer_id = peer_id;
    peer->uploaded_pieces = 0;
    peer->downloaded_pieces = 0;
    peer->active = 1;

    table->peer_count++;

    return 0;
}

int fairness_record_upload(
    FairnessTable *table,
    uint32_t peer_id
)
{
    PeerContribution *peer =
        fairness_find_peer(table, peer_id);

    if (peer == NULL) {
        return -1;
    }

    peer->uploaded_pieces++;

    return 0;
}

int fairness_record_download(
    FairnessTable *table,
    uint32_t peer_id
)
{
    PeerContribution *peer =
        fairness_find_peer(table, peer_id);

    if (peer == NULL) {
        return -1;
    }

    peer->downloaded_pieces++;

    return 0;
}
