#ifndef PIECE_H
#define PIECE_H

#include <stddef.h>
#include "../common/types.h"

void piece_init(
    PieceInfo *piece,
    uint32_t piece_id,
    uint64_t offset,
    uint64_t size
);

void piece_set_status(
    PieceInfo *piece,
    PieceStatus status
);

PieceStatus piece_get_status(
    const PieceInfo *piece
);

int piece_is_available(
    const PieceInfo *piece
);
int piece_store(
    const char *pieces_dir,
    PieceInfo *piece,
    const unsigned char *data,
    size_t data_size
);
#endif
