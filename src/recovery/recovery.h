#ifndef RECOVERY_H
#define RECOVERY_H

#include <stddef.h>
#include <stdint.h>

#include "failure.h"

#define MAX_RECOVERY_ATTEMPTS 3

typedef enum {
    RECOVERY_SUCCESS = 0,
    RECOVERY_FAILED = -1
} RecoveryResult;

typedef int (*RecoveryAttemptFn)(
    uint32_t peer_id,
    uint32_t piece_id,
    void *context
);

typedef struct {
    uint32_t piece_id;
    uint32_t current_peer_id;
    uint32_t attempts;
    uint32_t max_attempts;
} RecoveryTask;

void recovery_task_init(
    RecoveryTask *task,
    uint32_t piece_id,
    uint32_t initial_peer_id
);

int recovery_handle_failure(
    FailureTable *failure_table,
    RecoveryTask *task,
    uint32_t peer_id
);

int recovery_select_peer(
    FailureTable *failure_table,
    RecoveryTask *task,
    const uint32_t *peer_ids,
    size_t peer_count
);

int recovery_retry(
    FailureTable *failure_table,
    RecoveryTask *task,
    const uint32_t *peer_ids,
    size_t peer_count,
    RecoveryAttemptFn attempt,
    void *context
);

#endif
