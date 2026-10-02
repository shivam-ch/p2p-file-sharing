#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <stdint.h>

typedef struct {
    uint32_t timeout_ms;
    uint64_t start_time_ms;
} Timeout;

void timeout_start(
    Timeout *timer,
    uint32_t timeout_ms
);

int timeout_expired(
    const Timeout *timer
);

void timeout_reset(
    Timeout *timer
);

#endif
