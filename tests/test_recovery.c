#include <stdio.h>
#include <stdint.h>

#include "../src/recovery/recovery.h"

static int attempt_download(
    uint32_t peer_id,
    uint32_t piece_id,
    void *context
)
{
    (void)context;

    printf(
        "Requesting Piece %u from Peer %u\n",
        piece_id,
        peer_id
    );

    /*
     * Simulate Peer 1 failure.
     * Peer 2 succeeds.
     */
    if (peer_id == 1) {
        printf("Peer 1 failed\n");
        return -1;
    }

    printf("Peer %u sent Piece %u successfully\n",
           peer_id,
           piece_id);

    return 0;
}

int main(void)
{
    FailureTable failure_table;
    RecoveryTask task;

    uint32_t peers[] = {1, 2};
    size_t peer_count = 2;

    PeerInfo peer1 = {
        .peer_id = 1,
        .ip = "127.0.0.1",
        .port = 5001
    };

    PeerInfo peer2 = {
        .peer_id = 2,
        .ip = "127.0.0.1",
        .port = 5002
    };

    failure_table_init(&failure_table);

    failure_add_peer(&failure_table, peer1);
    failure_add_peer(&failure_table, peer2);

    recovery_task_init(
        &task,
        5,
        1
    );

    printf("Starting recovery test...\n\n");

    int result = recovery_retry(
        &failure_table,
        &task,
        peers,
        peer_count,
        attempt_download,
        NULL
    );

    printf("\n");

    if (result == RECOVERY_SUCCESS) {
        printf("PASS: Piece recovered from alternate peer\n");
    } else {
        printf("FAIL: Recovery failed\n");
        return 1;
    }

    if (!failure_is_available(&failure_table, 1)) {
        printf("PASS: Failed peer marked unavailable\n");
    } else {
        printf("FAIL: Failed peer still available\n");
        return 1;
    }

    printf("\nRecovery test completed successfully.\n");

    return 0;
}
