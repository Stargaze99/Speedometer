/*
 * oled.h
 *
 * Created: 2026-03-26 16:01:02
 *  Author: Admin
 */ 


#ifndef OLED_H_
#define OLED_H_

#include <stdint.h>
/* OLED size */
#define OLED_WIDTH     128
#define OLED_PAGES     8

/* Initialize OLED */
void oled_init(void);

/* Send one command byte */
void oled_cmd(uint8_t cmd);

/* Send one data byte */
void oled_data(uint8_t data);

/* Set write position */
void oled_set_pos(uint8_t page, uint8_t col);

/* Fill whole display with one pattern byte */
void oled_fill(uint8_t pattern);

/* Clear whole display */
void oled_clear(void);

/* Write one character using 5x7 font */
void oled_write_char(char c);

/* Write zero-terminated string */
void oled_write_string(const char *str);




/* Clear 16-pixel high span across two pages */
void oled_clear_span_16(uint8_t page, int16_t x, uint8_t width);


#endif /* OLED_H_ */