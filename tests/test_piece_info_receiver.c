#include "../src/network/peer.h"
#include "../src/storage/availability.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(void)
{
    int peer_fd;

    uint32_t piece_id;
    uint32_t peer_id;

    AvailabilityTable table;

    availability_init(&table);

    printf("Peer B connecting to Peer A...\n");

    peer_fd = connect_to_peer(
        "127.0.0.1",
        5001
    );

    if (peer_fd < 0) {
        printf("Connection failed.\n");
        return 1;
    }

    printf("Connected to Peer A.\n");

    if (receive_piece_info(
            peer_fd,
            &piece_id,
            &peer_id
        ) != 0) {

        printf("Failed to receive piece info.\n");
        close(peer_fd);
        return 1;
    }

    printf(
        "Received piece information: Piece %u -> Peer %u\n",
        piece_id,
        peer_id
    );

    if (availability_add_piece(&table, piece_id) != 0) {
    printf("Failed to add piece to availability table.\n");
    close(peer_fd);
    return 1;
    }

    if (availability_add_peer(
        &table,
        piece_id,
        peer_id
    ) != 0) {

    printf("Failed to add peer to availability table.\n");
    close(peer_fd);
    return 1;
    }

    PieceAvailability *piece =
    availability_find_piece(&table, piece_id);

    printf(
    "Availability table: Piece %u is available from %zu peer(s).\n",
    piece->piece_id,
    piece->peer_count
    );

    for (size_t i = 0; i < piece->peer_count; i++) {
    printf(
        "  Peer %u\n",
        piece->peer_ids[i]
    );
    }

    free(table.pieces);

    close(peer_fd);

    return 0;
}