#include "../src/storage/piece.h"
#include "../src/storage/sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int files_match(
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

    unsigned char buffer_a[1024];
    unsigned char buffer_b[1024];

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

        if (memcmp(
                buffer_a,
                buffer_b,
                read_a
            ) != 0) {

            fclose(a);
            fclose(b);
            return 0;
        }
    }

    fclose(a);
    fclose(b);

    return 1;
}

int main(void)
{
    const char *pieces_dir = "tests/store_pieces";
    const char *original_file = "tests/store_input.bin";

    const unsigned char data[] =
        "This is a test piece for storage.";

    const size_t data_size = sizeof(data) - 1;

    PieceInfo piece;

    piece_init(
        &piece,
        7,
        0,
        data_size
    );

    /*
     * Generate the expected SHA-256 hash from
     * the original test data.
     */
    FILE *input = fopen(original_file, "wb");

    if (input == NULL) {
        printf("Failed to create input file.\n");
        return 1;
    }

    if (fwrite(
            data,
            1,
            data_size,
            input
        ) != data_size) {

        fclose(input);
        return 1;
    }

    fclose(input);

    if (calculate_sha256(
            original_file,
            0,
            data_size,
            piece.hash
        ) != 0) {

        printf("Failed to calculate expected hash.\n");
        remove(original_file);
        return 1;
    }

    piece.status = PIECE_MISSING;

    if (piece_store(
            pieces_dir,
            &piece,
            data,
            data_size
        ) != 0) {

        printf("piece_store() failed.\n");
        remove(original_file);
        return 1;
    }

    if (piece.status != PIECE_AVAILABLE) {
        printf("Piece status was not updated correctly.\n");
        remove(original_file);
        return 1;
    }

    char stored_path[256];

    snprintf(
        stored_path,
        sizeof(stored_path),
        "%s/piece_%u",
        pieces_dir,
        piece.piece_id
    );

    if (!files_match(
            original_file,
            stored_path
        )) {

        printf("Stored piece does not match original data.\n");
        remove(original_file);
        remove(stored_path);
        rmdir(pieces_dir);
        return 1;
    }
        /*
     * Test corrupted data.
     * Change one byte so the SHA-256 hash no longer matches.
     */
    unsigned char corrupted_data[sizeof(data) - 1];

    memcpy(
        corrupted_data,
        data,
        data_size
    );

    corrupted_data[0] ^= 0xFF;

    piece.status = PIECE_MISSING;

    if (piece_store(
            pieces_dir,
            &piece,
            corrupted_data,
            data_size
        ) == 0) {

        printf("Corrupted piece was incorrectly accepted.\n");
        remove(stored_path);
        rmdir(pieces_dir);
        remove(original_file);
        return 1;
    }

    if (piece.status != PIECE_MISSING) {
        printf("Corrupted piece status was not reset correctly.\n");
        remove(stored_path);
        rmdir(pieces_dir);
        remove(original_file);
        return 1;
    }

    if (access(stored_path, F_OK) == 0) {
        printf("Corrupted piece was not removed.\n");
        remove(stored_path);
        rmdir(pieces_dir);
        remove(original_file);
        return 1;
    }
        /*
     * Test reading a successfully stored piece.
     */
    unsigned char read_buffer[sizeof(data) - 1];

    memset(
        read_buffer,
        0,
        sizeof(read_buffer)
    );

    /*
     * Restore the valid piece because the previous
     * corrupted-piece test intentionally removed it.
     */
    if (piece_store(
            pieces_dir,
            &piece,
            data,
            data_size
        ) != 0) {

        printf("Failed to restore valid piece for read test.\n");
        remove(original_file);
        rmdir(pieces_dir);
        return 1;
    }

    if (piece_read(
            pieces_dir,
            &piece,
            read_buffer,
            sizeof(read_buffer)
        ) != 0) {

        printf("piece_read() failed.\n");
        remove(stored_path);
        rmdir(pieces_dir);
        remove(original_file);
        return 1;
    }

    if (memcmp(
            read_buffer,
            data,
            data_size
        ) != 0) {

        printf("Read piece does not match original data.\n");
        remove(stored_path);
        rmdir(pieces_dir);
        remove(original_file);
        return 1;
    }

    printf("Piece read test passed.\n");

    printf("Corrupted piece rejection passed.\n");

    printf("Piece storage test passed.\n");
    printf("Piece SHA-256 verification passed.\n");
    printf("Piece status update passed.\n");

    remove(stored_path);
    rmdir(pieces_dir);
    remove(original_file);

    return 0;
}
