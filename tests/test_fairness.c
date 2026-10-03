#include <stdio.h>
#include <stdint.h>

#include "../src/recovery/fairness.h"

int main(void)
{
    FairnessTable table;

    printf("Starting fairness test...\n");

    fairness_init(&table);

    if (fairness_add_peer(&table, 1) != 0 ||
        fairness_add_peer(&table, 2) != 0) {
        printf("FAIL: Could not add peers\n");
        return 1;
    }

    /* Peer 1 uploads 3 pieces */
    fairness_record_upload(&table, 1);
    fairness_record_upload(&table, 1);
    fairness_record_upload(&table, 1);

    /* Peer 1 downloads 1 piece */
    fairness_record_download(&table, 1);

    /* Peer 2 uploads 1 piece */
    fairness_record_upload(&table, 2);

    /* Peer 2 downloads 3 pieces */
    fairness_record_download(&table, 2);
    fairness_record_download(&table, 2);
    fairness_record_download(&table, 2);

    PeerContribution *peer1 =
        fairness_find_peer(&table, 1);

    PeerContribution *peer2 =
        fairness_find_peer(&table, 2);

    if (peer1 == NULL || peer2 == NULL) {
        printf("FAIL: Peer contribution not found\n");
        return 1;
    }

    printf(
        "Peer 1 -> uploaded: %u, downloaded: %u\n",
        peer1->uploaded_pieces,
        peer1->downloaded_pieces
    );

    printf(
        "Peer 2 -> uploaded: %u, downloaded: %u\n",
        peer2->uploaded_pieces,
        peer2->downloaded_pieces
    );

    if (peer1->uploaded_pieces != 3 ||
        peer1->downloaded_pieces != 1) {
        printf("FAIL: Peer 1 contribution incorrect\n");
        return 1;
    }

    if (peer2->uploaded_pieces != 1 ||
        peer2->downloaded_pieces != 3) {
        printf("FAIL: Peer 2 contribution incorrect\n");
        return 1;
    }

    printf("PASS: Upload/download contributions tracked correctly\n");
    printf("Fairness test completed successfully.\n");

    return 0;
}
