#include "../src/app/p2p.h"

#include <stdio.h>

int main(void)
{
    P2PNode node;

    if (p2p_node_init(&node, 42, 5042) != 0) {
        printf("P2P node initialization failed.\n");
        return 1;
    }

    printf("Peer ID: %u\n", node.peer_id);
    printf("Port: %u\n", node.port);
    printf("Pieces directory: %s\n", node.pieces_dir);
    printf("Peer count: %zu\n", node.peers.count);
    printf("Piece count: %zu\n", node.piece_count);

    p2p_node_free(&node);

    printf("P2P node lifecycle test passed.\n");

    return 0;
}
