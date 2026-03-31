/*
 * oled.c
 *
 * Created: 2026-03-26 16:00:47
 *  Author: Admin
 */ 

#include "config.h"
#include "oled.h"
#include "spi.h"


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

static const uint8_t *oled_font_get(char c)
{
    /* 5x7 font, one byte per column */
    /* Bit 0 = top pixel, bit 7 = bottom pixel in current page */

    static const uint8_t font_space[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    static const uint8_t font_dot[5]   = {0x00, 0x60, 0x60, 0x00, 0x00};
    static const uint8_t font_colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
    static const uint8_t font_dash[5]  = {0x08, 0x08, 0x08, 0x08, 0x08};
    static const uint8_t font_slash[5] = {0x20, 0x10, 0x08, 0x04, 0x02};

    static const uint8_t font_0[5] = {0x3E, 0x51, 0x49, 0x45, 0x3E};
    static const uint8_t font_1[5] = {0x00, 0x42, 0x7F, 0x40, 0x00};
    static const uint8_t font_2[5] = {0x42, 0x61, 0x51, 0x49, 0x46};
    static const uint8_t font_3[5] = {0x21, 0x41, 0x45, 0x4B, 0x31};
    static const uint8_t font_4[5] = {0x18, 0x14, 0x12, 0x7F, 0x10};
    static const uint8_t font_5[5] = {0x27, 0x45, 0x45, 0x45, 0x39};
    static const uint8_t font_6[5] = {0x3C, 0x4A, 0x49, 0x49, 0x30};
    static const uint8_t font_7[5] = {0x01, 0x71, 0x09, 0x05, 0x03};
    static const uint8_t font_8[5] = {0x36, 0x49, 0x49, 0x49, 0x36};
    static const uint8_t font_9[5] = {0x06, 0x49, 0x49, 0x29, 0x1E};

    static const uint8_t font_A[5] = {0x7E, 0x11, 0x11, 0x11, 0x7E};
    static const uint8_t font_C[5] = {0x3E, 0x41, 0x41, 0x41, 0x22};
    static const uint8_t font_D[5] = {0x7F, 0x41, 0x41, 0x22, 0x1C};
    static const uint8_t font_E[5] = {0x7F, 0x49, 0x49, 0x49, 0x41};
    static const uint8_t font_F[5] = {0x7F, 0x09, 0x09, 0x09, 0x01};
    static const uint8_t font_G[5] = {0x3E, 0x41, 0x49, 0x49, 0x7A};
    static const uint8_t font_H[5] = {0x7F, 0x08, 0x08, 0x08, 0x7F};
    static const uint8_t font_I[5] = {0x00, 0x41, 0x7F, 0x41, 0x00};
    static const uint8_t font_L[5] = {0x7F, 0x40, 0x40, 0x40, 0x40};
    static const uint8_t font_M[5] = {0x7F, 0x02, 0x0C, 0x02, 0x7F};
    static const uint8_t font_O[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
    static const uint8_t font_P[5] = {0x7F, 0x09, 0x09, 0x09, 0x06};
    static const uint8_t font_R[5] = {0x7F, 0x09, 0x19, 0x29, 0x46};
    static const uint8_t font_S[5] = {0x46, 0x49, 0x49, 0x49, 0x31};
    static const uint8_t font_T[5] = {0x01, 0x01, 0x7F, 0x01, 0x01};
    static const uint8_t font_U[5] = {0x3F, 0x40, 0x40, 0x40, 0x3F};
    static const uint8_t font_V[5] = {0x1F, 0x20, 0x40, 0x20, 0x1F};
    static const uint8_t font_X[5] = {0x63, 0x14, 0x08, 0x14, 0x63};

    switch (c)
    {
        case ' ': return font_space;
        case '.': return font_dot;
        case ':': return font_colon;
        case '-': return font_dash;
        case '/': return font_slash;

        case '0': return font_0;
        case '1': return font_1;
        case '2': return font_2;
        case '3': return font_3;
        case '4': return font_4;
        case '5': return font_5;
        case '6': return font_6;
        case '7': return font_7;
        case '8': return font_8;
        case '9': return font_9;

        case 'A': return font_A;
        case 'C': return font_C;
        case 'D': return font_D;
        case 'E': return font_E;
        case 'F': return font_F;
        case 'G': return font_G;
        case 'H': return font_H;
        case 'I': return font_I;
        case 'L': return font_L;
        case 'M': return font_M;
        case 'O': return font_O;
        case 'P': return font_P;
        case 'R': return font_R;
        case 'S': return font_S;
        case 'T': return font_T;
        case 'U': return font_U;
        case 'V': return font_V;
        case 'X': return font_X;

        default:
        return font_space;
    }
}

void oled_write_char(char c)
{
    uint8_t i;
    const uint8_t *font;

    /* Get pointer to 5-byte font pattern */
    font = oled_font_get(c);

    /* Write 5 font columns */
    for (i = 0; i < 5; i++)
    {
        oled_data(font[i]);
    }

    /* One blank column as spacing between characters */
    oled_data(0x00);
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