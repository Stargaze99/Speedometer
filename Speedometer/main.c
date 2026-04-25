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
#include "gps_cfg.h"
#include "spi.h"
#include "oled.h"
#include "background.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>


/* Airplane sprite  */
/* 32 pixels wide, 16 pixels high */
/* Top half = first 8 rows, bottom half = next 8 rows */

static const uint8_t plane_top[32] =
{
    0x06, 0x0E, 0x9E, 0xFC, 0xFC, 0xF8, 0xF8, 0xF0,
    0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF3, 0xF7, 0xFF,
    0xFF, 0xFE, 0xFC, 0xFC, 0xF8, 0xF8, 0xF0, 0xF0,
    0xF0, 0xF0, 0xE0, 0xE0, 0xC0, 0x80, 0x80, 0x00
};


static const uint8_t plane_bottom[32] =
{
    0x00, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0xC0,
    0xE1, 0xF1, 0xFB, 0x7B, 0x3F, 0x3F, 0x1F, 0x1F,
    0x1F, 0x0F, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x03, 0x03
};

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
    sys_time_init();
    data_init();
    spi_init();
    oled_init();
    gps_cfg_profile(1);
    sei();
   
   uint32_t bg_last_ms = 0;
   
   uint32_t cyclist_last_ms = 0;
   uint8_t anim_frame = 0;
   
   
    while (1) {
        uint8_t b;
        
        /* Count one main loop cycle */
        gps_data.debug.main_loop.total_count++;
        
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
            
            /* Main loop cycles per second */
            gps_data.debug.main_loop.per_second =
            gps_data.debug.main_loop.total_count - gps_data.debug.main_loop.prev_count;

            gps_data.debug.main_loop.prev_count =
            gps_data.debug.main_loop.total_count;
        }
        /*_delay_ms(1);*/
        
        
        /* Parallax background */
        if (elapsed_ms(&bg_last_ms, 50)){
            
            /*bg_update_speed();*/
            bg_speed_level = 2;
            bg_draw();
            
            draw_gps_info();
            }
            
        if (elapsed_ms(&cyclist_last_ms, 200))
        {
            anim_frame++;
            if (anim_frame >= CYCLIST_FRAMES)
            {
                anim_frame = 0;
            }
        }
        cyclist_draw_frame(anim_frame, 4, 4);
        

        
/*
        oled_set_pos(0, 0);
        oled_write_string("GPS TEST");

        oled_set_pos(2, 0);
        oled_write_string("HELLO");

        oled_set_pos(4, 0);
        oled_write_string("0123456789");

        oled_set_pos(6, 0);
        oled_write_string("ABCXYZ abcxyz");
        
        oled_set_pos(7, 0);
        oled_write_string("!#¤%&/()=");*/
    }
}