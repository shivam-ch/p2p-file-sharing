#include "../src/network/discovery.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    PeerTable table;

    PeerInfo peer_b = {
        .peer_id = 2,
        .ip = "127.0.0.1",
        .port = 5002
    };

    PeerInfo peer_c = {
        .peer_id = 3,
        .ip = "127.0.0.1",
        .port = 5003
    };

    peer_table_init(&table);

    printf("Initial peer count: %zu\n", table.count);

    if (peer_table_add(&table, peer_b) != 0) {
        printf("Failed to add Peer B.\n");
        return 1;
    }

    if (peer_table_add(&table, peer_c) != 0) {
        printf("Failed to add Peer C.\n");
        return 1;
    }

    printf("Peer count after adding B and C: %zu\n", table.count);

    PeerInfo *found = peer_table_find(&table, 3);

    if (found == NULL) {
        printf("Failed to find Peer C.\n");
        return 1;
    }

    printf(
        "Found Peer C: ID=%u IP=%s Port=%u\n",
        found->peer_id,
        found->ip,
        found->port
    );

    if (peer_table_remove(&table, 2) != 0) {
        printf("Failed to remove Peer B.\n");
        return 1;
    }

    printf("Peer count after removing B: %zu\n", table.count);

    if (peer_table_find(&table, 2) != NULL) {
        printf("Peer B was not removed correctly.\n");
        return 1;
    }

    printf("Discovery table tests passed.\n");

    return 0;
}