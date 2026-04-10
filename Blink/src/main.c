/*
 * Blink a LED using GPIO library and delay.
 * (c) 2018-2025 Tomas Fryza, MIT license
 *
 * Developed using PlatformIO and Atmel AVR platform.
 * Tested on Arduino Uno board and ATmega328P, 16 MHz.
 */

// -- Defines ----------------------------------------------
#define LED PD7 // On-board LED
#define BTN PD5

// -- Includes ---------------------------------------------
#include <avr/io.h> // AVR device-specific IO definitions
#include <gpio.h>   // GPIO library for AVR-GCC
#include <util/delay.h>

int main(void)
{
    gpio_mode_output(&DDRD, LED);
    gpio_mode_input_pullup(&DDRD, BTN);

    gpio_write_low(&PORTD, LED);

    // Infinite loop
    while (1)
    {
        
        if (gpio_read(&PIND, BTN) == 0)
        {
            gpio_write_high(&PORTD, LED); // Turn LED on
        }
        else
        {
            gpio_write_low(&PORTD, LED); // Turn LED off
        }
        /*
        gpio_write_high(&PORTD, LED); // Turn LED on
        _delay_ms(2000);
        gpio_write_low(&PORTD, LED); // Turn LED off
        _delay_ms(2000);
        */
    }

    // Will never reach this
    return 0;
}