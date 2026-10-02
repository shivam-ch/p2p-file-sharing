#ifndef FILE_IO_H
#define FILE_IO_H

#include <stdint.h>
#include <stddef.h>

int read_piece_block(
    const char *file_path,
    uint64_t offset,
    uint32_t size,
    unsigned char *buffer
);

int write_piece_block(
    const char *file_path,
    uint64_t offset,
    uint32_t size,
    const unsigned char *buffer
);

#endif