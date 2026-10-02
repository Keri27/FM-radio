#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
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

/* Draw centered string */
static inline void u8g2_DrawStrCentered(u8g2_t *u8g2, uint8_t y, const char *text);

/* Trims trailing and leading spaces, returns pointer to the new start */
static char *trim_spaces(char *str)
{
    // Right trim
    for (int i = strlen(str) - 1; i >= 0; i--)
    {
        if (str[i] == ' ')
        {
            str[i] = '\0';
        }
        else
        {
            break;
        }
    }

    // Left trim
    char *start = str;
    while (*start == ' ')
    {
        start++;
    }

    return start;
}

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

static inline void u8g2_DrawStrCentered(u8g2_t *u8g2, uint8_t y, const char *text)
{
    char buf[16];
    strncpy(buf, text, 15);
    buf[15] = '\0';

    char *trimmedText = trim_spaces(buf);

    u8g2_uint_t textWidth = u8g2_GetStrWidth(u8g2, trimmedText);

    u8g2_uint_t x = 0;
    if (textWidth < 128)
    {
        x = (128 - textWidth) / 2;
    }

    u8g2_DrawStr(u8g2, x, y, trimmedText);
}

void display_updateChannel(float actFreq, uint8_t rssi, uint8_t stereo, uint8_t seekFail, uint8_t battery, const char *programmeName, uint8_t hour, uint8_t minute)
{
    char freq_str[8]; // Buffer (e.g. "106.50")
    char rssi_str[4];
    char batt_str[8];
    char time_str[6]; // "HH:MM"

    dtostrf(actFreq, 4, 1, freq_str);
    itoa(rssi, rssi_str, /* dec */ 10); // integer to ascii
    itoa(battery, batt_str, 10);

    u8g2_ClearBuffer(&u8g2);

    /* Frequency */
    u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
    u8g2_DrawStrCentered(&u8g2, 33, freq_str);

    /* Time */
    u8g2_SetFont(&u8g2, u8g2_font_courB08_tf);

    if (hour < 24)
    {
        snprintf(time_str, sizeof(time_str), "%02u:%02u", hour, minute);
    }
    else
    {
        snprintf(time_str, sizeof(time_str), "--:--"); // Čekáme na data z RDS
    }

    u8g2_DrawStr(&u8g2, 5, 8, time_str);

    /* Stereo/Mono indicator */
    if (!stereo)
        u8g2_DrawStr(&u8g2, 50, 8, "M");
    else
        u8g2_DrawStr(&u8g2, 50, 8, "S");

    /* RSSI */
    // strcat("RSSI:", rssi_str);
    u8g2_DrawStr(&u8g2, 70, 8, rssi_str);

    /* Battery percentage */
    u8g2_DrawXBM(&u8g2, 95, 2, 8, 6, battery_icon);
    strcat(batt_str, "%");
    u8g2_DrawStr(&u8g2, 105, 8, batt_str);

    /* Station name */
    if (seekFail)
    {
        u8g2_DrawStrCentered(&u8g2, 50, "Stanice");
        u8g2_DrawStrCentered(&u8g2, 60, "nenalezena!");
    }
    else
    {
        if (strcmp(programmeName, "loading") == 0) // if programmeName == "loading" -> loading
        {
            u8g2_SetFont(&u8g2, u8g2_font_courB10_tf);
            u8g2_DrawStrCentered(&u8g2, 55, "...");
        }
        else if (strcmp(programmeName, "none") == 0) // if radio is unable to decode the RDS
        {
            u8g2_DrawStrCentered(&u8g2, 55, "Spatny signal");
        }
        else
        {
            u8g2_SetFont(&u8g2, u8g2_font_courB10_tf);
            u8g2_DrawStrCentered(&u8g2, 55, programmeName); // e.g. Radio Krokodyl
        }
    }

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

    u8g2_DrawFrame(&u8g2, 10, 25, 84, 6);
    if (volume)
        u8g2_DrawBox(&u8g2, 12, 27, (volume) * 10, 2);

    u8g2_SendBuffer(&u8g2);
}

void display_changeAudioOutput(uint8_t output)
{
    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
    u8g2_DrawStrCentered(&u8g2, 15, "Audio vystup");
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

    u8g2_DrawCircle(&u8g2, 40, 32, 30, U8G2_DRAW_ALL);
    uint8_t rad = (brightness / 10) * 3;
    u8g2_DrawFilledEllipse(&u8g2, 40, 32, rad, rad, U8G2_DRAW_ALL);

    u8g2_SetFont(&u8g2, u8g2_font_courB12_tf);
    u8g2_DrawStr(&u8g2, 85, 31, "Jas");
    strcat(str, "%");
    u8g2_DrawStr(&u8g2, 85, 45, str);
    // u8g2_DrawStr(&u8g2, 115, 45, "%");

    u8g2_SendBuffer(&u8g2);
}