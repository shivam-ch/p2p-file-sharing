#ifndef PIECE_SCHEDULER_H
#define PIECE_SCHEDULER_H

#include <stddef.h>

#include "../common/types.h"
#include "availability.h"

int piece_scheduler_sort_by_rarity(
    size_t *order,
    size_t count,
    PieceInfo *remote_pieces,
    AvailabilityTable *availability
);

#endif