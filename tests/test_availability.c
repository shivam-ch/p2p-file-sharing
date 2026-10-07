#include "../src/storage/availability.h"

#include <stdio.h>
#include <stdint.h>

#define TEST_PEER_COUNT 20

int main(void)
{
    AvailabilityTable table;

    availability_init(&table);

    printf(
        "Initial piece count: %zu\n",
        table.piece_count
    );

    /* Create three pieces */
    if (availability_add_piece(&table, 0) != 0 ||
        availability_add_piece(&table, 1) != 0 ||
        availability_add_piece(&table, 2) != 0) {

        printf("Failed to add pieces.\n");
        availability_free(&table);
        return 1;
    }

    printf(
        "Piece count after adding pieces: %zu\n",
        table.piece_count
    );

    /*
     * Piece 0 is available from 20 peers.
     * This specifically verifies that there is no
     * fixed 10-peer limitation anymore.
     */
    for (uint32_t peer_id = 1;
         peer_id <= TEST_PEER_COUNT;
         peer_id++) {

        if (availability_add_peer(&table, 0, peer_id) != 0) {
            printf("Failed to add Peer %u to Piece 0.\n", peer_id);
            availability_free(&table);
            return 1;
        }
    }

    PieceAvailability *piece = availability_find_piece(
        &table,
        0
    );

    if (piece == NULL) {
        printf("Failed to find Piece 0.\n");
        availability_free(&table);
        return 1;
    }

    printf(
        "Piece 0 is available from %zu peers.\n",
        piece->peer_count
    );

    if (piece->peer_count != TEST_PEER_COUNT) {
        printf(
            "ERROR: Expected %d peers, found %zu.\n",
            TEST_PEER_COUNT,
            piece->peer_count
        );
        availability_free(&table);
        return 1;
    }

    /*
     * Verify the first and last peer.
     */
    if (piece->peer_ids[0] != 1 ||
        piece->peer_ids[piece->peer_count - 1] != TEST_PEER_COUNT) {

        printf("Peer storage test failed.\n");
        availability_free(&table);
        return 1;
    }

    printf(
        "First peer: %u\n",
        piece->peer_ids[0]
    );

    printf(
        "Last peer: %u\n",
        piece->peer_ids[piece->peer_count - 1]
    );

    /*
     * Piece 1 is available from Peer 21.
     */
    if (availability_add_peer(&table, 1, 21) != 0) {
        printf("Failed to add Peer 21 to Piece 1.\n");
        availability_free(&table);
        return 1;
    }

    /*
     * Piece 2 is available from three peers.
     */
    availability_add_peer(&table, 2, 1);
    availability_add_peer(&table, 2, 2);
    availability_add_peer(&table, 2, 3);

    piece = availability_find_piece(&table, 2);

    if (piece == NULL) {
        printf("Failed to find Piece 2.\n");
        availability_free(&table);
        return 1;
    }

    printf(
        "Piece 2 is available from %zu peers.\n",
        piece->peer_count
    );

    /*
     * Remove Peer 2 from Piece 2.
     */
    if (availability_remove_peer(
            &table,
            2,
            2
        ) != 0) {

        printf("Failed to remove Peer 2.\n");
        availability_free(&table);
        return 1;
    }

    piece = availability_find_piece(&table, 2);

    printf(
        "Piece 2 after removing Peer 2: %zu peers\n",
        piece->peer_count
    );

    if (piece->peer_count != 2) {
        printf("Peer removal test failed.\n");
        availability_free(&table);
        return 1;
    }

    /*
     * Test peer selection.
     * The current implementation selects the first
     * available peer.
     */
    uint32_t selected_peer;

    if (availability_select_peer(
            &table,
            2,
            &selected_peer
        ) != 0) {

        printf("Failed to select a peer for Piece 2.\n");
        availability_free(&table);
        return 1;
    }

    printf(
        "Selected Peer %u for Piece 2.\n",
        selected_peer
    );

    if (selected_peer != 1) {
        printf("Peer selection test failed.\n");
        availability_free(&table);
        return 1;
    }

    /*
     * Test selection for a missing piece.
     */
    if (availability_select_peer(
            &table,
            99,
            &selected_peer
        ) != -1) {

        printf("Missing piece selection test failed.\n");
        availability_free(&table);
        return 1;
    }

    printf("Peer selection tests passed.\n");

    printf("Dynamic availability tests passed.\n");

    availability_free(&table);

    return 0;
}