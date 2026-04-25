/*
 * backgrund.h
 *
 * Created: 2026-04-08 13:17:42
 *  Author: Admin
 */ 


#ifndef BACKGRUND_H_
#define BACKGRUND_H_

#include <stdint.h>
#include "oled.h"
#include "data.h"
#include <avr/pgmspace.h>

#define BG_TILE_WIDTH   128
#define BG_TOP_PAGES    4
#define BG_BOTTOM_PAGES 4

#define CYCLIST_W       24
#define CYCLIST_PAGES   3
#define CYCLIST_FRAMES  4

extern uint8_t bg_speed_level;

extern const uint8_t bg_top[4][128] PROGMEM;
extern const uint8_t bg_bottom[4][128] PROGMEM;

uint8_t bg_sample_top(uint8_t page, int16_t x);
uint8_t bg_sample_bottom(uint8_t page, int16_t x);
void bg_draw_parallax(int16_t top_x, int16_t bottom_x);

void bg_update_speed(void);
void bg_draw(void);

void cyclist_draw_frame(uint8_t frame, uint8_t page, uint8_t col);

void draw_gps_info(void);
void oled_write_2digit(uint8_t value);

#endif /* BACKGRUND_H_ */