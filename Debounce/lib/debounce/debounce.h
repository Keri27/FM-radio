#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>

extern volatile uint8_t debounceTimer;

/*Debounce function for both edges - returns new (debounced) value after X stable states of button*/
uint8_t Debounce(uint8_t pressedButton);

#endif