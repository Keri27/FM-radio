#include <avr/io.h>
#include <avr/interrupt.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>

#include "gpio.h"
#include "timer.h"
#include "debounce.h"

#define UP PD2
#define DOWN PD3
#define SEEK PD4
#define LED PD6

#define SEEK_IDX 0
#define UP_IDX 1
#define DOWN_IDX 2

uint8_t changeColor = 1;

int main(void)
{
  gpio_mode_input_pullup(&DDRD, UP);
  gpio_mode_input_pullup(&DDRD, DOWN);
  gpio_mode_input_pullup(&DDRD, SEEK);
  gpio_mode_output(&DDRD, LED);

  oldD = PIND; // update current state of port D

  /* Enable PCINT2 */
  PCICR |= (1 << PCIE2);

  /* Enable interrupts on PD2 */
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19) | (1 << PCINT20);

  /* Enable global interrupts */
  sei();

  while (1)
  {
    // If timer overflowed or first cycle (PCINT)
    if (debounceTimer == 1)
    {
      Debounce(&buttons[bttn_idx], Sample(bttn_idx));

      // If buttons released and debounce function is not counting
      if ((buttons[bttn_idx].stableState == 0) && (buttons[bttn_idx].debounceCount == 0))
      {
        debounceReady = 1;
        changeColor = 1;
      }
    }

    if ((buttons[SEEK_IDX].stableState == 1) && (changeColor))
    {
      gpio_toggle(&PORTD, LED);
      changeColor = 0;
    }
    else if ((buttons[UP_IDX].stableState == 1) && (changeColor))
    {
      gpio_toggle(&PORTD, LED);
      changeColor = 0;
    }
    else if ((buttons[DOWN_IDX].stableState == 1) && (changeColor))
    {
      gpio_toggle(&PORTD, LED);
      changeColor = 0;
    }
  }
}