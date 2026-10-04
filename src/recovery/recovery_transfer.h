#ifndef RECOVERY_TRANSFER_H
#define RECOVERY_TRANSFER_H

#include <stddef.h>
#include <stdint.h>

#include "../common/types.h"
#include "../transfer/transfer.h"
#include "failure.h"
#include "fairness.h"

typedef struct {
    uint32_t timeout_ms;
    uint32_t max_attempts;
} RecoveryTransferConfig;

int recovery_download_piece(
    DownloadTask *task,
    FailureTable *failure_table,
    FairnessTable *fairness_table,
    const PeerInfo *peers,
    size_t peer_count,
    RecoveryTransferConfig config
);

#endif
