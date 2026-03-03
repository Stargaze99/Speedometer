/*
 * Speedometer.c
 *
 * Created: 2026-02-12 14:19:38
 * Author : Admin
 */ 

#include "config.h"
#include "uart.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>

/* NMEA radbuffer */
#define LINE_BUF_SIZE 160
static char line_buf[LINE_BUF_SIZE];
static uint8_t line_pos = 0;

static void leds_init(void)
{
    /* PD2..PD7 som output (LED). PD0/PD1 lämnas till UART. */
    DDRD |= 0b11111100;

    /* Pull-up på RX (PD0) så pinnen inte flyter om kabel glappar */
    PORTD |= (1 << PD0);
}

static void led_toggle(uint8_t pd_pin)
{
    /* AVR: skriva 1 till PINx togglar output */
    PIND = (1 << pd_pin);
}

static void led_set(uint8_t pd_pin, bool on)
{
    if (on)  PORTD |=  (1 << pd_pin);
    else     PORTD &= ~(1 << pd_pin);
}

/* RMC status är fält 2: $GNRMC,<time>,<A/V>,... */
static char rmc_status_char(const char *line)
{
    uint8_t commas = 0;
    for (uint16_t i = 0; line[i] != '\0'; i++) {
        if (line[i] == ',') {
            commas++;
            if (commas == 2) {
                return line[i + 1]; /* 'A' eller 'V' */
            }
        }
    }
    return '?';
}

int main(void)
{
    leds_init();

    uart0_init(UART0_BAUD);
    sei(); /* behövs för att ISR i uart.c ska köras */

    /* Start: blinka PD7 tre gånger */
    for (uint8_t i = 0; i < 3; i++) {
        led_toggle(PD7);
        _delay_ms(150);
        led_toggle(PD7);
        _delay_ms(150);
    }

    uint16_t hb = 0;
    uint8_t byte_div = 0; /* gör PD2 blink synlig (inte flimmer) */

    while (1) {
        uint8_t b;

        /* PD6 = heartbeat (programmet lever) */
        hb++;
        if (hb >= 200) {          /* ~200 ms */
            hb = 0;
            led_toggle(PD6);
        }

        /* Läs bytes från ringbuffer (fylls av ISR) */
        while (uart0_read_nonblocking(&b)) {

            /* PD2: blink ibland när bytes kommer (inte varje byte) */
            byte_div++;
            if ((byte_div & 0x1F) == 0) { /* var 32:e byte */
                led_toggle(PD2);
            }

            /* NMEA brukar vara \r\n */
            if (b == '\r') {
                continue;
            }

            if (b == '\n') {
                /* Hel rad klar */
                line_buf[line_pos] = '\0';
                line_pos = 0;

                /* PD3: rad klar */
                led_toggle(PD3);

                /* PD4: raden börjar med '$' (NMEA) */
                if (line_buf[0] == '$') {
                    led_toggle(PD4);
                }

                /* PD5: RMC? (NEO-8 brukar vara GN) */
                bool is_rmc = (strncmp(line_buf, "$GNRMC", 6) == 0) ||
                (strncmp(line_buf, "$GPRMC", 6) == 0);
                if (is_rmc) {
                    led_toggle(PD5);

                    /* PD7 ON om fix är valid (A), OFF annars */
                    led_set(PD7, (rmc_status_char(line_buf) == 'A'));
                }

                } else {
                /* Bygg rad */
                if (line_pos < (LINE_BUF_SIZE - 1)) {
                    line_buf[line_pos++] = (char)b;
                    } else {
                    /* Overflow: kasta raden */
                    line_pos = 0;
                    led_toggle(PD5); /* använd PD5 som "overflow-varning" */
                }
            }
        }

        _delay_ms(1);
    }
}