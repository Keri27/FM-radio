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

Button_t buttons[4] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}; // Array of 3 structures

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
    // MENU changed
    else if (bttn_idx == 3)
    {
        if ((newD & (1 << MENU)) == 0)
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

/* Interrupt service routine PORTD */
ISR(PCINT2_vect)
{
    if (debounceReady)
    {
        newD = PIND; // update current state of port D

        // SEEK changed (PCINT) - rising or falling edge
        if ((newD ^ oldD) & (1 << SEEK))
        {
            bttn_idx = 0;
        }
        // UP changed
        else if ((newD ^ oldD) & (1 << UP))
        {
            bttn_idx = 1;
        }
        // DOWN changed
        else if ((newD ^ oldD) & (1 << DOWN))
        {
            bttn_idx = 2;
        }
        // MENU changed
        else if ((newD ^ oldD) & (1 << MENU))
        {
            bttn_idx = 3;
        }

        debounceReady = 0;
        debounceTimer = 1;
        TCNT0 = 0;
        tim0_ovf_4ms();
        tim0_ovf_enable();
    }

    oldD = newD;
}

/* Interrupt service routine TIMER0 overflow */
ISR(TIMER0_OVF_vect)
{
    debounceTimer = 1;
}