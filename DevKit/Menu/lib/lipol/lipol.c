#include <avr/io.h>
#include <stdint.h>
#include "lipol.h"

/* Initializes the ADC module */
void ADC_Init(void)
{
    // Intern reference 1.1V
    ADMUX = (1 << REFS1) | (1 << REFS0);

    // Switch ON ADC and set prescaler to 64 (8 MHz / 64 = 125 kHz)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
}

/*
 * Reads the analog value from a specific channel
 * Output: 10-bit raw ADC value (0-1023)
 */
uint16_t ADC_Read(void)
{
    // Clear old channel bits and set the new one (preserve REFS bits)
    ADMUX = (ADMUX & 0xF0) | (ADC_BATT & 0x0F);

    ADCSRA |= (1 << ADSC);

    // Wait for the conversion to complete
    while (ADCSRA & (1 << ADSC));

    return ADC;
}

/*
 * Non-linear Li-Po discharge curve (in mV).
 * Warning: This licp curve is wrong and needs to be fixed: device stays on 5% for more than 5h!
 */
const uint16_t lipo_curve_mv[] =
    {
        3300, // 0%
        3338, // 1%
        3414, // 3%
        3490, // 5%
        3680, // 10%
        3710, // 15%
        3740, // 20%
        3770, // 30%
        3790, // 40%
        3820, // 50%
        3870, // 60%
        3920, // 70%
        3980, // 80%
        4060, // 90%
        4130, // 95 %
        4200  // 100% (Fully charged)
};

const uint8_t lipo_curve_pct[] =
    {
        0, 1, 3, 5, 10, 15, 20, 30, 40, 50, 60, 70, 80, 90, 95, 100};

/*
 * Calculates battery percentage
 * Input: Voltage in mV (e.g., 3850)
 * Output: Percentage 0-100 (non-linear mapping)
 */
uint8_t getBatteryPercentage(uint16_t rawADC)
{
    // 1. Convert ADC to pin voltage in mV
    uint32_t pinVoltage = ((uint32_t)rawADC * VREF) / 1024;

    // 2. Calculate actual battery voltage (divider ratio = 4.3)
    uint32_t batVoltage = (pinVoltage * 43U) / 10U;

    // 3. Out of bounds protection
    if (batVoltage >= 4200)
        return 100;
    if (batVoltage <= 3300)
        return 0;

    // 4. Iterate from 100% down to 0%
    for (int8_t i = (int8_t)(sizeof(lipo_curve_mv) / sizeof(lipo_curve_mv[0])) - 1; i >= 0; i--)
    {
        if (batVoltage >= lipo_curve_mv[i])
        {
            return lipo_curve_pct[i];
        }
    }

    return 0;
}