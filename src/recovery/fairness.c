#include "fairness.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
double fairness_get_ratio(const PeerContribution *peer)
{
    if (peer == NULL || peer->downloaded_pieces == 0) {
        return (peer != NULL && peer->uploaded_pieces > 0)
                   ? (double)peer->uploaded_pieces
                   : 0.0;
    }

    return (double)peer->uploaded_pieces /
           (double)peer->downloaded_pieces;
}


int fairness_save(
    const FairnessTable *table,
    const char *filepath
)
{
    if (table == NULL || filepath == NULL) {
        return -1;
    }

    char temp_path[1024];

    int path_len = snprintf(
        temp_path,
        sizeof(temp_path),
        "%s.tmp",
        filepath
    );

    if (path_len < 0 ||
        (size_t)path_len >= sizeof(temp_path)) {
        return -1;
    }

    FILE *file = fopen(temp_path, "w");

    if (file == NULL) {
        return -1;
    }

    int failed = 0;

    if (fprintf(
            file,
            "peer_id,uploaded_pieces,downloaded_pieces\n"
        ) < 0) {
        failed = 1;
    }

    for (size_t i = 0;
         !failed && i < table->peer_count;
         i++) {

        const PeerContribution *peer = &table->peers[i];

        if (!peer->active) {
            continue;
        }

        if (fprintf(
                file,
                "%u,%u,%u\n",
                peer->peer_id,
                peer->uploaded_pieces,
                peer->downloaded_pieces
            ) < 0) {
            failed = 1;
        }
    }

    if (fflush(file) != 0) {
        failed = 1;
    }

    if (fclose(file) != 0) {
        failed = 1;
    }

    if (failed) {
        remove(temp_path);
        return -1;
    }

    if (rename(temp_path, filepath) != 0) {
        remove(temp_path);
        return -1;
    }

    return 0;
}


int fairness_load(
    FairnessTable *table,
    const char *filepath
)
{
    if (table == NULL || filepath == NULL) {
        return -1;
    }

    FILE *file = fopen(filepath, "r");

    if (file == NULL) {
        return -1;
    }

    char line[256];

    /* Skip the CSV header. */
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return -1;
    }

    fairness_init(table);

    while (fgets(line, sizeof(line), file) != NULL) {
        unsigned int peer_id;
        unsigned int uploaded;
        unsigned int downloaded;
        char extra;

        int fields = sscanf(
            line, "%u,%u,%u %c",
            &peer_id, &uploaded, &downloaded, &extra
        );

        if (fields != 3 || peer_id == 0) {
            fclose(file);
            fairness_init(table);
            return -1;
        }

        if (fairness_add_peer(table, (uint32_t)peer_id) != 0) {
            fclose(file);
            fairness_init(table);
            return -1;
        }

        PeerContribution *peer =
            fairness_find_peer(table, (uint32_t)peer_id);

        if (peer == NULL) {
            fclose(file);
            fairness_init(table);
            return -1;
        }

        peer->uploaded_pieces = (uint32_t)uploaded;
        peer->downloaded_pieces = (uint32_t)downloaded;
    }

    if (ferror(file)) {
        fclose(file);
        fairness_init(table);
        return -1;
    }

    fclose(file);
    return 0;
}
