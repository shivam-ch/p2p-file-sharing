#define _POSIX_C_SOURCE 200809L

#include "piece.h"
#include "sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

void piece_init(
    PieceInfo *piece,
    uint32_t piece_id,
    uint64_t offset,
    uint64_t size
)
{
    if (piece == NULL) {
        return;
    }

    piece->piece_id = piece_id;
    piece->offset = offset;
    piece->size = size;
    piece->status = PIECE_MISSING;
}

void piece_set_status(
    PieceInfo *piece,
    PieceStatus status
)
{
    if (piece == NULL) {
        return;
    }

    piece->status = status;
}

PieceStatus piece_get_status(
    const PieceInfo *piece
)
{
    if (piece == NULL) {
        return PIECE_MISSING;
    }

    return piece->status;
}

int piece_is_available(
    const PieceInfo *piece
)
{
    if (piece == NULL) {
        return 0;
    }

    return piece->status == PIECE_AVAILABLE;
}

static int ensure_directory(
    const char *path
)
{
    struct stat st;

    if (path == NULL) {
        return -1;
    }

    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? 0 : -1;
    }

    if (mkdir(path, 0755) != 0) {
        return -1;
    }

    return 0;
}

int piece_store(
    const char *pieces_dir,
    PieceInfo *piece,
    const unsigned char *data,
    size_t data_size
)
{
    char piece_path[1024];
    FILE *file;

    if (pieces_dir == NULL ||
        piece == NULL ||
        data == NULL) {
        return -1;
    }

    if (data_size != piece->size) {
        return -1;
    }

    if (ensure_directory(pieces_dir) != 0) {
        return -1;
    }

    int written = snprintf(
        piece_path,
        sizeof(piece_path),
        "%s/piece_%u",
        pieces_dir,
        piece->piece_id
    );

    if (written < 0 ||
        (size_t)written >= sizeof(piece_path)) {
        return -1;
    }

    file = fopen(piece_path, "wb");

    if (file == NULL) {
        return -1;
    }

    if (fwrite(data, 1, data_size, file) != data_size) {
        fclose(file);
        remove(piece_path);
        return -1;
    }

    if (fclose(file) != 0) {
        remove(piece_path);
        return -1;
    }

    int verification = verify_sha256(
        piece_path,
        0,
        piece->size,
        piece->hash
    );

    if (verification != 1) {
        piece->status = PIECE_MISSING;
        remove(piece_path);
        return -1;
    }

    piece->status = PIECE_AVAILABLE;
    return 0;
}
int piece_read(
    const char *pieces_dir,
    const PieceInfo *piece,
    unsigned char *buffer,
    size_t buffer_size
)
{
    char piece_path[1024];
    FILE *file;
    size_t bytes_read;
    int written;

    if (pieces_dir == NULL ||
        piece == NULL ||
        buffer == NULL) {
        return -1;
    }

    if (buffer_size < piece->size) {
        return -1;
    }

    if (!piece_is_available(piece)) {
        return -1;
    }

    written = snprintf(
        piece_path,
        sizeof(piece_path),
        "%s/piece_%u",
        pieces_dir,
        piece->piece_id
    );

    if (written < 0 ||
        (size_t)written >= sizeof(piece_path)) {
        return -1;
    }

    file = fopen(piece_path, "rb");

    if (file == NULL) {
        return -1;
    }

    bytes_read = fread(
        buffer,
        1,
        (size_t)piece->size,
        file
    );

    if (bytes_read != (size_t)piece->size) {
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        return -1;
    }

    return 0;
}
