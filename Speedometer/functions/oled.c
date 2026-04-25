/*
 * oled.c
 *
 * Created: 2026-03-26 16:00:47
 *  Author: Admin
 */ 

#include "config.h"
#include "oled.h"
#include "spi.h"
#include "font.h"


#include <avr/io.h>
#include <util/delay.h>

/* OLED control pins on PORTB */
#define OLED_PORT      PORTB
#define OLED_DDR       DDRB

#define OLED_RES_PIN   PB0
#define OLED_DC_PIN    PB1
#define OLED_CS_PIN    PB2

/* OLED size */
#define OLED_WIDTH     128
#define OLED_PAGES     8

/* Internal helper functions */
static void oled_pins_init(void);
static void oled_reset(void);


void oled_init(void)
{
    /* Initialize OLED control pins */
    oled_pins_init();

    /* Reset OLED controller */
    oled_reset();

    /* Unlock command interface */
    oled_cmd(0xFD);
    oled_cmd(0x12);

    /* Display OFF */
    oled_cmd(0xAE);

    /* Display clock divide ratio / oscillator frequency */
    oled_cmd(0xD5);
    oled_cmd(0x70);

    /* Multiplex ratio = 64 rows */
    oled_cmd(0xA8);
    oled_cmd(0x3F);

    /* Display offset = 0 */
    oled_cmd(0xD3);
    oled_cmd(0x00);

    /* Display start line = 0 */
    oled_cmd(0x40);

    /* Page addressing mode */
    oled_cmd(0x20);
    oled_cmd(0x02);

    /* Segment remap */
    oled_cmd(0xA1);

    /* COM scan direction remap */
    oled_cmd(0xC8);

    /* COM pins hardware configuration */
    oled_cmd(0xDA);
    oled_cmd(0x12);

    /* Pre-charge period */
    oled_cmd(0xD9);
    oled_cmd(0x22);

    /* VCOMH deselect level */
    oled_cmd(0xDB);
    oled_cmd(0x34);

    /* Resume to RAM content */
    oled_cmd(0xA4);

    /* Normal display */
    oled_cmd(0xA6);

    /* Disable scroll */
    oled_cmd(0x2E);

    /* Display ON */
    oled_cmd(0xAF);

    oled_clear();
}


static void oled_pins_init(void)
{
    /* RES, DC and CS are outputs */
    OLED_DDR |= (1 << OLED_RES_PIN);
    OLED_DDR |= (1 << OLED_DC_PIN);
    OLED_DDR |= (1 << OLED_CS_PIN);

    /* Idle state */
    /* RES high = not in reset */
    /* DC high = data mode if nothing else is selected */
    /* CS high = OLED not selected */
    OLED_PORT |= (1 << OLED_RES_PIN);
    OLED_PORT |= (1 << OLED_DC_PIN);
    OLED_PORT |= (1 << OLED_CS_PIN);
}

static void oled_reset(void)
{
    /* Make sure reset starts high */
    OLED_PORT |= (1 << OLED_RES_PIN);
    _delay_ms(1);

    /* Pull reset low */
    OLED_PORT &= ~(1 << OLED_RES_PIN);
    _delay_ms(10);

    /* Release reset */
    OLED_PORT |= (1 << OLED_RES_PIN);
    _delay_ms(10);
}

void oled_cmd(uint8_t cmd)
{
    /* Select OLED */
    OLED_PORT &= ~(1 << OLED_CS_PIN);

    /* Command mode */
    OLED_PORT &= ~(1 << OLED_DC_PIN);

    /* Send one command byte over SPI */
    spi_transfer(cmd);

    /* Deselect OLED */
    OLED_PORT |= (1 << OLED_CS_PIN);
}

void oled_data(uint8_t data)
{
    /* Select OLED */
    OLED_PORT &= ~(1 << OLED_CS_PIN);

    /* Data mode */
    OLED_PORT |= (1 << OLED_DC_PIN);

    /* Send one data byte over SPI */
    spi_transfer(data);

    /* Deselect OLED */
    OLED_PORT |= (1 << OLED_CS_PIN);
}


void oled_set_pos(uint8_t page, uint8_t col)
{
    /* Set page address: B0h to B7h */
    oled_cmd((uint8_t)(0xB0 | (page & 0x07)));

    /* Set lower column address nibble */
    oled_cmd((uint8_t)(0x00 | (col & 0x0F)));

    /* Set higher column address nibble */
    oled_cmd((uint8_t)(0x10 | ((col >> 4) & 0x0F)));
}

void oled_fill(uint8_t pattern)
{
    uint8_t page;
    uint8_t col;

    /* Write the same byte pattern to the whole display */
    for (page = 0; page < OLED_PAGES; page++)
    {
        oled_set_pos(page, 0);

        for (col = 0; col < OLED_WIDTH; col++)
        {
            oled_data(pattern);
        }
    }
}

void oled_clear(void)
{
    /* Fill display RAM with 0x00 = all pixels OFF */
    oled_fill(0x00);
}


void oled_write_char(char c)
{
    uint8_t i;
    const uint8_t *glyph;

    /* Get pointer to 8x8 glyph stored in flash */
    glyph = font8x8_get(c);

    /* Write 8 columns */
    for (i = 0; i < FONT8X8_WIDTH; i++)
    {
        oled_data(pgm_read_byte(&glyph[i]));
    }
}

void oled_write_string(const char *str)
{
    if (str == 0)
    {
        return;
    }

    /* Write characters until end of string */
    while (*str != '\0')
    {
        oled_write_char(*str);
        str++;
    }
}


void oled_clear_span_16(uint8_t page, int16_t x, uint8_t width)
{
    uint8_t i;
    int16_t col;

    if (page >= (OLED_PAGES - 1))
    {
        return;
    }

    for (i = 0; i < width; i++)
    {
        col = x + i;

        if ((col < 0) || (col >= OLED_WIDTH))
        {
            continue;
        }

        /* Clear upper page */
        oled_set_pos(page, (uint8_t)col);
        oled_data(0x00);

        /* Clear lower page */
        oled_set_pos((uint8_t)(page + 1), (uint8_t)col);
        oled_data(0x00);
    }
}