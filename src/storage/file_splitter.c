#define _POSIX_C_SOURCE 200809L

#include "file_splitter.h"
#include "sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static int get_file_size(
    FILE *file,
    uint64_t *size
)
{
    if (file == NULL || size == NULL) {
        return -1;
    }

    if (fseeko(file, 0, SEEK_END) != 0) {
        return -1;
    }

    off_t end = ftello(file);

    if (end < 0) {
        return -1;
    }

    *size = (uint64_t)end;

    if (fseeko(file, 0, SEEK_SET) != 0) {
        return -1;
    }

    return 0;
}

static int ensure_directory(
    const char *path
)
{
    struct stat st;

    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0;
        }

        return -1;
    }

    if (mkdir(path, 0755) != 0) {
        return -1;
    }

    return 0;
}

static int copy_piece(
    FILE *input,
    const char *output_path,
    uint64_t piece_size
)
{
    FILE *output = NULL;
    unsigned char *buffer = NULL;

    output = fopen(output_path, "wb");

    if (output == NULL) {
        return -1;
    }

    /*
     * Use a fixed-size buffer instead of allocating
     * the entire piece. This keeps memory usage small
     * even for large pieces.
     */
    const size_t buffer_size = 1024 * 1024;

    buffer = malloc(buffer_size);

    if (buffer == NULL) {
        fclose(output);
        return -1;
    }

    uint64_t remaining = piece_size;

    while (remaining > 0) {

        size_t to_read =
            remaining > buffer_size
                ? buffer_size
                : (size_t)remaining;

        size_t bytes_read =
            fread(buffer, 1, to_read, input);

        if (bytes_read == 0) {

            if (ferror(input)) {
                free(buffer);
                fclose(output);
                return -1;
            }

            break;
        }

        if (fwrite(buffer, 1, bytes_read, output)
            != bytes_read) {

            free(buffer);
            fclose(output);
            return -1;
        }

        remaining -= bytes_read;
    }

    free(buffer);

    if (fclose(output) != 0) {
        return -1;
    }

    return remaining == 0 ? 0 : -1;
}

int split_file(
    const char *input_path,
    const char *output_dir,
    uint64_t piece_size,
    PieceInfo **pieces,
    size_t *piece_count
)
{
    if (input_path == NULL ||
        output_dir == NULL ||
        pieces == NULL ||
        piece_count == NULL ||
        piece_size == 0) {

        return -1;
    }

    *pieces = NULL;
    *piece_count = 0;

    FILE *input = fopen(input_path, "rb");

    if (input == NULL) {
        return -1;
    }

    uint64_t file_size = 0;

    if (get_file_size(input, &file_size) != 0) {
        fclose(input);
        return -1;
    }

    if (ensure_directory(output_dir) != 0) {
        fclose(input);
        return -1;
    }

    /*
     * Empty files don't require any pieces.
     */
    if (file_size == 0) {
        fclose(input);
        return 0;
    }

    uint64_t piece_count_64 =
        (file_size + piece_size - 1) / piece_size;

    if (piece_count_64 > SIZE_MAX / sizeof(PieceInfo)) {
        fclose(input);
        return -1;
    }

    size_t count = (size_t)piece_count_64;

    PieceInfo *metadata =
        calloc(count, sizeof(PieceInfo));

    if (metadata == NULL) {
        fclose(input);
        return -1;
    }

    for (size_t i = 0; i < count; i++) {

        uint64_t offset =
            (uint64_t)i * piece_size;

        uint64_t remaining =
            file_size - offset;

        uint64_t current_size =
            remaining < piece_size
                ? remaining
                : piece_size;

        piece_init(
            &metadata[i],
            (uint32_t)i,
            offset,
            current_size
        );

        char piece_path[1024];

        int written =
            snprintf(
                piece_path,
                sizeof(piece_path),
                "%s/piece_%u",
                output_dir,
                metadata[i].piece_id
            );

        if (written < 0 ||
            (size_t)written >= sizeof(piece_path)) {

            free(metadata);
            fclose(input);
            return -1;
        }

        /*
         * Copy this piece from the original file
         * into its own piece file.
         */
        if (copy_piece(
                input,
                piece_path,
                current_size
            ) != 0) {

            free(metadata);
            fclose(input);
            return -1;
        }

        /*
         * Calculate SHA-256 hash for this piece.
         *
         * The hash is calculated from the corresponding
         * offset and size in the original file.
         */
        if (calculate_sha256(
                input_path,
                offset,
                current_size,
                metadata[i].hash
            ) != 0) {

            free(metadata);
            fclose(input);
            return -1;
        }

        /*
         * The piece exists successfully on disk
         * and its SHA-256 hash has been calculated.
         * It is now safe to mark the piece available.
         */
        piece_set_status(
            &metadata[i],
            PIECE_AVAILABLE
        );
    }

    fclose(input);

    *pieces = metadata;
    *piece_count = count;

    return 0;
}

void free_pieces(
    PieceInfo *pieces
)
{
    free(pieces);
}
