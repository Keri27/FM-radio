#include <avr/io.h>
#include <avr/interrupt.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>
//#include <util/delay.h>

#include "gpio.h"
#include "timer.h"
#include "debounce.h"

#define LED PD6
#define UP PD2

uint8_t buttonPD2isPressed = 0;
volatile uint8_t nonDebouncedPD2Pressed = 0;

uint8_t changeColor = 1;

//uint8_t buttonReleased = 0;

volatile uint8_t oldD;

int main(void)
{
  gpio_mode_input_pullup(&DDRD, UP);
  gpio_mode_output(&DDRD, LED);

  oldD = PIND; // update current state of port D

  /* Enable PCINT2 */
  PCICR |= (1 << PCIE2);

  /* Enable interrupts on PD2 */
  PCMSK2 |= (1 << PCINT18);

  /* Enable global interrupts */
  sei();

  while (1)
  {
    buttonPD2isPressed = Debounce(nonDebouncedPD2Pressed);

    if (buttonPD2isPressed && changeColor)
    {
      gpio_toggle(&PORTD, LED);
      changeColor = 0;
    }
    /* Button released */
    else if (!buttonPD2isPressed && !changeColor)
    {
      //buttonReleased = 0; // Force "0" (after buttonReleased run this only once)
      changeColor = 1;
    }
  }
}

/* Interrupt service routine PORTD */
ISR(PCINT2_vect)
{
  uint8_t newD = PIND; // update current state of port D
  debounceTimer = 1;

  // PD2 (PCINT18) pressed
  if ((newD & (1 << PD2)) == 0 &&
      (oldD & (1 << PD2)) != 0)
  { // falling edge detection (pull-up)  1 \___ 0
    nonDebouncedPD2Pressed = 1;
  }
  else // PDx release - if any button released reset everything - rising edge detection 0 ___/ 1
  {
    nonDebouncedPD2Pressed = 0;
    //buttonReleased = 1;
  }

  oldD = newD;
}