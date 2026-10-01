#include "piece.h"

#include <stdio.h>

int main(void)
{
    PieceInfo piece;

    piece_init(&piece, 5, 5242880, 1048576);

    if (piece.piece_id != 5 ||
        piece.offset != 5242880 ||
        piece.size != 1048576) {

        fprintf(stderr, "Piece metadata test failed\n");
        return 1;
    }

    if (piece_get_status(&piece) != PIECE_MISSING) {
        fprintf(stderr, "Initial status test failed\n");
        return 1;
    }

    piece_set_status(&piece, PIECE_DOWNLOADING);

    if (piece_get_status(&piece) != PIECE_DOWNLOADING) {
        fprintf(stderr, "Downloading status test failed\n");
        return 1;
    }

    piece_set_status(&piece, PIECE_AVAILABLE);

    if (!piece_is_available(&piece)) {
        fprintf(stderr, "Available status test failed\n");
        return 1;
    }

    printf("Piece management tests passed\n");

    return 0;
}
