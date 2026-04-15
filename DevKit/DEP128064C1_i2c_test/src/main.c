#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "u8g2.h"
#include "gpio.h"

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#define RST1 PB0

u8g2_t u8g2;

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
            while ((TWCR & 0x80) == 0); // Wait until transmission is complete (TWINT flag is set)
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
        _delay_ms(50);  // Give the display controller time to boot up
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

int main(void)
{
    // Setup u8g2
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R2, u8x8_byte_hw_i2c_avr, u8x8_gpio_and_delay_avr); // U8G2_R2

    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_SetContrast(&u8g2, 150);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_courB08_tf);
    u8g2_DrawStr(&u8g2, 10, 20, "Pozdravuje Vas");
    u8g2_DrawStr(&u8g2, 10, 30, "pan...");
    u8g2_SendBuffer(&u8g2);

    while (1)
    {
    }
}