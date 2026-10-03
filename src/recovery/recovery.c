#include "recovery.h"

#include <stddef.h>

void recovery_task_init(
    RecoveryTask *task,
    uint32_t piece_id,
    uint32_t initial_peer_id
)
{
    if (task == NULL) {
        return;
    }

    task->piece_id = piece_id;
    task->current_peer_id = initial_peer_id;
    task->attempts = 0;
    task->max_attempts = MAX_RECOVERY_ATTEMPTS;
}

int recovery_handle_failure(
    FailureTable *failure_table,
    RecoveryTask *task,
    uint32_t peer_id
)
{
    if (failure_table == NULL || task == NULL) {
        return -1;
    }

    if (failure_mark_failed(failure_table, peer_id) != 0) {
        return -1;
    }

    task->attempts++;

    return 0;
}

int recovery_select_peer(
    FailureTable *failure_table,
    RecoveryTask *task,
    const uint32_t *peer_ids,
    size_t peer_count
)
{
    if (failure_table == NULL ||
        task == NULL ||
        peer_ids == NULL) {
        return -1;
    }

    for (size_t i = 0; i < peer_count; i++) {
        uint32_t peer_id = peer_ids[i];

        if (peer_id == task->current_peer_id) {
            continue;
        }

        if (failure_is_available(failure_table, peer_id)) {
            task->current_peer_id = peer_id;
            return 0;
        }
    }

    return -1;
}

int recovery_retry(
    FailureTable *failure_table,
    RecoveryTask *task,
    const uint32_t *peer_ids,
    size_t peer_count,
    RecoveryAttemptFn attempt,
    void *context
)
{
    if (failure_table == NULL ||
        task == NULL ||
        peer_ids == NULL ||
        attempt == NULL) {
        return RECOVERY_FAILED;
    }

    while (task->attempts < task->max_attempts) {

        if (attempt(
                task->current_peer_id,
                task->piece_id,
                context
            ) == 0) {

            failure_mark_available(
                failure_table,
                task->current_peer_id
            );

            return RECOVERY_SUCCESS;
        }

        recovery_handle_failure(
            failure_table,
            task,
            task->current_peer_id
        );

        if (task->attempts >= task->max_attempts) {
            break;
        }

        if (recovery_select_peer(
                failure_table,
                task,
                peer_ids,
                peer_count
            ) != 0) {
            break;
        }
    }

    return RECOVERY_FAILED;
}
