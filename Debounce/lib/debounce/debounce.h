#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>

extern volatile uint8_t debounceTimer;
extern Button_t buttons[3];

typedef struct
{
    uint8_t lastSample;
    uint8_t debounceCount;
    uint8_t stableState;
} Button_t;

/*Debounce function for both edges - returns new (debounced) value after X stable states of button*/
uint8_t Debounce(Button_t *btn, uint8_t currentSample);

#endif