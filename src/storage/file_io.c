#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include "file_io.h"

#include <stdio.h>

int read_piece_block(
    const char *file_path,
    uint64_t offset,
    uint32_t size,
    unsigned char *buffer
)
{
    FILE *file;
    size_t bytes_read;

    if (file_path == NULL || buffer == NULL || size == 0) {
        return -1;
    }

    file = fopen(file_path, "rb");

    if (file == NULL) {
        return -1;
    }

    if (fseeko(file, (off_t)offset, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }

    bytes_read = fread(buffer, 1, size, file);

    fclose(file);

    if (bytes_read != size) {
        return -1;
    }

    return 0;
}

int write_piece_block(
    const char *file_path,
    uint64_t offset,
    uint32_t size,
    const unsigned char *buffer
)
{
    FILE *file;
    size_t bytes_written;

    if (file_path == NULL || buffer == NULL || size == 0) {
        return -1;
    }

    file = fopen(file_path, "r+b");

    if (file == NULL) {
        file = fopen(file_path, "w+b");
    }

    if (file == NULL) {
        return -1;
    }

    if (fseeko(file, (off_t)offset, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }

    bytes_written = fwrite(buffer, 1, size, file);

    if (bytes_written != size) {
        fclose(file);
        return -1;
    }

    if (fflush(file) != 0) {
        fclose(file);
        return -1;
    }

    fclose(file);

    return 0;
}