#include "../src/network/discovery.h"

#include <stdio.h>
#include <string.h>

#define TEST_PEER_COUNT 20

int main(void)
{
    PeerTable table;

    peer_table_init(&table);

    printf("Initial peer count: %zu\n", table.count);

    /* Add 20 peers */
    for (uint32_t i = 1; i <= TEST_PEER_COUNT; i++) {
        PeerInfo peer;

        peer.peer_id = i;
        snprintf(peer.ip, sizeof(peer.ip), "127.0.0.1");
        peer.port = 5000 + i;

        if (peer_table_add(&table, peer) != 0) {
            printf("Failed to add peer %u.\n", i);
            peer_table_free(&table);
            return 1;
        }
    }

    printf("Peer count after adding %d peers: %zu\n",
           TEST_PEER_COUNT, table.count);

    /* Verify all 20 peers exist */
    if (table.count != TEST_PEER_COUNT) {
        printf("ERROR: Expected %d peers, found %zu.\n",
               TEST_PEER_COUNT, table.count);
        peer_table_free(&table);
        return 1;
    }

    /* Test finding several peers */
    uint32_t test_ids[] = {1, 10, 20};

    for (size_t i = 0; i < 3; i++) {
        PeerInfo *found = peer_table_find(&table, test_ids[i]);

        if (found == NULL) {
            printf("Failed to find Peer %u.\n", test_ids[i]);
            peer_table_free(&table);
            return 1;
        }

        printf(
            "Found Peer %u: IP=%s Port=%u\n",
            found->peer_id,
            found->ip,
            found->port
        );
    }

    /* Remove one peer */
    if (peer_table_remove(&table, 10) != 0) {
        printf("Failed to remove Peer 10.\n");
        peer_table_free(&table);
        return 1;
    }

    printf("Peer count after removing Peer 10: %zu\n", table.count);

    /* Verify Peer 10 was removed */
    if (peer_table_find(&table, 10) != NULL) {
        printf("ERROR: Peer 10 was not removed correctly.\n");
        peer_table_free(&table);
        return 1;
    }

    /* Verify another peer still exists */
    if (peer_table_find(&table, 20) == NULL) {
        printf("ERROR: Peer 20 disappeared unexpectedly.\n");
        peer_table_free(&table);
        return 1;
    }

    printf("Dynamic N-peer discovery test passed.\n");

    peer_table_free(&table);

    return 0;
}