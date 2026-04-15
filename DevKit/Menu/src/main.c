#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <avr/pgmspace.h>

#include "gpio.h"
#include "debounce.h"
#include "timer.h"
#include "Si4703.h"
#include "DEP128064C1_TWI.h"

#define LED PD6

/* For clarity (not meant to be changed)*/
#define SEEK_IDX 0
#define UP_IDX 1
#define DOWN_IDX 2
#define MENU_IDX 3

uint8_t change = 0;
uint8_t bttn_released = 1;
uint8_t screen = 0; // carries index of the current screen - "0" belongs to the FM radio
uint8_t volume = 8; // volume 0-15 step 2?
uint8_t output = 0; // audio output: speaker (default) or headphones
uint8_t brightness = 150; // brightness of the OLED <0, 100> %; step: 10 %

float actFreq;

int main(void)
{
  gpio_mode_input_pullup(&DDRD, UP);
  gpio_mode_input_pullup(&DDRD, DOWN);
  gpio_mode_input_pullup(&DDRD, SEEK);
  gpio_mode_input_pullup(&DDRD, MENU);
  gpio_mode_output(&DDRD, LED);

  oldD = PIND; // update current state of port D

  /* Enable PCINT2 */
  PCICR |= (1 << PCIE2);

  /* Enable interrupts on PD2 */
  PCMSK2 |= (1 << PCINT19) | (1 << PCINT20) | (1 << PCINT21) | (1 << PCINT22);

  /* Enable global interrupts */
  sei();

  /* 3-wire SPI constructor */
  u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_hw_i2c_avr, u8x8_gpio_and_delay_avr); // U8G2_R2

  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);  // switch off power save mode
  u8g2_SetContrast(&u8g2, brightness); // <0; 255>
  u8g2_ClearDisplay(&u8g2);

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
    // If timer overflowed or first cycle (PCINT)
    if (debounceTimer == 1)
    {
      Debounce(&buttons[bttn_idx], Sample(bttn_idx));

      // If any button pressed, debounce function finished and bttns have been released (last condition breaks the loop)
      if ((buttons[bttn_idx].stableState == 1) && (buttons[bttn_idx].debounceCount == 0) && bttn_released)
      {
        bttn_released = 0;
        change = 1; // enable change (freq, RDS, menu ...)

        if ((buttons[MENU_IDX].stableState == 1))
        {
          if (screen == 3)
          {
            screen = 0;
          }
          else
          {
            screen++;
          }
        }
      }

      // If buttons released and debounce function finished
      if ((buttons[bttn_idx].stableState == 0) && (buttons[bttn_idx].debounceCount == 0))
      {
        debounceReady = 1;
        bttn_released = 1;
      }
    }

    if (change == 1) 
    {
      change = 0;

      /* Screen 0: FM radio (default) */
      if (screen == 0) 
      {
        if (buttons[SEEK_IDX].stableState == 1)
        {
          if (SI4703_SeekUp())
          {
            actFreq = SI4703_GetFreq();
            display_updateFreq(actFreq);
          }
          else
          { // sometime seek runs out of time (timeout), but still manages to find the station -> not actual freq
            SI4703_SeekClear();
            actFreq = SI4703_GetFreq();
            display_seekFail(actFreq);
          }
        }
        else if (buttons[UP_IDX].stableState == 1) 
        {
          actFreq += 0.1;
          SI4703_SetFreq(actFreq);
          display_updateFreq(actFreq);
        }
        else if (buttons[DOWN_IDX].stableState == 1) 
        {
          actFreq -= 0.1;
          SI4703_SetFreq(actFreq);
          display_updateFreq(actFreq);
        }
        else
        {
          // get freq<
          display_updateFreq(actFreq);
        }
      }
      /* Screen 1: Volume settings */
      else if (screen == 1)
      {


        display_changeVolume(volume);
      }
      /* Screen 2: Audio output */
      else if (screen == 2)
      {


        display_changeAudioOutput(output);
      }
      /* Screen 3: OLED brightness */
      else if (screen == 3)
      {


        display_changeBrightness(brightness);
      }
    }
  }
}