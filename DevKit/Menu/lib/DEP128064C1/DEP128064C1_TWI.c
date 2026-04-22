#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "gpio.h"
#include "u8g2.h"
#include "DEP128064C1_TWI.h"

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

u8g2_t u8g2;

/* 8x6 battery icon */
const unsigned char battery_icon[] =
{
    0xfc, // . . █ █ █ █ █ █
    0xff, // █ █ █ █ █ █ █ █
    0xff, // █ █ █ █ █ █ █ █
    0xff, // █ █ █ █ █ █ █ █
    0xff, // █ █ █ █ █ █ █ █
    0xfc  // . . █ █ █ █ █ █
};

/* Hardware I2C (TWI) callback function for u8g2 library */
uint8_t u8x8_byte_hw_i2c_avr(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    uint8_t *data;

    switch (msg)
    {
    case U8X8_MSG_BYTE_SEND:
        /* Send multiple bytes of data over I2C */
        data = (uint8_t *)arg_ptr;
        while (arg_int > 0)
        {
            TWDR = *data++; // Load the next byte into TWI Data Register
            TWCR = 0x84;    // Clear TWINT flag to start transmission (0x84 = TWINT | TWEN)
            while ((TWCR & 0x80) == 0)
                ; // Wait until transmission is complete (TWINT flag is set)
            arg_int--;
        }
        break;

    case U8X8_MSG_BYTE_INIT:
        /* Initialize TWI bus for 400 kHz Fast Mode (assuming 8 MHz CPU) */
        TWSR = 0;         // Set prescaler to 1
        TWBR = 2;         // Set bit rate register
        TWCR = _BV(TWEN); // Enable TWI hardware module
        break;

    case U8X8_MSG_BYTE_SET_DC:
        /* Data/Command pin is not used in I2C communication, ignore */
        break;

    case U8X8_MSG_BYTE_START_TRANSFER:
        /* 1. Generate and send START condition */
        TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
        while ((TWCR & _BV(TWINT)) == 0)
            ; // Wait for START to be transmitted

        /* 2. Send the target I2C device address */
        TWDR = u8x8_GetI2CAddress(u8x8);
        TWCR = _BV(TWINT) | _BV(TWEN);
        while ((TWCR & _BV(TWINT)) == 0)
            ; // Wait for address to be transmitted
        break;

    case U8X8_MSG_BYTE_END_TRANSFER:
        /* Generate and send STOP condition */
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
        while (TWCR & _BV(TWSTO))
            ; // Wait for the hardware to auto-clear the STOP bit
        break;
    }
    return 1;
}

/* Callback function for delay and GPIO handling (specifically hardware Reset) */
uint8_t u8x8_gpio_and_delay_avr(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg)
    {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        /* Initialize Reset pin as output and set it HIGH safely */
        DDRB |= _BV(RST1);
        PORTB |= _BV(RST1);
        _delay_ms(50); // Give the display controller time to boot up
        break;

    case U8X8_MSG_DELAY_MILLI:
        /* Delay by the specified number of milliseconds */
        while (arg_int--)
        {
            _delay_ms(1);
        }
        break;

    case U8X8_MSG_DELAY_10MICRO:
        /* Delay by the specified number of 10-microsecond multiples */
        while (arg_int--)
        {
            _delay_us(10);
        }
        break;

    case U8X8_MSG_DELAY_100NANO:
        /* 1 NOP instruction at 8 MHz takes exactly 125 ns, which is perfect for u8g2 */
        __asm__ volatile("nop");
        break;

    case U8X8_MSG_GPIO_RESET:
        /* Control the Reset pin (arg_int is 1 for HIGH, 0 for LOW) */
        if (arg_int)
        {
            PORTB |= _BV(RST1);
        }
        else
        {
            PORTB &= ~_BV(RST1);
        }
        break;
    }
    return 1;
}

void display_updateChannel(float actFreq, uint8_t rssi, uint8_t stereo, uint8_t seekFail, uint8_t battery)
{
    char str1[8]; // Buffer (e.g. "106.50")
    char str2[4];
    char str3[8];

    dtostrf(actFreq, 4, 1, str1);
    itoa(rssi, str2, /* dec */ 10); // integer to ascii
    itoa(battery, str3, 10);

    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
    // u8g2_GetStrWidth(u8g2_t *u8g2, const char *s); // center freq
    u8g2_DrawStr(&u8g2, 40, 28, str1); // frequency

    u8g2_SetFont(&u8g2, u8g2_font_courB08_tf);
    if (seekFail)
    {
        u8g2_DrawStr(&u8g2, 32, 45, "Stanice");
        u8g2_DrawStr(&u8g2, 25, 55, "nenalezena!");
    }

    // u8g2_DrawStr(&u8g2, 50, 10, "RSSI:"); // RSSI
    // strcat("RSSI:", str2);
    u8g2_DrawStr(&u8g2, 55, 8, str2); // RSSI
    if (!stereo)
        u8g2_DrawStr(&u8g2, 20, 8, "M"); // stereo/mono indicator
    else
        u8g2_DrawStr(&u8g2, 20, 8, "S");

    u8g2_DrawXBM(&u8g2, 95, 2, 8, 6, battery_icon);
    strcat(str3, "%");
    u8g2_DrawStr(&u8g2, 110, 8, str3); // battery percentage

    u8g2_SendBuffer(&u8g2);
}

