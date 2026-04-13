#ifndef DEP128064C1_H_
#define DEP128064C1_H_

#include <avr/io.h>
#include <stdbool.h>
#include "u8g2.h"

/* Display pinout */
#define PIN_SCK PB5
#define PIN_MOSI PB3
#define PIN_CS PB2
#define PIN_RST PB0

extern u8g2_t u8g2;

uint8_t gpio_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

void display_updateFreq(float actFreq);
void display_seekFail(float actFreq);

void display_changeVolume(uint8_t volume);
void display_changeAudioOutput(uint8_t output);
void display_changeBrightness(uint8_t brightness);

#endif /* DEP128064C1 */