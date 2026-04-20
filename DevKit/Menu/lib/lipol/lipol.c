#include <avr/io.h>
#include <stdint.h>
#include "lipol.h"

/* Initializes the ADC module */
void ADC_Init(void)
{
    // Set internal 1.1V reference
    ADMUX = (1 << REFS1) | (1 << REFS0);

    // Enable ADC (ADEN) and set prescaler to 128 (ADPSx)
    // 16 MHz / 128 = 125 kHz ADC clock
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

/*
 * Reads the analog value from a specific channel
 * Input: channel number (e.g., 6 for ADC6)
 * Output: 10-bit raw ADC value (0-1023)
 */
uint16_t ADC_Read(void)
{
    // Clear old channel bits and set the new one (preserve REFS bits)
    ADMUX = (ADMUX & 0xF0) | (ADC6 & 0x0F);

    // Start the conversion
    ADCSRA |= (1 << ADSC);

    // Wait for the conversion to complete (hardware clears ADSC)
    while (ADCSRA & (1 << ADSC))
        ;

    // Return the 16-bit result (automatically combines ADCL and ADCH)
    return ADC;
}

/*
 * Non-linear Li-Po discharge curve (in mV)
 * 0% = 3300mV, 100% = 4200mV
 */
const uint16_t lipo_curve[11] = {
    3300, // 0%
    3680, // 10%
    3740, // 20%
    3770, // 30%
    3790, // 40%
    3820, // 50%
    3870, // 60%
    3920, // 70%
    3980, // 80%
    4060, // 90%
    4200  // 100% (Fully charged)
};

/*
 * Calculates battery percentage
 * Input: Voltage in mV (e.g., 3850)
 * Output: Percentage 0-100 (in steps of 10)
 */
uint8_t getBatteryPercentage(uint16_t rawADC)
{
    // 1. Convert ADC to pin voltage in mV
    uint32_t pinVoltage = ((uint32_t)rawADC * VREF) / 1024;

    // 2. Calculate actual battery voltage (divider ratio = 4.3)
    // Note: Using * 43 / 10 instead of * 4.3 to keep it 100% integer math!
    uint16_t voltage = (pinVoltage * 43) / 10;

    // 3. Out of bounds protection
    if (voltage >= 4200)
        return 100;
    if (voltage <= 3300)
        return 0;

    // 4. Iterate from 100% down to 0%
    for (int8_t i = 10; i >= 0; i--)
    {
        if (voltage >= lipo_curve[i])
        {
            return i * 10; // Match found
        }
    }

    return 0;
}