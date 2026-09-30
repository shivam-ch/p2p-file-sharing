#include "failure.h"
#include <stddef.h>
void failure_init(PeerFailureState *state, PeerInfo peer)
{
    if (state == NULL) {
        return;
    }

    state->peer = peer;
    state->status = PEER_AVAILABLE;
    state->failed_attempts = 0;
}

void failure_mark_failed(PeerFailureState *state)
{
    if (state == NULL) {
        return;
    }

    state->status = PEER_FAILED;
    state->failed_attempts++;
}

void failure_mark_disconnected(PeerFailureState *state)
{
    if (state == NULL) {
        return;
    }

    state->status = PEER_DISCONNECTED;
    state->failed_attempts++;
}

void failure_mark_available(PeerFailureState *state)
{
    if (state == NULL) {
        return;
    }

    state->status = PEER_AVAILABLE;
}

int failure_is_available(const PeerFailureState *state)
{
    if (state == NULL) {
        return 0;
    }

    return state->status == PEER_AVAILABLE;
}

