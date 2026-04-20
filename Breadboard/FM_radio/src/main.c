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
 * OLED library:
 * 
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "gpio.h"
#include "timer.h"
#include "debounce.h"
#include "Si4703.h"
#include "DEP128064C1.h"

//#define GPIO2 PD7 // PD7 (breadboard)
#define LED PD6   // PD6

/* For clarity (not meant to be changed) */
#define SEEK_IDX 0
#define UP_IDX 1
#define DOWN_IDX 2

#define OVF_NUM 5 // Number of ovfs for 66 ms; determines speed of the auto frequency change

uint8_t changeFreq = 1;
uint8_t longPress = 0;

float actFreq;
volatile uint8_t timer1Cycles =  0;
volatile uint8_t gpio2 = 0; // STC and RDS interrupt flag

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

  /* 3-wire SPI constructor */
  u8g2_Setup_ssd1306_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_3wire_sw_spi, gpio_cb);

  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);  // switch off power save mode
  u8g2_SetContrast(&u8g2, 150); // <0; 255>

  SI4703_Init();

  if (SI4703_SeekUp())
  {
    actFreq = SI4703_GetFreq();
    display_updateFreq(actFreq);
  }
  else
  {
    SI4703_SeekClear();
    actFreq = SI4703_GetFreq();
    display_seekFail(actFreq);
  }

  while (1)
  {
    /* If timer overflowed or first cycle (PCINT) */
    if (debounceTimer == 1)
    {
      Debounce(&buttons[bttn_idx], Sample(bttn_idx));

      // If button is still pressed and debounce function finished -> long press
      if (((buttons[UP_IDX].stableState == 1) && (buttons[UP_IDX].debounceCount == 0)) ||
          ((buttons[DOWN_IDX].stableState == 1) && (buttons[DOWN_IDX].debounceCount == 0)))
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
        changeFreq = 1;

        // If used, leave from frequency change mode
        if (longPress)      
        {
        tim1_stop();
        tim1_ovf_disable();

        longPress = 0;
        timer1Cycles = 0;
        }
      }
    }
    /* Seek up relevant station (treshold: RSSI = , SNR = ) */
    if ((buttons[SEEK_IDX].stableState == 1) && (changeFreq))
    {
      if (SI4703_SeekUp())
      {
        actFreq = SI4703_GetFreq();
        display_updateFreq(actFreq);
      }
      else
      {
        SI4703_SeekClear();
        actFreq = SI4703_GetFreq();
        display_seekFail(actFreq);
      }

      changeFreq = 0;
    }
    /* Step up frequency for 0.1 MHz */
    else if ((buttons[UP_IDX].stableState == 1) && (changeFreq))
    {
      actFreq += 0.1;
      SI4703_SetFreq(actFreq);
      //display_updateFreq(actFreq);
      changeFreq = 0;
    }
    /* Step down frequency for 0.1 MHz */
    else if ((buttons[DOWN_IDX].stableState == 1) && (changeFreq))
    {
      actFreq -= 0.1;
      SI4703_SetFreq(actFreq);
      //display_updateFreq(actFreq);
      changeFreq = 0;
    }
  
  }
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

    debounceReady = 0;
    debounceTimer = 1;
    TCNT2 = 0;
    tim2_ovf_4ms();
    tim2_ovf_enable();

    oldD = newD;
  }
  /*
  // GPIO falling edge (active low)  1 \___ 0
  if ((newD & (1 << GPIO2)) == 0 && (oldD & (1 << GPIO2)) == 1)
  {
    gpio2 = 1;
  }
  */
}

/* Interrupt service routine TIMER1 overflow */
ISR(TIMER1_OVF_vect)
{
  // Time of 1 Cycle: OVF_NUM (5) * 66ms = 330 ms; Time of the first: 524m + (OVF_NUM -1) * 66m
  if (timer1Cycles >= OVF_NUM)
  {
    timer1Cycles = 0;      
    changeFreq = 1;
  }

  timer1Cycles++;
  tim1_ovf_66ms();
}

/* Interrupt service routine TIMER2 overflow */
ISR(TIMER2_OVF_vect)
{
  debounceTimer = 1;
}