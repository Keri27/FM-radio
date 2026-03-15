#include "debounce.h"
#include "timer.h"

#include <avr/interrupt.h>
#include <stdint.h>

#define SAMPLE_TRESHOLD 10

volatile uint8_t debounceTimer = 0;

/* Debounce function for both edges - returns new (debounced) value after X stable states of button */
uint8_t Debounce(uint8_t currentSample)
{
    static uint8_t oldDebounce = 0;
    static uint8_t lastSample = 0;
    static uint8_t debounceCount = 0;

    // If timer overflowed or first cycle (PCINT)
    if (debounceTimer == 1) 
    {
        debounceTimer = 0;

        if (currentSample == lastSample)
        {       
            debounceCount++;

            // DEBOUNCE_TRESHOLD x 4 ms = x ms of stable state
            if (debounceCount >= SAMPLE_TRESHOLD)
            {
                oldDebounce = currentSample; // Update oldDebounce
                debounceCount = 0;

                tim0_stop();
                tim0_ovf_disable();
            }
        }
        else // sample changed
        {
            debounceCount = 1;
        }

        lastSample = currentSample;
    }

    return oldDebounce; // Return (old) debounced button state
}

/* Interrupt service routine TIMER0 overflow */
ISR(TIMER0_OVF_vect)
{
    debounceTimer = 1;
}