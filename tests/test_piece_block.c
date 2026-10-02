#include <stdio.h>
#include <string.h>

#include "../src/transfer/transfer.h"

int main(void)
{
    PieceBlock original;
    PieceBlock restored;

    unsigned char data[] = "HELLO PIECE BLOCK";
    unsigned char restored_data[100];
    unsigned char buffer[MAX_PAYLOAD_SIZE];

    uint32_t output_size;

    original.piece_id = 5;
    original.offset = 5242880ULL;
    original.data_size = (uint32_t)strlen((char *)data);

    if (serialize_piece_block(
            &original,
            data,
            buffer,
            sizeof(buffer),
            &output_size
        ) != 0) {
        printf("Serialization failed.\n");
        return 1;
    }

    printf("Serialized block size: %u bytes\n", output_size);

    if (deserialize_piece_block(
            buffer,
            output_size,
            &restored,
            restored_data,
            sizeof(restored_data)
        ) != 0) {
        printf("Deserialization failed.\n");
        return 1;
    }

    restored_data[restored.data_size] = '\0';

    printf("Piece ID: %u\n", restored.piece_id);
    printf("Offset: %llu\n",
           (unsigned long long)restored.offset);
    printf("Data size: %u\n", restored.data_size);
    printf("Data: %s\n", restored_data);

    if (restored.piece_id != original.piece_id ||
        restored.offset != original.offset ||
        restored.data_size != original.data_size ||
        memcmp(data, restored_data, original.data_size) != 0) {
        printf("Piece block test FAILED.\n");
        return 1;
    }

    printf("Piece block serialization test passed.\n");

    return 0;
}