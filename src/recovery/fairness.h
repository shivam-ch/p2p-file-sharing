#ifndef FAIRNESS_H
#define FAIRNESS_H

#include <stdint.h>
#include <stddef.h>

#define MAX_TRACKED_PEERS 100

typedef struct {
    uint32_t peer_id;
    uint32_t uploaded_pieces;
    uint32_t downloaded_pieces;
    int active;
} PeerContribution;

typedef struct {
    PeerContribution peers[MAX_TRACKED_PEERS];
    size_t peer_count;
} FairnessTable;

void fairness_init(FairnessTable *table);

int fairness_add_peer(
    FairnessTable *table,
    uint32_t peer_id
);

int fairness_record_upload(
    FairnessTable *table,
    uint32_t peer_id
);

int fairness_record_download(
    FairnessTable *table,
    uint32_t peer_id
);

PeerContribution *fairness_find_peer(
    FairnessTable *table,
    uint32_t peer_id
);

#endif
