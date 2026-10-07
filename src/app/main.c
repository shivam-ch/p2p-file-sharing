#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "p2p.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: %s <peer_id> <port>\n", argv[0]);
        return 1;
    }

    uint32_t peer_id = (uint32_t)strtoul(argv[1], NULL, 10);
    uint16_t port = (uint16_t)strtoul(argv[2], NULL, 10);

    P2PNode node;

    if (p2p_node_init(&node, peer_id, port) != 0) {
        printf("Failed to initialize P2P node.\n");
        return 1;
    }

    if (p2p_start_listener(&node) != 0) {
        printf("Failed to start listener on port %u.\n", port);
        p2p_node_free(&node);
        return 1;
    }

    printf("P2P node started.\n");
    printf("Peer ID: %u\n", peer_id);
    printf("Port: %u\n", port);
    printf("Pieces directory: %s\n", node.pieces_dir);
    printf("Waiting for peers...\n");

    char command[256];

    while (node.running) {

        printf("> ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }

        if (strncmp(command, "exit", 4) == 0) {
        break;
        }

        char ip[46];
        unsigned int port;

        if (sscanf(command, "connect %45s %u", ip, &port) == 2) {

            if (port > 65535) {
                printf("Invalid port.\n");
                continue;
            }

            if (p2p_connect_to_peer(
                &node,
                ip,
                (uint16_t)port
            ) != 0) {

            printf("Failed to connect to peer.\n");
            }

            continue;
        }

        printf("Unknown command. Use: connect <ip> <port> or exit\n");
    }

    p2p_node_free(&node);

    return 0;
}