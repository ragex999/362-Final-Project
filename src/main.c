#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/adc.h" // need this library for adc

//////////////////////////////////////////////////////////////////////////////

const char* username = "wang5940";

//////////////////////////////////////////////////////////////////////////////

uint32_t adc_fifo_out = 0;
void display_init_pins();
void display_init_timer();
void display_char_print(const char* buffer);
void autotest();

//////////////////////////////////////////////////////////////////////////////

// Only uncomment ONE step at a time.

// When testing manual ADC single-shot conversion
// #define STEP2
// When testing manual ADC free-running conversion
#define STEP3
// When testing automated ADC sampling with DMA
// #define STEP4

//////////////////////////////////////////////////////////////////////////////

#define ADC_CH   5          // adc channel 5
#define ADC_GPIO 45         // Channel 5 is on GPIO45 on RP2350B (QFN-80)

void init_adc() {
    // fill in
    adc_init();                    // powers up the adc block (CS.EN)
    adc_gpio_init(ADC_GPIO);       // prepares GPIO45 for analog (disables digital pad)
    adc_select_input(ADC_CH);      // selects channel 5 (CS.AINSEL = 5)
}

uint16_t read_adc() {
    // fill in
    // start a single-shot conversion (CS.START_ONCE = 1)
    hw_set_bits(&adc_hw->cs, ADC_CS_START_ONCE_BITS);

    // wait until the adc is ready again (conversion finished: CS.READY = 1)
    while (!(adc_hw->cs & ADC_CS_READY_BITS)) { // bitwise and to test if the READY bit is set
        tight_loop_contents(); 
    }
    return (uint16_t)adc_hw->result;   // 12-bit result (0..4095)
}

void init_adc_freerun() {
    // fill in
    adc_init();                          // powers up ADC (CS.EN=1)
    adc_gpio_init(ADC_GPIO);             // prepare GPIO45 for analog
    adc_select_input(ADC_CH);            // CS.AINSEL = 5
    adc_set_round_robin(0);              // single channel only (CS.RROBIN = 0)
    adc_set_clkdiv(0.0f);                // DIV = 0 -> back-to-back conversions
    hw_set_bits(&adc_hw->cs, ADC_CS_START_MANY_BITS);  // start free-running
}

void init_dma() {
    // fill in
}

void init_adc_dma() {
    // fill in
}

//////////////////////////////////////////////////////////////////////////////

int main()
{
    // Configures our microcontroller to 
    // communicate over UART through the TX/RX pins
    stdio_init_all();

    // Uncomment when you need to run autotest.
    // Keep this commented out until you need it
    // since it adds a lot of time to the upload process.
    autotest();

    // Step 2 - singleshot
    #ifdef STEP2
    init_adc();
    for(;;) {
        printf("ADC Result: %d     \r", read_adc());
        // We've found that when we do NOT send a newline character,
        // the output is not flushed immediately, which can cause
        // the output to be delayed or not appear at all in some cases.
        // The fflush function forces a flush.
        fflush(stdout);
        sleep_ms(250);
    }
    #endif

    // Step 3 - freerun
    #ifdef STEP3
    init_adc_freerun();

    int i = 0;
    for(;;) {
        printf("ADC Result: %d     \r", adc_hw->result);
        fflush(stdout);
        sleep_ms(250);
    }
    #endif

    // Step 4 - adc_dma
    #ifdef STEP4

    // Don't forget to copy in display.c from lab 3.
    display_init_pins();
    display_init_timer();

    init_adc_dma();
    char buffer[10];
    for(;;) {
        float f = (adc_fifo_out * 3.3) / 4095.0;
        snprintf(buffer, sizeof(buffer), "%1.7f", f);
        display_char_print(buffer);

        // If you need to debug without the display, 
        // you can uncomment the following lines.

        // printf("ADC Result: %s     \r", buffer);
        // fflush(stdout);
        
        sleep_ms(250);
    }
    #endif

    for(;;);
    return 0;
}
