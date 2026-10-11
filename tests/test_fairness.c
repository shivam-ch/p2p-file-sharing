
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "../src/recovery/fairness.h"

int main(void)
{
    FairnessTable table;

    printf("Starting fairness test...\n");

    fairness_init(&table);

    if (fairness_add_peer(&table, 1) != 0 ||
        fairness_add_peer(&table, 2) != 0 ||
        fairness_add_peer(&table, 3) != 0) {
        printf("FAIL: Could not add peers\n");
        return 1;
    }

    /* Peer 1 uploads 3 pieces and downloads 1 */
    fairness_record_upload(&table, 1);
    fairness_record_upload(&table, 1);
    fairness_record_upload(&table, 1);
    fairness_record_download(&table, 1);

    /* Peer 2 uploads 1 piece and downloads 3 */
    fairness_record_upload(&table, 2);
    fairness_record_download(&table, 2);
    fairness_record_download(&table, 2);
    fairness_record_download(&table, 2);

    /* Peer 3 uploads 0 pieces and downloads 1 */
    fairness_record_download(&table, 3);

    PeerContribution *peer1 =
        fairness_find_peer(&table, 1);
    PeerContribution *peer2 =
        fairness_find_peer(&table, 2);
    PeerContribution *peer3 =
        fairness_find_peer(&table, 3);

    if (peer1 == NULL || peer2 == NULL || peer3 == NULL) {
        printf("FAIL: Peer contribution not found\n");
        return 1;
    }

    printf("Peer 1 -> uploaded: %u, downloaded: %u\n",
           peer1->uploaded_pieces, peer1->downloaded_pieces);
    printf("Peer 2 -> uploaded: %u, downloaded: %u\n",
           peer2->uploaded_pieces, peer2->downloaded_pieces);
    printf("Peer 3 -> uploaded: %u, downloaded: %u\n",
           peer3->uploaded_pieces, peer3->downloaded_pieces);

    if (peer1->uploaded_pieces != 3 ||
        peer1->downloaded_pieces != 1 ||
        peer2->uploaded_pieces != 1 ||
        peer2->downloaded_pieces != 3) {
        printf("FAIL: Contribution tracking incorrect\n");
        return 1;
    }

    double ratio1 = fairness_get_ratio(peer1);
    double ratio2 = fairness_get_ratio(peer2);
    double ratio3 = fairness_get_ratio(peer3);

    printf("Peer 1 contribution ratio: %.2f\n", ratio1);
    printf("Peer 2 contribution ratio: %.2f\n", ratio2);
    printf("Peer 3 contribution ratio: %.2f\n", ratio3);

    if (ratio1 != 3.0 ||
        ratio2 < 0.333 || ratio2 > 0.334 ||
        ratio3 != 0.0) {
        printf("FAIL: Contribution ratio incorrect\n");
        return 1;
    }

    printf("PASS: Upload/download contributions tracked correctly\n");
    printf("PASS: Contribution ratios calculated correctly\n");
    printf("Fairness test completed successfully.\n");


    /* Test fairness CSV save and load */
    const char *csv_path = "/tmp/p2p_fairness_test.csv";

    if (fairness_save(&table, csv_path) != 0) {
        printf("FAIL: Could not save fairness history\n");
        return 1;
    }

    FairnessTable loaded;
    fairness_init(&loaded);

    if (fairness_load(&loaded, csv_path) != 0) {
        printf("FAIL: Could not load fairness history\n");
        unlink(csv_path);
        return 1;
    }

    PeerContribution *loaded1 =
        fairness_find_peer(&loaded, 1);
    PeerContribution *loaded2 =
        fairness_find_peer(&loaded, 2);
    PeerContribution *loaded3 =
        fairness_find_peer(&loaded, 3);

    if (loaded1 == NULL || loaded2 == NULL || loaded3 == NULL ||
        loaded1->uploaded_pieces != 3 ||
        loaded1->downloaded_pieces != 1 ||
        loaded2->uploaded_pieces != 1 ||
        loaded2->downloaded_pieces != 3 ||
        loaded3->uploaded_pieces != 0 ||
        loaded3->downloaded_pieces != 1) {
        printf("FAIL: Loaded fairness history does not match\n");
        unlink(csv_path);
        return 1;
    }

    printf("PASS: Fairness history saved and restored correctly\n");

    /* Test missing-file handling */
    unlink(csv_path);

    if (fairness_load(&loaded, csv_path) == 0) {
        printf("FAIL: Missing CSV file was accepted\n");
        return 1;
    }

    printf("PASS: Missing fairness file handled correctly\n");


    return 0;
}
