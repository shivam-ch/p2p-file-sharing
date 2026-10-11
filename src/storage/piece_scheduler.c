#include "piece_scheduler.h"

#include <stdint.h>

static size_t get_availability_count(
    AvailabilityTable *availability,
    uint32_t piece_id
)
{
    PieceAvailability *piece =
        availability_find_piece(availability, piece_id);

    if (piece == NULL || piece->peer_count == 0) {
        return SIZE_MAX;
    }

    return piece->peer_count;
}

int piece_scheduler_sort_by_rarity(
    size_t *order,
    size_t count,
    PieceInfo *remote_pieces,
    AvailabilityTable *availability
)
{
    if (count == 0) {
        return 0;
    }

    if (order == NULL ||
        remote_pieces == NULL ||
        availability == NULL) {
        return -1;
    }

    for (size_t i = 0; i < count; i++) {
        order[i] = i;
    }

    /* Stable insertion sort: rarest pieces first. */
    for (size_t i = 1; i < count; i++) {
        size_t key = order[i];

        size_t key_count = get_availability_count(
            availability,
            remote_pieces[key].piece_id
        );

        size_t j = i;

        while (j > 0) {
            size_t previous_count = get_availability_count(
                availability,
                remote_pieces[order[j - 1]].piece_id
            );

            if (previous_count <= key_count) {
                break;
            }

            order[j] = order[j - 1];
            j--;
        }

        order[j] = key;
    }

    return 0;
}