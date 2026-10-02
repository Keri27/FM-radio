#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>

#define UP PD2   // PD2 (breadboard)
#define DOWN PD3 // PD3
#define SEEK PD4 // PD4

extern volatile uint8_t debounceTimer;
extern volatile uint8_t debounceReady;

extern volatile uint8_t newD;
extern volatile uint8_t oldD;
extern volatile uint8_t bttn_idx;

typedef struct
{
    uint8_t lastSample;
    uint8_t debounceCount;
    uint8_t stableState;
} Button_t;

extern Button_t buttons[3];

uint8_t Sample(uint8_t newD);

/*Debounce function for both edges - returns new (debounced) value after X stable states of button*/
void Debounce(Button_t *btn, uint8_t currentSample);

#endif