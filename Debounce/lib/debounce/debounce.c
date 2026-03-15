#include "debounce.h"
#include "timer.h"

#include <avr/interrupt.h>
#include <stdint.h>

#define SAMPLE_TRESHOLD 10

volatile uint8_t debounceTimer = 0;

Button_t buttons[3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}; // Array of 3 structures

/* Debounce function for both edges - returns new (debounced) value after X stable states of button */
uint8_t Debounce(Button_t *btn, uint8_t currentSample)
{

    // If timer overflowed or first cycle (PCINT)
    if (debounceTimer == 1) 
    {
        debounceTimer = 0;

        if (currentSample == btn->lastSample)
        {       
            btn->debounceCount++;

            // DEBOUNCE_TRESHOLD x 4 ms = x ms of stable state
            if (btn->debounceCount >= SAMPLE_TRESHOLD)
            {
                btn->stableState = currentSample; // Update stableState
                btn->debounceCount = 0;

                tim0_stop();
                tim0_ovf_disable();
            }
        }
        else // sample changed
        {
            btn->debounceCount = 1;
        }

        btn->lastSample = currentSample;
    }

    return btn->stableState; // Return (old) debounced button state
}

/* Interrupt service routine TIMER0 overflow */
ISR(TIMER0_OVF_vect)
{
    debounceTimer = 1;
}