/*
 * font.h
 *
 * Created: 2026-04-02 17:23:35
 *  Author: Admin
 */ 


#ifndef FONT_H_
#define FONT_H_

#include <stdint.h>
#include <avr/pgmspace.h>

#define FONT8X8_WIDTH   8
#define FONT8X8_FIRST   32
#define FONT8X8_LAST    126

extern const uint8_t font8x8[][FONT8X8_WIDTH] PROGMEM;
const uint8_t *font8x8_get(char c);

#endif /* FONT_H_ */