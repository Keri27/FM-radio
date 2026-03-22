/*
 * main.c
 *
 * PROJECT: Miniature FM radio in a matchbox-sized enclosure.
 * AUTHOR: Adam Keřka
 * YEAR: 2026
 *
 * REFERENCE
 * GPIO and Timer library: https://github.com/tomas-fryza/avr-examples
 * Si4703 library: https://github.com/eziya/AVR_SI4703/tree/master
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>

#include "gpio.h"
#include "timer.h"
#include "debounce.h"

#define LED PD6

/* For clarity (not meant to be changed) */
#define SEEK_IDX 0
#define UP_IDX 1
#define DOWN_IDX 2

#define OVF_NUM 5 // Number of ovfs for 66 ms; determines speed of the auto frequency change

uint8_t changeColor = 1;
uint8_t longPress = 0;

volatile uint8_t timer1Cycles =  0;

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
    /* If timer overflowed or first cycle (PCINT) */
    if (debounceTimer == 1)
    {
      Debounce(&buttons[bttn_idx], Sample(bttn_idx));

      // If button is still pressed and debounce function finished -> long press
      if ((buttons[bttn_idx].stableState == 1) && (buttons[bttn_idx].debounceCount == 0))
      {
        TCNT1 = 0;
        tim1_ovf_524ms();
        tim1_ovf_enable();

        longPress = 1;
      }

      // If button released and debounce function finished
      if ((buttons[bttn_idx].stableState == 0) && (buttons[bttn_idx].debounceCount == 0))
      {
        debounceReady = 1;
        changeColor = 1;

        // If used, leave from frequency change mode
        if (longPress)      
        {
        tim2_stop();
        tim2_ovf_disable();

        longPress = 0;
        timer1Cycles = 0;
        }
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

/* Interrupt service routine TIMER1 overflow */
ISR(TIMER1_OVF_vect)
{
  // Time of 1 Cycle: OVF_NUM (5) * 66ms = 330 ms
  if (timer1Cycles >= OVF_NUM)
  {
    timer1Cycles = 0;      
    changeColor = 1;
  }

  timer1Cycles++;
  tim1_ovf_66ms();
}