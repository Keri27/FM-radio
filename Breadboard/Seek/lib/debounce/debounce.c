#include "debounce.h"
#include "timer.h"

#include <avr/interrupt.h>
#include <stdint.h>

#define SAMPLE_TRESHOLD 5

volatile uint8_t debounceTimer = 0;
volatile uint8_t debounceReady = 1;

volatile uint8_t newD;
volatile uint8_t oldD;
volatile uint8_t bttn_idx = 0; // init value doesnt matter

Button_t buttons[3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}; // Array of 3 structures

/* Sample function detects egde that caused the PCINT interrupt */
uint8_t Sample(uint8_t bttn_idx)
{
    newD = PIND;
    uint8_t edgeDetected = 0;
    
    // SEEK changed (PCINT) - rising or falling edge
    if (bttn_idx == 0)
    {
        // falling edge detection (pull-up -> active low)  1 \___ 0
        if ((newD & (1 << SEEK)) == 0)
        {
            edgeDetected = 1;
        }
        else
        {
            edgeDetected = 0;
        }
    }
    // UP changed
    else if (bttn_idx == 1)
    {
        if ((newD & (1 << UP)) == 0)
        {
            edgeDetected = 1;
        }
        else
        {
            edgeDetected = 0;
        }
    }
    // DOWN changed
    else if (bttn_idx == 2)
    {
        if ((newD & (1 << DOWN)) == 0)
        {
            edgeDetected = 1;
        }
        else
        {
            edgeDetected = 0;
        }
    }

    return edgeDetected;
}

/* Debounce function for both edges - returns new (debounced) value after X stable states of button */
void Debounce(Button_t *btn, uint8_t currentSample)
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
            debounceReady = 1;

            tim0_stop();           
            tim0_ovf_disable();
        }
    }
    else // Sample changed
    {
        btn->debounceCount = 1;
    }

    btn->lastSample = currentSample;
}