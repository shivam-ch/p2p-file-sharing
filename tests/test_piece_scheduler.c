#include "../src/storage/piece_scheduler.h"

#include <stdio.h>
#include <stdint.h>

int main(void)
{
    AvailabilityTable table;
    availability_init(&table);

    uint32_t ids[] = {10, 11, 12, 13};

    for (size_t i = 0; i < 4; i++) {
        if (availability_add_piece(&table, ids[i]) != 0) {
            printf("Failed to create availability entry.\n");
            availability_free(&table);
            return 1;
        }
    }

    /* Piece 10: three sources. */
    availability_add_peer(&table, 10, 1);
    availability_add_peer(&table, 10, 2);
    availability_add_peer(&table, 10, 3);

    /* Pieces 11 and 12: one source each. */
    availability_add_peer(&table, 11, 1);
    availability_add_peer(&table, 12, 2);

    /* Piece 13 has no sources. Piece 99 is unknown. */
    PieceInfo pieces[5] = {0};
    pieces[0].piece_id = 10;
    pieces[1].piece_id = 11;
    pieces[2].piece_id = 12;
    pieces[3].piece_id = 13;
    pieces[4].piece_id = 99;

    size_t order[5];

    if (piece_scheduler_sort_by_rarity(
            order, 5, pieces, &table) != 0) {
        printf("Scheduler returned an error.\n");
        availability_free(&table);
        return 1;
    }

    size_t expected[] = {1, 2, 0, 3, 4};

    for (size_t i = 0; i < 5; i++) {
        if (order[i] != expected[i]) {
            printf("Rarest-first ordering test failed.\n");
            availability_free(&table);
            return 1;
        }
    }

    /* Empty input must succeed. */
    if (piece_scheduler_sort_by_rarity(
            NULL, 0, NULL, NULL) != 0) {
        printf("Empty-input test failed.\n");
        availability_free(&table);
        return 1;
    }

    /* Non-empty input with invalid arguments must fail. */
    if (piece_scheduler_sort_by_rarity(
            NULL, 5, pieces, &table) != -1) {
        printf("Invalid-argument test failed.\n");
        availability_free(&table);
        return 1;
    }

    printf("Rarest-first ordering test passed.\n");
    printf("Stable tie-order test passed.\n");
    printf("Unavailable-piece ordering test passed.\n");
    printf("Empty-input test passed.\n");
    printf("Invalid-argument test passed.\n");

    availability_free(&table);
    return 0;
}