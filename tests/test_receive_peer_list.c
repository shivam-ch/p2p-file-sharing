#include "../src/network/peer.h"
#include "../src/network/discovery.h"

#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int peer_fd;

    PeerTable table;

    printf("Peer B connecting to Peer A...\n");

    peer_fd = connect_to_peer("127.0.0.1", 5001);

    if (peer_fd < 0) {
        printf("Connection failed.\n");
        return 1;
    }

    printf("Connected to Peer A.\n");

    if (receive_peer_list(peer_fd, &table) != 0) {
        printf("Failed to receive peer list.\n");
        close(peer_fd);
        return 1;
    }

    printf(
        "Received peer list containing %zu peers.\n",
        table.count
    );

    for (size_t i = 0; i < table.count; i++) {

        printf(
            "Peer %u: IP=%s Port=%u\n",
            table.peers[i].peer_id,
            table.peers[i].ip,
            table.peers[i].port
        );
    }

    close(peer_fd);

    return 0;
}