void display_updateRDS(float actFreq, uint8_t rssi, uint8_t stereo, uint8_t seekFail, uint8_t battery, const char *programmeName)
{
    char str1[8]; // Buffer (e.g. "106.50")
    char str2[4];
    char str3[8];

    dtostrf(actFreq, 4, 1, str1);
    itoa(rssi, str2, /* dec */ 10); // integer to ascii
    itoa(battery, str3, 10);

    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
    // u8g2_GetStrWidth(u8g2_t *u8g2, const char *s); // center freq
    u8g2_DrawStr(&u8g2, 40, 28, str1); // frequency

    u8g2_SetFont(&u8g2, u8g2_font_courB08_tf);
    if (seekFail)
    {
        u8g2_DrawStr(&u8g2, 32, 45, "Stanice");
        u8g2_DrawStr(&u8g2, 25, 55, "nenalezena!");
    }
    else
    {
        // u8g2_GetStrWidth(u8g2_t *u8g2, const char *s); // center text
        u8g2_DrawStr(&u8g2, 25, 50, programmeName); // e.g. Radio Krokodyl
    }

    // u8g2_DrawStr(&u8g2, 50, 10, "RSSI:"); // RSSI
    // strcat("RSSI:", str2);
    u8g2_DrawStr(&u8g2, 55, 8, str2); // RSSI
    if (!stereo)
        u8g2_DrawStr(&u8g2, 20, 8, "M"); // stereo/mono indicator
    else
        u8g2_DrawStr(&u8g2, 20, 8, "S");

    u8g2_DrawXBM(&u8g2, 95, 2, 8, 6, battery_icon);
    strcat(str3, "%");
    u8g2_DrawStr(&u8g2, 110, 8, str3); // battery percentage

    u8g2_SendBuffer(&u8g2);
}

void display_changeVolume(uint8_t volume)
{
    char str[3]; // 1 ascii char = 1B + end sign + reserve

    if (volume)
        volume = (volume + 1) / 2;
    else
        volume = 0;
    itoa(volume, str, /* dec */ 10); // integer to ascii

    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
    u8g2_DrawStr(&u8g2, 10, 15, "Hlasitost");
    u8g2_DrawStr(&u8g2, 20, 50, str);

    u8g2_DrawFrame(&u8g2, 13, 25, 84, 6);
    if (volume)
        u8g2_DrawBox(&u8g2, 15, 27, (volume) * 10, 2);

    u8g2_SendBuffer(&u8g2);
}

void display_changeAudioOutput(uint8_t output)
{
    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
    u8g2_DrawStr(&u8g2, 10, 15, "Audio vystup");
    u8g2_SetFont(&u8g2, u8g2_font_courB08_tf);
    u8g2_DrawStr(&u8g2, 40, 35, "Reproduktor");
    u8g2_DrawStr(&u8g2, 40, 55, "Sluchatka");

    u8g2_DrawCircle(&u8g2, 20, 30, 5, U8G2_DRAW_ALL);
    u8g2_DrawCircle(&u8g2, 20, 50, 5, U8G2_DRAW_ALL);

    /* Audio output: Speaker */
    if (!output)
    {
        u8g2_DrawFilledEllipse(&u8g2, 20, 30, 3, 3, U8G2_DRAW_ALL);
    }
    /* Audio output: Headphones */
    else
    {
        u8g2_DrawFilledEllipse(&u8g2, 20, 50, 3, 3, U8G2_DRAW_ALL);
    }

    u8g2_SendBuffer(&u8g2);
}

void display_changeBrightness(uint8_t brightness)
{
    char str[5];
    brightness = brightness / 2.5;
    itoa(brightness, str, 10);

    u8g2_ClearBuffer(&u8g2);

    u8g2_DrawCircle(&u8g2, 45, 32, 30, U8G2_DRAW_ALL);
    uint8_t rad = (brightness / 10) * 3;
    u8g2_DrawFilledEllipse(&u8g2, 45, 32, rad, rad, U8G2_DRAW_ALL);

    u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
    u8g2_DrawStr(&u8g2, 85, 31, "Jas");
    strcat(str, "%");
    u8g2_DrawStr(&u8g2, 85, 45, str);
    // u8g2_DrawStr(&u8g2, 115, 45, "%");

    u8g2_SendBuffer(&u8g2);
}