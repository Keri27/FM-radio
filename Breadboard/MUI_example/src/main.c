#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <avr/pgmspace.h>

#include "gpio.h"
#include "timer.h"
#include "debounce.h"
#include "DEP128064C1.h"
#include "u8g2.h"
#include "mui.h"
#include "mui_u8g2.h"

#define LED PD6

/* For clarity (not meant to be changed)*/
#define SEEK_IDX 0
#define UP_IDX 1
#define DOWN_IDX 2

uint8_t redraw = 0;
mui_t mui;

// FDS data - řetězec maker bez středníků mezi nimi
const fds_t fds_data[] PROGMEM = 
    MUI_FORM(1)
        MUI_STYLE(0)
            MUI_XYT("BN", 40, 40, " Select Me ");

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

  muif_t muif_list[] = {
    MUIF_U8G2_FONT_STYLE(0, u8g2_font_helvR08_tr),
    MUIF_U8G2_LABEL(),
    MUIF_BUTTON("BN", mui_u8g2_btn_exit_wm_fi)};

  /* 3-wire SPI constructor */
  u8g2_Setup_ssd1306_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_3wire_sw_spi, gpio_cb);

  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);  // switch off power save mode
  u8g2_SetContrast(&u8g2, 150); // <0; 255>

  u8g2_ClearBuffer(&u8g2);
  u8g2_ClearDisplay(&u8g2);

  u8g2_SetDrawColor(&u8g2, 1); // 1 = white
  u8g2_SetFontMode(&u8g2, 1);
  u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);

  mui_Init(&mui, &u8g2, fds_data, muif_list, sizeof(muif_list) / sizeof(muif_t));

  uint8_t form_ok = mui_GotoForm(&mui, 1, 0);
  u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
  if (form_ok == 1)
  {
    u8g2_DrawStr(&u8g2, 0, 30, "Menu OK!");
  }
  else
  {
    u8g2_DrawStr(&u8g2, 0, 30, "Menu ERROR");
  }

  mui_Draw(&mui);   

  u8g2_SendBuffer(&u8g2);

  while (1)
  {
    /*
    // If timer overflowed or first cycle (PCINT)
    if (debounceTimer == 1)
    {
      Debounce(&buttons[bttn_idx], Sample(bttn_idx));

      // If buttons released and debounce function is not counting
      if ((buttons[bttn_idx].stableState == 0) && (buttons[bttn_idx].debounceCount == 0))
      {
        debounceReady = 1;
        redraw = 1;
      }
    }

    // --- 2. LOGIKA MENU (Tlačítka) ---
    if (mui_IsFormActive(&mui))
    {
      // Reakce na tlačítko SEEK (Potvrzení / Select)
      if (buttons[SEEK_IDX].stableState == 1 && redraw)
      {
        redraw = 0;
        mui_SendSelect(&mui);

        u8g2_ClearBuffer(&u8g2);
        mui_GotoForm(&mui, 1, 0);
        mui_Draw(&mui);
        u8g2_SendBuffer(&u8g2);
      }
    }*/
  }
}