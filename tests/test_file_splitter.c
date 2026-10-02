#include "file_splitter.h"
#include "sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static int files_equal(
    const char *file_a,
    const char *file_b
)
{
    FILE *a = fopen(file_a, "rb");
    FILE *b = fopen(file_b, "rb");

    if (a == NULL || b == NULL) {
        if (a != NULL) {
            fclose(a);
        }

        if (b != NULL) {
            fclose(b);
        }

        return 0;
    }

    unsigned char buffer_a[64 * 1024];
    unsigned char buffer_b[64 * 1024];

    while (1) {

        size_t read_a = fread(
            buffer_a,
            1,
            sizeof(buffer_a),
            a
        );

        size_t read_b = fread(
            buffer_b,
            1,
            sizeof(buffer_b),
            b
        );

        if (read_a != read_b) {
            fclose(a);
            fclose(b);
            return 0;
        }

        if (read_a == 0) {
            break;
        }

        for (size_t i = 0; i < read_a; i++) {
            if (buffer_a[i] != buffer_b[i]) {
                fclose(a);
                fclose(b);
                return 0;
            }
        }
    }

    fclose(a);
    fclose(b);

    return 1;
}

int main(void)
{
    const char *input = "tests/split_input.bin";
    const char *pieces_dir = "tests/pieces";
    const char *reconstructed = "tests/reconstructed.bin";

    /*
     * Create a 5 MiB test file.
     */
    FILE *file = fopen(input, "wb");

    if (file == NULL) {
        perror("fopen");
        return 1;
    }

    for (int i = 0; i < 5 * 1024; i++) {

        unsigned char buffer[1024];

        for (size_t j = 0; j < sizeof(buffer); j++) {
            buffer[j] = (unsigned char)(j % 256);
        }

        if (fwrite(
                buffer,
                1,
                sizeof(buffer),
                file
            ) != sizeof(buffer)) {

            fclose(file);
            return 1;
        }
    }

    fclose(file);

    /*
     * Split the file into 1 MiB pieces.
     */
    PieceInfo *pieces = NULL;
    size_t piece_count = 0;

    if (split_file(
            input,
            pieces_dir,
            1024 * 1024,
            &pieces,
            &piece_count
        ) != 0) {

        fprintf(stderr, "File splitting failed\n");
        return 1;
    }

    /*
     * Verify the number of pieces.
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
     * Verify metadata and SHA-256 hash for
     * every piece.
     */
    for (size_t i = 0; i < piece_count; i++) {

        if (pieces[i].piece_id != i) {

            fprintf(
                stderr,
                "Piece ID test failed\n"
            );

            free_pieces(pieces);
            return 1;
        }

        if (pieces[i].offset !=
            (uint64_t)i * 1024 * 1024) {

            fprintf(
                stderr,
                "Piece offset test failed\n"
            );

            free_pieces(pieces);
            return 1;
        }

        if (pieces[i].size != 1024 * 1024) {

            fprintf(
                stderr,
                "Piece size test failed\n"
            );

            free_pieces(pieces);
            return 1;
        }

        if (!piece_is_available(&pieces[i])) {

            fprintf(
                stderr,
                "Piece availability test failed\n"
            );

            free_pieces(pieces);
            return 1;
        }

        /*
         * Verify the piece's SHA-256 hash.
         */
        char piece_path[1024];

        int written = snprintf(
            piece_path,
            sizeof(piece_path),
            "%s/piece_%u",
            pieces_dir,
            pieces[i].piece_id
        );

        if (written < 0 ||
            (size_t)written >= sizeof(piece_path)) {

            fprintf(
                stderr,
                "Piece path creation failed\n"
            );

            free_pieces(pieces);
            return 1;
        }

        int verification = verify_sha256(
            piece_path,
            0,
            pieces[i].size,
            pieces[i].hash
        );

        if (verification != 1) {

            fprintf(
                stderr,
                "Piece %zu SHA-256 verification failed\n",
                i
            );

            free_pieces(pieces);
            return 1;
        }
    }

    printf("All piece SHA-256 verification tests passed\n");

    /*
     * Reconstruct the original file.
     */
    if (reconstruct_file(
            reconstructed,
            pieces_dir,
            pieces,
            piece_count
        ) != 0) {

        fprintf(
            stderr,
            "File reconstruction failed\n"
        );

        free_pieces(pieces);
        return 1;
    }

    /*
     * Compare the reconstructed file with
     * the original file.
     */
    if (!files_equal(
            input,
            reconstructed
        )) {

        fprintf(
            stderr,
            "Reconstructed file does not match original\n"
        );

        free_pieces(pieces);
        return 1;
    }

    printf(
        "Reconstructed file matches original\n"
    );

    free_pieces(pieces);

    printf(
        "File splitting, verification, and reconstruction tests passed\n"
    );

    return 0;
}
