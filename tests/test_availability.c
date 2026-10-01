#include "../src/storage/availability.h"
#include <stdlib.h>
#include <stdio.h>

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
        return 1;
    }

    printf(
        "Piece count after adding pieces: %zu\n",
        table.piece_count
    );

    /* Piece 0 is available from Peer A and Peer C */
    availability_add_peer(&table, 0, 1);
    availability_add_peer(&table, 0, 3);

    /* Piece 1 is available from Peer B */
    availability_add_peer(&table, 1, 2);

    /* Piece 2 is available from all three peers */
    availability_add_peer(&table, 2, 1);
    availability_add_peer(&table, 2, 2);
    availability_add_peer(&table, 2, 3);

    PieceAvailability *piece = availability_find_piece(
        &table,
        2
    );

    if (piece == NULL) {
        printf("Failed to find Piece 2.\n");
        return 1;
    }

    printf(
        "Piece 2 is available from %zu peers:\n",
        piece->peer_count
    );

    for (size_t i = 0; i < piece->peer_count; i++) {

        printf(
            "  Peer %u\n",
            piece->peer_ids[i]
        );
    }

    /* Remove Peer 2 from Piece 2 */
    if (availability_remove_peer(
            &table,
            2,
            2
        ) != 0) {

        printf("Failed to remove Peer 2.\n");
        return 1;
    }

    piece = availability_find_piece(&table, 2);

    printf(
        "Piece 2 after removing Peer 2: %zu peers\n",
        piece->peer_count
    );

    if (piece->peer_count != 2) {
        printf("Peer removal test failed.\n");
        return 1;
    }

    printf("Availability tests passed.\n");

    free(table.pieces);

    return 0;
}