#ifndef FAILURE_H
#define FAILURE_H

#include <stdint.h>
#include "../common/types.h"

typedef enum {
    PEER_AVAILABLE = 0,
    PEER_FAILED,
    PEER_DISCONNECTED
} PeerStatus;

typedef struct {
    PeerInfo peer;
    PeerStatus status;
    uint32_t failed_attempts;
} PeerFailureState;

void failure_init(PeerFailureState *state, PeerInfo peer);

void failure_mark_failed(PeerFailureState *state);

void failure_mark_disconnected(PeerFailureState *state);

void failure_mark_available(PeerFailureState *state);

int failure_is_available(const PeerFailureState *state);

#endif
