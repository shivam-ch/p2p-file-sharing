#include "../src/transfer/transfer.h"

#include <stdio.h>

int main(void)
{
    PieceRequest original;
    PieceRequest restored;

    unsigned char buffer[20];
    uint32_t output_size;

    original.piece_id = 5;
    original.offset = 5242880ULL;
    original.size = 1048576ULL;

    if (serialize_piece_request(
            &original,
            buffer,
            sizeof(buffer),
            &output_size
        ) != 0) {

        printf("Serialization failed.\n");
        return 1;
    }

    printf(
        "Serialized request size: %u bytes\n",
        output_size
    );

    if (deserialize_piece_request(
            buffer,
            output_size,
            &restored
        ) != 0) {

        printf("Deserialization failed.\n");
        return 1;
    }

    printf("Piece ID: %u\n", restored.piece_id);
    printf("Offset: %llu\n",
           (unsigned long long)restored.offset);
    printf("Size: %llu\n",
           (unsigned long long)restored.size);

    if (restored.piece_id != original.piece_id ||
        restored.offset != original.offset ||
        restored.size != original.size) {

        printf("Piece request mismatch.\n");
        return 1;
    }

    printf("Piece request serialization tests passed.\n");

    return 0;
}