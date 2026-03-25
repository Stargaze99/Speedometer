/*
 * Speedometer.c
 *
 * Created: 2026-02-12 14:19:38
 * Author : Admin
 */ 

#include "config.h"
#include "uart.h"
#include "data.h"
#include "nmea_parse.h"
#include "sys_time.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define LINE_BUF_SIZE 128
#define NMEA_STORE_SIZE 98 /*96+2 byte för overload*/

static char line_buf[LINE_BUF_SIZE];
static uint8_t line_pos = 0;
static uint8_t in_sentence = 0;

/* Exempel: separata “senaste”-buffers */
static char rmc_buf[NMEA_STORE_SIZE];
static char gga_buf[NMEA_STORE_SIZE];
/*static char vtg_buf[NMEA_STORE_SIZE];*/

static inline void led_on(uint8_t pd)     { PORTD |=  (1 << pd); }
static inline void led_off(uint8_t pd)    { PORTD &= ~(1 << pd); }
static inline void led_toggle(uint8_t pd) { PIND   =  (1 << pd); }

static void leds_init(void)
{
    /* PD2, PD3, PD6 som output */
    DDRD |= (1 << PD2) | (1 << PD3) | (1 << PD6);

    /* Start: stabilt läge */
    led_on(PD6);
    led_off(PD2);
    led_off(PD3);

    /* Pull-up på RX (PD0) om du vill */
    PORTD |= (1 << PD0);
}

static void copy_sentence(char *dst, const char *src)
{
    strncpy(dst, src, NMEA_STORE_SIZE - 1);
    dst[NMEA_STORE_SIZE - 1] = '\0';
}

static void store_sentence(const char *s)
{
    if (!strncmp(s, "$GNRMC", 6) || !strncmp(s, "$GPRMC", 6)) {
        copy_sentence(rmc_buf, s);
        } else if (!strncmp(s, "$GNGGA", 6) || !strncmp(s, "$GPGGA", 6)) {
        copy_sentence(gga_buf, s);
        }/* else if (!strncmp(s, "$GNVTG", 6) || !strncmp(s, "$GPVTG", 6)) {
        copy_sentence(vtg_buf, s);
    }*/
    /* Lägg till fler om du vill: GSA/GSV/GLL osv */
}

int main(void)
{
    leds_init();
    
    uint32_t gps_rate_last_ms = 0;
    
    uart0_init(UART0_BAUD);
    data_init();
    sys_time_init();
    gps_cfg_profile(1);
    sei();

    while (1) {
        uint8_t b;

        while (uart0_read_nonblocking(&b)) {

            if (b == '\r') {
                continue;
            }

            /* Start på ny mening */
            if (b == '$') {
                in_sentence = 1;
                line_pos = 0;
                line_buf[line_pos++] = '$';

                /* PD2 ON = tar emot mening */
                led_on(PD2);

                /* PD6 OFF = inte stabilt (mening pågår) */
                led_off(PD6);

                continue;
            }

            /* Ignorera allt tills vi sett '$' */
            if (!in_sentence) {
                continue;
            }

            /* Slut på mening */
            if (b == '\n') {
                line_buf[line_pos] = '\0';
                in_sentence = 0;

                /* Copy/överföring (kritisk sektion) */
                led_off(PD6);        /* OFF medan vi skriver arrays */
                led_toggle(PD3);     /* blink start */

                store_sentence(line_buf);
                nmea_parse_line(line_buf);

                led_toggle(PD3);     /* blink slut */

                /* PD2 OFF = meningen färdig */
                led_off(PD2);

                /* PD6 ON = stabilt läge (pausa debugger här) */
                led_on(PD6);

                line_pos = 0;
                continue;
            }

            /* Bygg line_buf */
            if (line_pos < (LINE_BUF_SIZE - 1)) {
                line_buf[line_pos++] = (char)b;
                } else {
                /* Overflow: droppa meningen */
                in_sentence = 0;
                line_pos = 0;
                led_off(PD2);
                led_on(PD6);
            }
        }
        
        /* Update debug counters every 1000 ms */
        if (elapsed_ms(&gps_rate_last_ms, 1000))
        {
            gps_data.debug.rmc.per_second =
            (uint8_t)(gps_data.debug.rmc.total_count - gps_data.debug.rmc.prev_count);

            gps_data.debug.rmc.prev_count =
            gps_data.debug.rmc.total_count;

            gps_data.debug.gga.per_second =
            (uint8_t)(gps_data.debug.gga.total_count - gps_data.debug.gga.prev_count);

            gps_data.debug.gga.prev_count =
            gps_data.debug.gga.total_count;
        }
        /*_delay_ms(1);*/
    }
}