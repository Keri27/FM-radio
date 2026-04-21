#ifndef DEP128064C1_TWI_H_
#define DEP128064C1_TWI_H_

#include <avr/io.h>
#include <stdint.h>
#include "u8g2.h"

/* Display pinout */
#define RST1 PB0
// no need to define SDA and SCL

extern u8g2_t u8g2;

/* Callback functions */
uint8_t u8x8_byte_hw_i2c_avr(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_gpio_and_delay_avr(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

void display_updateChannel(float actFreq, uint8_t rssi, uint8_t stereo, uint8_t seekFail, uint8_t battery);
void display_seekFail(float actFreq);

void display_changeVolume(uint8_t volume);
void display_changeAudioOutput(uint8_t output);
void display_changeBrightness(uint8_t brightness);

#endif /* DEP128064C1_TWI */