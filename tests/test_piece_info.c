#include "../src/storage/availability.h"

#include <stdio.h>

int main(void)
{
    unsigned char buffer[8];
    uint32_t output_size;

    uint32_t original_piece_id = 5;
    uint32_t original_peer_id = 3;

    uint32_t received_piece_id;
    uint32_t received_peer_id;

    if (serialize_piece_info(
            original_piece_id,
            original_peer_id,
            buffer,
            sizeof(buffer),
            &output_size
        ) != 0) {

        printf("Serialization failed.\n");
        return 1;
    }

    printf(
        "Serialized piece info size: %u bytes\n",
        output_size
    );

    if (deserialize_piece_info(
            buffer,
            output_size,
            &received_piece_id,
            &received_peer_id
        ) != 0) {

        printf("Deserialization failed.\n");
        return 1;
    }

    printf(
        "Received piece ID: %u\n",
        received_piece_id
    );

    printf(
        "Received peer ID: %u\n",
        received_peer_id
    );

    if (received_piece_id != original_piece_id ||
        received_peer_id != original_peer_id) {

        printf("Piece info mismatch.\n");
        return 1;
    }

    printf("Piece info serialization tests passed.\n");

    return 0;
}