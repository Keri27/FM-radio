#include <stdint.h>

#define VREF 1100 // voltage reference (ATmega328p)
#define ADC6 6

void ADC_Init(void);
uint16_t ADC_Read(void);

uint8_t getBatteryPercentage(uint16_t rawADC);