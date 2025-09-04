#ifndef QUEUE_H
#define QUEUE_H

#include <hardware/timer.h>
#include "pico/stdlib.h"

// Basic queue structure for tracking events
// Tracks 32 events, with each event being {pressed, key}
typedef struct {
    uint16_t q[32];
    uint16_t head;
    uint16_t tail;
} KeyEvents;

extern KeyEvents kev; // Global key event queue 

uint16_t key_pop() {
    // Queue is empty
    while (kev.head == kev.tail) {
        sleep_ms(10);   // Wait for an event to be pushed
    }
    uint16_t value = kev.q[kev.tail];
    kev.tail = (kev.tail + 1) % 32;
    return value;
}

void key_push(uint16_t value) {
    // Queue is full, drop new keys
    if ((kev.head + 1) % 32 == kev.tail) {
        return; 
    }
    kev.q[kev.head] = value;
    kev.head = (kev.head + 1) % 32;
}

#endif
