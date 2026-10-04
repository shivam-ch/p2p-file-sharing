#define _POSIX_C_SOURCE 200809L

#include "recovery_transfer.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "../network/peer.h"

static int set_receive_timeout(
    int socket_fd,
    uint32_t timeout_ms
)
{
    struct timeval timeout;

    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec =
        (timeout_ms % 1000) * 1000;

    return setsockopt(
        socket_fd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &timeout,
        sizeof(timeout)
    );
}

int recovery_download_piece(
    DownloadTask *task,
    FailureTable *failure_table,
    FairnessTable *fairness_table,
    const PeerInfo *peers,
    size_t peer_count,
    RecoveryTransferConfig config
)
{
    if (task == NULL ||
        task->piece == NULL ||
        peers == NULL ||
        peer_count == 0) {
        return -1;
    }

    if (config.timeout_ms == 0) {
        config.timeout_ms = 3000;
    }

    if (config.max_attempts == 0) {
        config.max_attempts = 3;
    }

    uint32_t attempts = 0;

    for (size_t i = 0;
         i < peer_count &&
         attempts < config.max_attempts;
         i++) {

        const PeerInfo *peer = &peers[i];

        if (failure_table != NULL &&
            !failure_is_available(
                failure_table,
                peer->peer_id)) {
            continue;
        }

        attempts++;

        printf(
            "[Recovery] Trying peer %u for piece %u\n",
            peer->peer_id,
            task->piece->info.piece_id
        );

        int socket_fd = connect_to_peer(
            peer->ip,
            peer->port
        );

        if (socket_fd < 0) {
            printf(
                "[Recovery] Connection to peer %u failed\n",
                peer->peer_id
            );

            if (failure_table != NULL) {
                failure_mark_failed(
                    failure_table,
                    peer->peer_id
                );
            }

            continue;
        }

        if (set_receive_timeout(
                socket_fd,
                config.timeout_ms
            ) != 0) {

            close(socket_fd);

            if (failure_table != NULL) {
                failure_mark_failed(
                    failure_table,
                    peer->peer_id
                );
            }

            continue;
        }

        DownloadTask retry_task = *task;
        retry_task.socket_fd = socket_fd;
        retry_task.result = -1;

        retry_task.piece->state =
            TRANSFER_DOWNLOADING;

        int result = download_piece(&retry_task);

        close(socket_fd);

        if (result == 0) {

            retry_task.piece->state =
                TRANSFER_COMPLETED;

            retry_task.result = 0;

            if (failure_table != NULL) {
                failure_mark_available(
                    failure_table,
                    peer->peer_id
                );
            }

            if (fairness_table != NULL) {
                fairness_record_download(
                    fairness_table,
                    peer->peer_id
                );
            }

            task->result = 0;

            printf(
                "[Recovery] Piece %u recovered from peer %u\n",
                task->piece->info.piece_id,
                peer->peer_id
            );

            return 0;
        }

        retry_task.piece->state =
            TRANSFER_FAILED;

        if (failure_table != NULL) {
            failure_mark_failed(
                failure_table,
                peer->peer_id
            );
        }

        printf(
            "[Recovery] Peer %u failed for piece %u\n",
            peer->peer_id,
            task->piece->info.piece_id
        );
    }

    task->result = -1;
    task->piece->state = TRANSFER_FAILED;

    return -1;
}
