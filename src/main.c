#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/adc.h" // need this library for adc
#include "hardware/regs/dma.h"
#include "hardware/structs/dma.h"
#include "hardware/regs/dreq.h"

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
// #define STEP3
// When testing automated ADC sampling with DMA
#define STEP4

//////////////////////////////////////////////////////////////////////////////

#define ADC_CH   5          // adc channel 5
#define ADC_GPIO 45         // Channel 5 is GPIO45
void init_adc() {
    // fill in
    adc_init();                    // powers up the adc block 
    adc_gpio_init(ADC_GPIO);       // prepares GPIO45 for analog 
    adc_select_input(ADC_CH);      // selects channel 5 as adc input
}

uint16_t read_adc() {
    // fill in
    // start a single-shot conversion (CS.START_ONCE = 1)
    hw_set_bits(&adc_hw->cs, ADC_CS_START_ONCE_BITS);

    // wait until the adc is ready again (conversion finished: CS.READY = 1)
    while (!(adc_hw->cs & ADC_CS_READY_BITS)) { // bitwise and to test if the READY bit is set
        tight_loop_contents(); 
    }
    return (uint16_t)adc_hw->result;   // 12-bit result 
}

void init_adc_freerun() {
    // fill in
    adc_init();                          // powers up ADC 
    adc_gpio_init(ADC_GPIO);             // prepare GPIO45 for analog
    adc_select_input(ADC_CH);            // CS.AINSEL = 5
    adc_set_round_robin(0);              // single channel only 
    adc_set_clkdiv(0.0f);                // DIV = 0 -> back-to-back conversions
    hw_set_bits(&adc_hw->cs, ADC_CS_START_MANY_BITS);  // start running
}

void init_dma() {
    // fill in
    // Channel 0: stop it first 
    dma_hw->ch[0].ctrl_trig = 0;

    // Read from ADC FIFO, write to variable
    dma_hw->ch[0].read_addr  = (uint32_t)&adc_hw->fifo;
    dma_hw->ch[0].write_addr = (uint32_t)&adc_fifo_out;

    // TRANS_COUNT = MODE | COUNT
    // MODE = 1 (retrigger when COUNT hits 0)
    // COUNT = 1 (move one packet per trigger, packet size set in CTRL_TRIG.DATA_SIZE)
    dma_hw->ch[0].transfer_count = (1u << DMA_CH0_TRANS_COUNT_MODE_LSB) | (1u << DMA_CH0_TRANS_COUNT_COUNT_LSB);
}

void init_adc_dma() {
    // fill in
    // Program DMA channel (addresses, counts) but keep it disabled
    init_dma();

    // Configure ADC for free-running on CH5 (GPIO45)
    init_adc_freerun();

    // Enable FIFO and DREQ from ADC:
    //    - FCS.EN:       enable FIFO
    //    - FCS.DREQ_EN:  assert DMA request when FIFO has data
    //    - FCS.THRESH=1: DREQ when at least 1 sample is present
    adc_hw->fcs = 0;  // clear to a known state (optional)
    adc_hw->fcs = ADC_FCS_EN_BITS | ADC_FCS_DREQ_EN_BITS
                | (1u << ADC_FCS_THRESH_LSB);

    // Build CTRL_TRIG for DMA CH0 in a temp word, then write once:
    //    - DATA_SIZE = 1 (halfword = 16-bit; ADC is 12-bit so halfword fits)
    //    - TREQ_SEL = DREQ_ADC (pace transfers by ADC's DREQ)
    //    - INCR_READ = 0 (reading a fixed register)
    //    - INCR_WRITE = 0 (writing to the same scalar variable)
    //    - EN = 1 (enable channel)
    uint32_t temp = 0;
    temp |= (1u << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB);                 // halfword
    temp |= ((uint32_t)DREQ_ADC << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB);  // ADC DREQ
    temp |= DMA_CH0_CTRL_TRIG_EN_BITS;                               // enable

    
    dma_hw->ch[0].ctrl_trig = temp; // Write the control word to start the DMA channel 
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
    // autotest();

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
