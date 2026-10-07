#include "../src/network/discovery.h"
#include "../src/common/protocol.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    PeerTable original;
    PeerTable restored;

    unsigned char buffer[MAX_PAYLOAD_SIZE];
    uint32_t output_size;

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

    peer_table_init(&original);
    peer_table_init(&restored);

    peer_table_add(&original, peer_b);
    peer_table_add(&original, peer_c);

    if (serialize_peer_list(
            &original,
            buffer,
            sizeof(buffer),
            &output_size
        ) != 0) {

        printf("Serialization failed.\n");
        peer_table_free(&original);
        peer_table_free(&restored);
        return 1;
    }

    printf(
        "Serialized peer list size: %u bytes\n",
        output_size
    );

    if (deserialize_peer_list(
            &restored,
            buffer,
            output_size
        ) != 0) {

        printf("Deserialization failed.\n");
        peer_table_free(&original);
        peer_table_free(&restored);
        return 1;
    }

    printf(
        "Restored peer count: %zu\n",
        restored.count
    );

    for (size_t i = 0; i < restored.count; i++) {

        printf(
            "Peer %u: IP=%s Port=%u\n",
            restored.peers[i].peer_id,
            restored.peers[i].ip,
            restored.peers[i].port
        );
    }

    if (restored.count != original.count) {
        printf("Peer count mismatch.\n");
        return 1;
    }

    for (size_t i = 0; i < original.count; i++) {

        if (restored.peers[i].peer_id !=
                original.peers[i].peer_id ||

            restored.peers[i].port !=
                original.peers[i].port ||

            strcmp(
                restored.peers[i].ip,
                original.peers[i].ip
            ) != 0) {

            printf("Peer data mismatch.\n");
            return 1;
        }
    }

    printf("Peer list serialization tests passed.\n");

    peer_table_free(&original);
    peer_table_free(&restored);
    return 0;
}