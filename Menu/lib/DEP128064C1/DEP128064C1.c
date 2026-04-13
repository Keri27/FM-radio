#include <avr/io.h>
#include <stdbool.h>
#include <stdlib.h>
#include <util/delay.h>

#include "gpio.h"
#include "u8g2.h"
#include "DEP128064C1.h"

u8g2_t u8g2;

/* GPIO + delay callback for u8g2 */
uint8_t gpio_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    /* Suppress warnings about unused parameters */
    (void)u8x8;
    (void)arg_ptr;

    switch (msg)
    {
    /* Init all required pins */
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        gpio_mode_output(&DDRB, PIN_SCK);
        gpio_mode_output(&DDRB, PIN_MOSI);
        gpio_mode_output(&DDRB, PIN_CS);
        gpio_mode_output(&DDRB, PIN_RST);

        gpio_write_high(&PORTB, PIN_CS);  // CS high (inactive)
        gpio_write_high(&PORTB, PIN_RST); // RST high
        return 1;

    /* Chip Select pin */
    case U8X8_MSG_GPIO_CS:
        if (arg_int)
            gpio_write_high(&PORTB, PIN_CS); // CS high
        else
            gpio_write_low(&PORTB, PIN_CS); // CS low
        return 1;

    /* Reset pin */
    case U8X8_MSG_GPIO_RESET:
        if (arg_int)
            gpio_write_high(&PORTB, PIN_RST);
        else
            gpio_write_low(&PORTB, PIN_RST);
        return 1;

    /* Clock pin */
    case U8X8_MSG_GPIO_SPI_CLOCK:
        if (arg_int)
            gpio_write_high(&PORTB, PIN_SCK);
        else
            gpio_write_low(&PORTB, PIN_SCK);
        return 1;

    /* MOSI pin */
    case U8X8_MSG_GPIO_SPI_DATA:
        if (arg_int)
            gpio_write_high(&PORTB, PIN_MOSI);
        else
            gpio_write_low(&PORTB, PIN_MOSI);
        return 1;

    /* Delay */
    case U8X8_MSG_DELAY_MILLI:
        while (arg_int--)
            _delay_ms(1);
        return 1;
    }

    return 0;
}

void display_updateFreq(float actFreq)
{
    char str[10]; // Buffer (e.g. "106.50")
    dtostrf(actFreq, 4, 1, str);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
    u8g2_DrawStr(&u8g2, 40, 28, str);
    u8g2_SendBuffer(&u8g2);
}

void display_seekFail(float actFreq)
{
    char str[10]; // Buffer (e.g. "106.50")
    dtostrf(actFreq, 4, 1, str);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
    u8g2_DrawStr(&u8g2, 40, 28, str);
    u8g2_SetFont(&u8g2, u8g2_font_courB08_tf);
    u8g2_DrawStr(&u8g2, 25, 45, "Stanice nenalezena!");
    u8g2_SendBuffer(&u8g2);
}

void display_changeVolume(uint8_t volume)
{
    char str[10];
    volume = (volume/1.5)*10;
    itoa(volume, str, 10);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
    u8g2_DrawStr(&u8g2, 20, 15, "Hlasitost:");
    //u8g2_DrawHLine(&u8g2, 10, 14, 144);
    u8g2_DrawStr(&u8g2, 40, 40, str);
    u8g2_DrawStr(&u8g2, 60, 40, "%");
    u8g2_SendBuffer(&u8g2);
}

void display_changeAudioOutput(uint8_t output)
{
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
    u8g2_DrawStr(&u8g2, 10, 15, "Audio vystup:");

    u8g2_DrawCircle(&u8g2, 20, 35, 5, U8G2_DRAW_ALL);
    u8g2_DrawCircle(&u8g2, 20, 55, 5, U8G2_DRAW_ALL);
    u8g2_SetFont(&u8g2, u8g2_font_courB08_tf);
    u8g2_DrawStr(&u8g2, 40, 35, "Reproduktor");
    u8g2_DrawStr(&u8g2, 40, 55, "Sluchatka");
    
    /* Audio output: Speaker */
    if (!output)
    {
        u8g2_DrawFilledEllipse(&u8g2, 20, 35, 6, 6, U8G2_DRAW_ALL);
    }
    /* Audio output: Headphones */
    else
    {
        u8g2_DrawFilledEllipse(&u8g2, 20, 55, 6, 6, U8G2_DRAW_ALL);
    }

    u8g2_SendBuffer(&u8g2);
}

void display_changeBrightness(uint8_t brightness)
{
    char str[10];
    brightness = brightness/2.5;
    itoa(brightness, str, 10);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
    u8g2_DrawStr(&u8g2, 20, 15, "Jas:");
    // u8g2_DrawHLine(&u8g2, 10, 14, 144);
    u8g2_DrawStr(&u8g2, 40, 40, str);
    u8g2_DrawStr(&u8g2, 60, 40, "%");
    u8g2_SendBuffer(&u8g2);
}

