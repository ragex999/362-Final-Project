#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
// 7-segment display message buffer
// Declared as static to limit scope to this file only.
static char msg[8] = {
    0x3F, // seven-segment value of 0
    0x06, // seven-segment value of 1
    0x5B, // seven-segment value of 2
    0x4F, // seven-segment value of 3
    0x66, // seven-segment value of 4
    0x6D, // seven-segment value of 5
    0x7D, // seven-segment value of 6
    0x07, // seven-segment value of 7
};
extern char font[]; // Font mapping for 7-segment display
static int index = 0; // Current index in the message buffer

// We provide you with this function for directly displaying characters.
// However, it can't use the decimal point, which display_print does.
void display_char_print(const char message[]) {
    for (int i = 0; i < 8; i++) {
        msg[i] = font[message[i] & 0xFF];
    }
}

/********************************************************* */
// Implement the functions below.


#define SEL0 18
#define SEL1 19
#define SEL2 20
#define DATA0 10   // GP10–GP17 are D1–D8

void display_init_pins() {
    for (int p = 10; p <= 20; p++) {// Configure GP10..GP20 as outputs, start low
        gpio_init(p);
        gpio_set_dir(p, GPIO_OUT);
        gpio_put(p, 0);
    }
}

void display_isr() {
    timer1_hw->intr = (1u << 0); // Acknowledge TIMER1 ALARM0 (W1C)

    // Build 11-bit bus: {SEL2,SEL1,SEL0,D8..D1} 
    uint32_t select3 = (uint32_t)(index & 0x7) << 8;   // 3 bits at [10:8] for decoder
    uint32_t seg8    = (uint32_t)(msg[index] & 0xFF);  // 8 bits at [7:0]
    uint32_t bus11   = select3 | seg8;                 // combine

    uint32_t out_bits = bus11 << 10; // Shift into GPIO positions (GP20..GP10)
    uint32_t mask = 0;// Mask for GP10..GP20
    for (int p = 10; p <= 20; ++p){
        mask |= (1u << p);
    }
    gpio_put_masked(mask, out_bits);// Atomically update all 11 pins

    // Increment index and wrap (0–7)
    index = (index + 1) & 0x7;

    timer1_hw->alarm[0] = timer1_hw->timerawl + 3000u; // Re-arm alarm in 3000u (ms)
}

void display_init_timer() {
    timer1_hw->intr = 0xFu; // Clear any latched alarms, 15 unsigned

    timer1_hw->inte |= (1u << 0); // Enable ALARM0 on TIMER1
    
    uint32_t now = timer1_hw->timerawl;// Schedule first alarm in 3 ms
    timer1_hw->alarm[0] = now + 3000u;

    // Hook exclusive handler
    irq_set_exclusive_handler(TIMER1_IRQ_0, display_isr);
    irq_set_enabled(TIMER1_IRQ_0, true);
}

void display_print(const uint16_t message[]) {
    for (int i = 0; i < 8; ++i) {
        uint16_t ev = message[i];
        unsigned char ascii = (unsigned char)(ev & 0xFF);
        unsigned char seg   = (unsigned char)font[ascii];
        if (ev & 0x100) {
            seg |= 0x80;   // pressed → DP on
        } else {
            seg &= ~0x80;  // released → DP off
        }
        msg[i] = (char)seg;
    }
}
