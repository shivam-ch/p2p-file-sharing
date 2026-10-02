#include "file_splitter.h"
#include "sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static int hashes_equal(
    const unsigned char *a,
    const unsigned char *b
)
{
    for (size_t i = 0; i < SHA256_HASH_SIZE; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }

    return 1;
}

int main(void)
{
    const char *input = "tests/split_input.bin";
    const char *output = "tests/pieces";

    FILE *file = fopen(input, "wb");

    if (file == NULL) {
        perror("fopen");
        return 1;
    }

    /*
     * Create 5 MiB test file.
     */
    for (int i = 0; i < 5 * 1024; i++) {
        unsigned char buffer[1024];

        for (size_t j = 0; j < sizeof(buffer); j++) {
            buffer[j] = (unsigned char)(j % 256);
        }

        if (fwrite(buffer, 1, sizeof(buffer), file)
            != sizeof(buffer)) {

            fclose(file);
            return 1;
        }
    }

    fclose(file);

    PieceInfo *pieces = NULL;
    size_t piece_count = 0;

    if (split_file(
            input,
            output,
            1024 * 1024,
            &pieces,
            &piece_count
        ) != 0) {

        fprintf(stderr, "File splitting failed\n");
        return 1;
    }

    /*
     * Check number of pieces.
     */
    if (piece_count != 5) {
        fprintf(
            stderr,
            "Expected 5 pieces, got %zu\n",
            piece_count
        );

        free_pieces(pieces);
        return 1;
    }

    /*
     * Check metadata, status and SHA-256 hash
     * for every piece.
     */
    for (size_t i = 0; i < piece_count; i++) {

        /*
         * Check piece ID.
         */
        if (pieces[i].piece_id != i) {
            fprintf(stderr, "Piece ID test failed\n");
            free_pieces(pieces);
            return 1;
        }

        /*
         * Check piece offset.
         */
        if (pieces[i].offset !=
            (uint64_t)i * 1024 * 1024) {

            fprintf(stderr, "Piece offset test failed\n");
            free_pieces(pieces);
            return 1;
        }

        /*
         * Check piece size.
         */
        if (pieces[i].size != 1024 * 1024) {
            fprintf(stderr, "Piece size test failed\n");
            free_pieces(pieces);
            return 1;
        }

        /*
         * Check piece status.
         */
        if (pieces[i].status != PIECE_AVAILABLE) {
            fprintf(stderr, "Piece status test failed\n");
            free_pieces(pieces);
            return 1;
        }

        /*
         * Calculate the expected SHA-256 hash
         * directly from the corresponding range
         * of the original file.
         */
        unsigned char expected_hash[SHA256_HASH_SIZE];

        if (calculate_sha256(
                input,
                pieces[i].offset,
                pieces[i].size,
                expected_hash
            ) != 0) {

            fprintf(
                stderr,
                "Expected hash calculation failed for piece %zu\n",
                i
            );

            free_pieces(pieces);
            return 1;
        }

        /*
         * Compare the hash stored in PieceInfo
         * with the expected SHA-256 hash.
         */
        if (!hashes_equal(
                pieces[i].hash,
                expected_hash
            )) {

            fprintf(
                stderr,
                "Piece %zu SHA-256 hash test failed\n",
                i
            );

            free_pieces(pieces);
            return 1;
        }
    }

    free_pieces(pieces);

    printf("File splitting and SHA-256 tests passed\n");

    return 0;
}
