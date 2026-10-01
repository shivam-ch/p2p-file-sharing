#include "piece.h"
#include <stddef.h>

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
