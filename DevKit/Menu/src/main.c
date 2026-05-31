#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdint.h>
#include <util/delay.h>
#include <string.h>
#include <avr/eeprom.h>

#include "gpio.h"
#include "debounce.h"
#include "timer.h"
#include "Si4703.h"
#include "DEP128064C1_TWI.h"
#include "lipol.h"

#define LED PD7
#define SD1 PB2   // Speaker shutdown (TPA741)
#define SD2 PC1   // Headphones shutdown (TPA6111)
#define GPIO2 PD2 // RDS/STC interrupt

/* For clarity (do not change)*/
#define SEEK_IDX 0
#define UP_IDX 1
#define DOWN_IDX 2
#define MENU_IDX 3

#define TIM0_WAIT_OVF_NUM 15
#define TIM0_FREQ_OVF_NUM 8

volatile uint8_t change = 0;
volatile uint8_t tim0Cycles = 0;
volatile uint8_t tim1Cycles = 0;
volatile uint8_t longPress = 0;
volatile uint8_t fastFreqChange = 0;
volatile uint8_t CTburst = 0; // RDS clock time burst window opened/closed
volatile uint8_t groupType = 99; // RDS group type
volatile uint8_t seekRequest = 0; // Prevents multiple seek event
volatile uint8_t gpio2 = 0;

const char* channelName = "loading";
uint8_t newRDSgroup = 0;
uint8_t firstCTgroup = 1;
uint8_t actHour = 99; // serves for identification of first minute
uint8_t actMinute = 99;

uint8_t screen = 0; // holds index of the current screen - "0" belongs to the FM radio
uint8_t rssi = 0;
uint8_t stereo = 0;
uint8_t seekFail = 0;

uint8_t volume;           // volume: 0, 1, 3, 5, ..., 15
uint8_t output;           // audio output: speaker (default) or headphones
uint8_t brightness = 150; // brightness of the OLED <0, 100> %; step: 10 %
uint8_t battery;          // battery perctentage <0, 100> %; step: 10 %

/* EEPROM (holds stored values after powerdown) */
uint8_t ee_volume EEMEM;
uint8_t ee_output EEMEM;

float actFreq;

