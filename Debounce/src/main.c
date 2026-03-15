#include <avr/io.h>
#include <avr/interrupt.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>

#include "gpio.h"
#include "timer.h"
#include "debounce.h"

#define LED PD6
#define UP PD2
#define DOWN PD3
#define SEEK PD4

uint8_t PD2Pressed = 0;
uint8_t PD3Pressed = 0;
uint8_t PD4Pressed = 0;
volatile uint8_t PD2Sample = 0;
volatile uint8_t PD3Sample = 0;
volatile uint8_t PD4Sample = 0;

uint8_t changeColor = 1;

volatile uint8_t oldD;

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
    PD2Pressed = Debounce(&buttons[bttn_idx], PD2Sample);

    if (PD2Pressed && changeColor)
    {
      gpio_toggle(&PORTD, LED);
      changeColor = 0;
    }
    /* Button released */
    else if (!PD2Pressed && !changeColor)
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

  TCNT0 = 0; // Reset timer0
  tim0_ovf_4ms();
  tim0_ovf_enable();
  debounceTimer = 1;

  // PD2 (PCINT18) pressed
  if ((newD & (1 << PD2)) == 0 &&
      (oldD & (1 << PD2)) != 0)
  { // falling edge detection (pull-up)  1 \___ 0
    PD2Sample = 1;
  }
  else // PDx release - if any button released reset everything - rising edge detection 0 ___/ 1
  {
    PD2Sample = 0;
  }

  oldD = newD;
}