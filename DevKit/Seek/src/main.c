#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#include "gpio.h"
#include "timer.h"
#include "debounce.h"
#include "Si4703.h"

#define LED PD7
#define GPIO2 PD2
#define SD1 PB2 // Speaker shutdown (TPA741)
#define SD2 PC1 // Headphones shutdown (TPA6111)

/* For clarity (not meant to be changed)*/
#define SEEK_IDX 0
#define UP_IDX 1
#define DOWN_IDX 2

uint8_t change = 1;
volatile uint8_t seek = 1;
volatile uint8_t gpio2 = 0;
volatile uint8_t seekFail = 0;
volatile float actFreq;


int main(void)
{
  gpio_mode_input_pullup(&DDRD, UP);
  gpio_mode_input_pullup(&DDRD, DOWN);
  gpio_mode_input_pullup(&DDRD, SEEK);
  gpio_mode_input_pullup(&DDRD, GPIO2);

  gpio_mode_output(&DDRB, SD1);
  gpio_mode_output(&DDRC, SD2);
  gpio_mode_output(&DDRD, LED);

  gpio_write_low(&PORTD, LED);
  gpio_write_low(&PORTB, SD1); // enable speaker
  gpio_write_high(&PORTC, SD2);

  oldD = PIND; // update current state of port D

  /* Enable PCINT2 */
  PCICR |= (1 << PCIE2);

  /* Enable interrupts on PD2 */
  PCMSK2 |= (1 << PCINT18 /* GPIO2 */) | (1 << PCINT19) | (1 << PCINT20) | (1 << PCINT21);

  /* Enable global interrupts */
  sei();

  SI4703_Init();
  _delay_ms(100);
  if (SI4703_SeekUp()) seek = 1;

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
        change = 1;
      }
    }
    /* Seek */
    if ((buttons[SEEK_IDX].stableState == 1) && (change))
    {
      if (SI4703_SeekUp()) seek = 1;

      //gpio_toggle(&PORTD, LED);
      change = 0;
    }
    /* UP */
    else if ((buttons[UP_IDX].stableState == 1) && (change))
    {
      gpio_toggle(&PORTD, LED);
      change = 0;
    }
    /* DOWN */
    else if ((buttons[DOWN_IDX].stableState == 1) && (change))
    {
      gpio_toggle(&PORTD, LED);
      change = 0;
    }
    /* Seek done */
    else if (gpio2)
    {
      gpio2 = 0;
      seek = 0;
    
      SI4703_SeekClear();
      // actFreq = SI4703_getFreq();
    }
    /* Seek fail */
    else if (seekFail)
    {
      seekFail = 0;
      seek = 0;
      SI4703_SeekClear();
      // Error: Seek failed
    }
  }
}

/* Interrupt service routine PORTD */
ISR(PCINT2_vect)
{
  newD = PIND; // update current state of port D

  if (seek)
  {
    // GPIO falling edge (active low)  1 \___ 0
    if ((newD & (1 << GPIO2)) == 0 && (oldD & (1 << GPIO2)) != 0)
    {
      gpio2 = 1;
      gpio_toggle(&PORTD, LED);
    }
  }

  if (debounceReady)
  {
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

    debounceReady = 0;
    debounceTimer = 1;
    TCNT2 = 0;
    tim2_ovf_4ms();
    tim2_ovf_enable();
  }

  oldD = newD;
}

/* Interrupt service routine TIMER1 overflow */
ISR(TIMER1_OVF_vect)
{
  seekFail = 1;
}

/* Interrupt service routine TIMER2 overflow */
ISR(TIMER2_OVF_vect)
{
  debounceTimer = 1;
}