int main(void)
{
  gpio_mode_input_pullup(&DDRD, UP);
  gpio_mode_input_pullup(&DDRD, DOWN);
  gpio_mode_input_pullup(&DDRD, SEEK);
  gpio_mode_input_pullup(&DDRD, MENU);
  gpio_mode_input_pullup(&DDRD, GPIO2);
  gpio_mode_output(&DDRD, LED);
  gpio_mode_output(&DDRB, SD1);
  gpio_mode_output(&DDRC, SD2);

  gpio_write_low(&PORTD, LED);
  gpio_write_high(&PORTB, SD1);
  gpio_write_high(&PORTC, SD2);

  oldD = PIND; // update current state of port D

  /* Enable PCINT2 */
  PCICR |= (1 << PCIE2);

  /* Enable interrupts on PD2 */
  PCMSK2 |= (1 << PCINT18 /* GPIO2 */) | (1 << PCINT19) | (1 << PCINT20) | (1 << PCINT21) | (1 << PCINT22);

  /* Enable global interrupts */
  sei();

  /* OLED init */
  u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_hw_i2c_avr, u8x8_gpio_and_delay_avr); // I2C constructor

  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);         // switch off power save mode
  u8g2_SetContrast(&u8g2, brightness); // <0; 255>
  u8g2_ClearDisplay(&u8g2);

  ADC_Init();

  SI4703_Init();

  /* Loading values from EEPROM */
  output = eeprom_read_byte(&ee_output); // load value from EEPROM

  if (output > 1) output = 0; // in case the stored value is out of range (0,1): enable reproductor

  if (!output)
  {
    gpio_write_high(&PORTC, SD2);
    gpio_write_low(&PORTB, SD1); // enable speaker (default)
    SI4703_SetMono(1);
  }
  else
  {
    gpio_write_high(&PORTB, SD1);
    gpio_write_low(&PORTC, SD2); // enable headphones
    SI4703_SetMono(0);
  }

  volume = eeprom_read_byte(&ee_volume);
  if (volume > 15) volume = 7;
  SI4703_SetVolume(volume);

  /* Seek sequence */
  if (!SI4703_SeekUp())
  {
    seekFail = 1;
    SI4703_SeekClear(); // If Seek fails due to SFBL or I2C failure, however seek_clear is needed only for I2C failure
  }

  SI4703_ResetPS();

  actFreq = SI4703_GetFreq();
  rssi = SI4703_GetRSSI();
  stereo = SI4703_GetStereo();
  battery = getBatteryPercentage(ADC_Read());

  display_updateChannel(actFreq, rssi, stereo, seekFail, battery, channelName, actHour, actMinute);

  /* Sleep mode used in this program: IDLE*/
  set_sleep_mode(SLEEP_MODE_IDLE);

  while (1)
  {
    /* Debounce: If timer overflowed or first cycle (PCINT) */
    if (debounceTimer == 1)
    {
      Debounce(&buttons[bttn_idx], Sample(bttn_idx));

      /*
      // If debounce function not finished: idle: wait for IRQ from timer 2
      if (buttons[bttn_idx].debounceCount != 0)
      {
        sleep_enable();
        sleep_cpu();

        sleep_disable(); // after ATmega328P quits IDLE (IRQ, buttons)
      }
      */

      // If any button pressed and debounce function finished
      if ((buttons[bttn_idx].stableState == 1) && (buttons[bttn_idx].debounceCount == 0))
      {
        change = 1; // enable change (freq, RDS, menu ...)

        /* Change screen */
        if ((buttons[MENU_IDX].stableState == 1))
        {
          if (screen == 3)
            screen = 0;
          else
            screen++;
        }
        /* Enter Fast frequency change mode */
        else if ((buttons[UP_IDX].stableState == 1) || ((buttons[DOWN_IDX].stableState == 1)))
        {
          TCNT0 = 0;
          tim0_ovf_33ms();
          tim0_ovf_enable();

          longPress = 1;
        }
        /* Prevent multiple seek change */
        else if ((buttons[SEEK_IDX].stableState == 1) && (screen == 0))
        {
          seekRequest = 1;
        }
      }

      // If any button released and debounce function finished
      if ((buttons[bttn_idx].stableState == 0) && (buttons[bttn_idx].debounceCount == 0))
      {
        debounceReady = 1;

        /* Stop long-press and fast frequency mode after release */
        if (longPress || fastFreqChange)
        {
          tim0_stop();
          tim0_ovf_disable();

          longPress = 0;
          fastFreqChange = 0;
          tim0Cycles = 0;
          change = 0;
        }
      }
      }

    /* User interface */
    if (change == 1)
    {
      change = 0;

      /* Screen 0: FM radio (default) */
      if (screen == 0)
      {
        /* SEEK station */
        if (seekRequest) // PS update few seconds after seek (RDS) used to reactivate seek fucntion when button pressed
        {
          seekRequest = 0;

          if (SI4703_SeekUp())
          {
            seekFail = 0;

            tim1Cycles = 0;
            SI4703_ResetPS();
            
            channelName = "loading";
          }
          else
          {
            seekFail = 1;
            SI4703_SeekClear();
          }

          actFreq = SI4703_GetFreq();
          rssi = SI4703_GetRSSI();
          stereo = SI4703_GetStereo();

          display_updateChannel(actFreq, rssi, stereo, seekFail, battery, channelName, actHour, actMinute);
        }
        /* Set frequency UP */
        else if (buttons[UP_IDX].stableState == 1)
        {
          SI4703_ResetPS();

          if (actFreq >= 108.0)
          {
            actFreq = 87.5;
          }
          else
          {
            actFreq += 0.1;
          }

          rssi = SI4703_GetRSSI();
          stereo = SI4703_GetStereo();
          seekFail = 0;
          channelName = "loading";

          SI4703_SetFreq(actFreq);
          display_updateChannel(actFreq, rssi, stereo, seekFail, battery, channelName, actHour, actMinute);
        }
        /* Set frequency DOWN */
        else if (buttons[DOWN_IDX].stableState == 1)
        {
          SI4703_ResetPS();

          if (actFreq <= 87.5)
          {
            actFreq = 108.0;
          }
          else
          {
            actFreq -= 0.1;
          }

          rssi = SI4703_GetRSSI();
          stereo = SI4703_GetStereo();
          seekFail = 0;
          channelName = "loading";

          SI4703_SetFreq(actFreq);
          display_updateChannel(actFreq, rssi, stereo, seekFail, battery, channelName, actHour, actMinute);
        }
        /* No user action */
        else
        {
          // Update info: Battery, RDS... every half a minute or 8 s if seek found staion
          battery = getBatteryPercentage(ADC_Read());

          // SI4703_RxRegs(); // included in GetFreq
          actFreq = SI4703_GetFreq(); // AFC may change freq -> keep freq up-to-date
          rssi = SI4703_GetRSSI();
          stereo = SI4703_GetStereo();

          display_updateChannel(actFreq, rssi, stereo, seekFail, battery, channelName, actHour, actMinute);
        }
      }
      /* Screen 1: Volume settings */
      else if (screen == 1)
      {
        /* Increase volume */
        if (buttons[UP_IDX].stableState == 1)
        {
          if (volume <= 13)
          {
            if (volume)
              volume += 2; // if mute
            else
              volume = 1;
            SI4703_SetVolume(volume);
            eeprom_update_byte(&ee_volume, volume); // save value to EEPROM
          }
          display_changeVolume(volume);
        }
        /* Decrease volume */
        else if (buttons[DOWN_IDX].stableState == 1)
        {
          if (volume > 1)
          {
            volume -= 2;
            SI4703_SetVolume(volume);
            eeprom_update_byte(&ee_volume, volume);
          }
          else if (volume == 1)
          {
            volume = 0; // mute
            SI4703_SetVolume(volume);
            eeprom_update_byte(&ee_volume, volume);
          }
          display_changeVolume(volume);
        }
        else
        {
          display_changeVolume(volume);
        }
      }
      /* Screen 2: Audio output */
      else if (screen == 2)
      {
        /* Switch to headphones */
        if (buttons[DOWN_IDX].stableState == 1)
        {
          gpio_write_high(&PORTB, SD1);
          gpio_write_low(&PORTC, SD2);
          SI4703_SetVolume(2); // safe volume
          SI4703_SetMono(0); // switch to stereo (depends on quality of the received signal)
          display_changeAudioOutput(output = 1);
          eeprom_update_byte(&ee_output, output);
        }
        /* Switch to speaker */
        else if (buttons[UP_IDX].stableState == 1)
        {
          gpio_write_high(&PORTC, SD2);
          gpio_write_low(&PORTB, SD1);
          SI4703_SetMono(1);
          display_changeAudioOutput(output = 0);
          eeprom_update_byte(&ee_output, output);
        }
        else
        {
          display_changeAudioOutput(output);
        }
      }
      /* Screen 3: OLED brightness */
      else if (screen == 3)
      {
        /* Decrease brightness */
        if (buttons[DOWN_IDX].stableState == 1)
        {
          if (brightness >= 50)
          {
            brightness -= 25;
            u8g2_SetContrast(&u8g2, brightness);
            display_changeBrightness(brightness);
          }
        }
        /* Increase brightness */
        else if (buttons[UP_IDX].stableState == 1)
        {
          if (brightness <= 225)
          {
            brightness += 25;
            u8g2_SetContrast(&u8g2, brightness);
            display_changeBrightness(brightness);
          }
        }
        else
        {
          display_changeBrightness(brightness);
        }
      }
    }

    /* GPIO2 */
    if (gpio2)
    {
      gpio2 = 0;

      if (SI4703_CheckRDSReady())
      {
        newRDSgroup = 1;
      }

      // seek = 0;
      // SI4703_SeekClear();
    }

    /* RDS processing */
    if (newRDSgroup)
    {
      newRDSgroup = 0;
      groupType = SI4703_RDSGetGroupType(); // Get RDS group type

      if (groupType == 0) 
      {
        if (!validPS)  // if channelName not valid (None or loading)
        {
          channelName = SI4703_RDSProgrammeService();  // Get name of the tuned station

          if (validPS) change = 1;
        }
      }
      else if (groupType == 8)  // 4 = CT group
      {       
        /* Set timer (watchdog): if we dont get CT or PS in 6,3 s since we received first CT group -> wait for another CT burst */
        if (firstCTgroup)
        {
          firstCTgroup = 0;

          CTburst = 1;
          if (strcmp(channelName, "none") == 0 || strcmp(channelName, "loading") == 0)
          {
            validPS = 0;
          }

          TCNT1 = 0;
          tim1_ovf_2s();
          tim1_ovf_enable();
          tim1Cycles = 0;
        }

        /* Get CT and update dipslay */
        if (SI4703_RDSClockTime(&actHour, &actMinute)) 
        {
          CTburst = 0;
          change = 1;
        }
      }

      groupType = 99; // Reset group type
    }

    /* Sleep mode entry (IDLE) */
    if (!CTburst && validPS && !change) // if no change available
    {
      /* Reset RDS watchdog */
      tim1_ovf_disable();
      tim1_stop();
      tim1Cycles = 0;

      //gpio_write_high(&PORTD, LED);
      sleep_enable();
      sleep_cpu();

      //gpio_write_low(&PORTD, LED);
      sleep_disable(); //after ATmega328P quits IDLE (IRQ, buttons)
    }
  }
}

