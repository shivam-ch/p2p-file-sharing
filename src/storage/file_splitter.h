#ifndef FILE_SPLITTER_H
#define FILE_SPLITTER_H

#include <stdint.h>
#include <stddef.h>
#include "piece.h"

#define DEFAULT_PIECE_SIZE (1024ULL * 1024ULL)

int split_file(
    const char *input_path,
    const char *output_dir,
    uint64_t piece_size,
    PieceInfo **pieces,
    size_t *piece_count
);

int reconstruct_file(
    const char *output_path,
    const char *pieces_dir,
    const PieceInfo *pieces,
    size_t piece_count
);

void free_pieces(
    PieceInfo *pieces
);

#endif
