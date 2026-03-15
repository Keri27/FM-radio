#include "debounce.h"
#include "timer.h"

#include <avr/interrupt.h>
#include <stdint.h>

#define DEBOUNCE_TRESHOLD 10

volatile uint8_t debounceTimer = 0;

/* Debounce function for both edges - returns new (debounced) value after X stable states of button */
uint8_t Debounce(uint8_t pressedButton)
{
    static uint8_t oldDebounce = 0;
    static uint8_t debounceCount = 0;

    // If timer overflowed or first cycle (PCINT)
    if (debounceTimer == 1) 
    {
        debounceTimer = 0;

        // If button state changed
        if (pressedButton != oldDebounce)
        {
            // Start counting if not already started (First cycle)
            if (debounceCount == 0)
            {
                debounceCount = 1;

                TCNT0 = 0; // Reset timer0
                tim0_ovf_4ms();
                tim0_ovf_enable();
            }
            else
            {
                debounceCount++;

                // DEBOUNCE_TRESHOLD x 4 ms = x ms of stable state
                if (debounceCount >= DEBOUNCE_TRESHOLD)
                {
                    oldDebounce = pressedButton; // Update oldDebounce
                    debounceCount = 0;

                    tim0_stop();
                    tim0_ovf_disable();
                }
            }
        }
        else // If pressedButton changed, reset debounceCounter and start again
        {
            debounceCount = 0;

            tim0_stop();
            tim0_ovf_disable();
        }
    }

    return oldDebounce; // Return (old) debounced button state
}

/* Interrupt service routine TIMER0 overflow */
ISR(TIMER0_OVF_vect)
{
    debounceTimer = 1;
}