/* Interrupt service routine TIMER0 overflow */
ISR(TIMER0_OVF_vect)
{
  /* Fast freq change mode: TIM0_FREQ_OVF_NUM x 33 ms = (198 ms) */
  if (fastFreqChange)
  {
    if (tim0Cycles >= TIM0_FREQ_OVF_NUM)
    {
      tim0Cycles = 0;

      change = 1; // -> setFrequency()
    }
  }
  /* Longpress detection: TIM0_WAIT_OVF_NUM x 33 ms = (495 ms + 198 ms) */
  else if (longPress)
  {
    if (tim0Cycles >= TIM0_WAIT_OVF_NUM)
    {
      tim0Cycles = 0;

      longPress = 0;
      fastFreqChange = 1;
    }
  }

  tim0Cycles++;
}

/* Interrupt service routine TIMER1 overflow */
ISR(TIMER1_OVF_vect)
{
  tim1Cycles++;

  /* RDS watchdog: CT or PS not detected -> wait aporximately 60s for another CT burst*/
  if ((CTburst || !validPS) && (tim1Cycles >= 3)) // 6,3s
  {
    firstCTgroup = 1;
    CTburst = 0;

    if (!validPS) // If channelName == none -> display Spatny signal
    {
      channelName = "none"; // In case no RDS group due to bad signal -> BLERB more than 0 in SI4703_RDSGetGroupType()
      validPS = 1;
      change = 1; 
    }

    tim1_ovf_disable();
    tim1_stop();
    tim1Cycles = 0;
  }
  /* Seek watchdog: if STC not set for 2s -> seekFail */
  /*
    else if (seek && (tim1Cycles >= 1))
    {
      // shouldnt control tim1 (tim1_ovf_disable, tim1Cycles = 0 ...)
    }
  */
}

/* Interrupt service routine TIMER2 overflow */
ISR(TIMER2_OVF_vect)
{
  debounceTimer = 1;
}

/* Interrupt service routine PORTD */
ISR(PCINT2_vect)
{
  newD = PIND; // update current state of port D

  // GPIO2 changed
  if ((newD ^ oldD) & (1 << GPIO2))
  {
    // GPIO falling edge (active low)  1 \___ 0
    if ((newD & (1 << GPIO2)) == 0)
    {
      gpio2 = 1;
    }

    oldD = newD;
    return;
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
    // MENU changed
    else if ((newD ^ oldD) & (1 << MENU))
    {
      bttn_idx = 3;
    }

    debounceReady = 0;
    debounceTimer = 1;
    TCNT2 = 0;
    tim2_ovf_4ms();
    tim2_ovf_enable();

    oldD = newD;
  }
}