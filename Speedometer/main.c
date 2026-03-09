/*
 * Speedometer.c
 *
 * Created: 2026-02-12 14:19:38
 * Author : Admin
 */ 

#include "config.h"
#include "uart.h"
#include "ubx.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>


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

volatile int16_t roll_deg, pitch_deg, yaw_deg;

volatile uint16_t g_nav_att_count = 0;

uint8_t test = 0;

int main(void)
{
    leds_init();

    uart0_init(UART0_BAUD);
    sei(); /* behövs för att ISR i uart.c ska köras */
    
    g_roll_1e5  = 0;
    g_pitch_1e5 = 0;
    g_yaw_1e5   = 0;

    /* Start: blinka PD7 tre gånger */
    for (uint8_t i = 0; i < 3; i++) {
        led_toggle(PD7);
        _delay_ms(150);
        led_toggle(PD7);
        _delay_ms(150); 
    }

    /* Låt M8U boota lite */
    _delay_ms(500);

    /*
     * Viktigt:
     * För att dessa kommandon ska nå modulen måste PD1 (TX) vara kopplad till GPS RX.
     * Om GPS RX är 3.3V-only: använd nivådelare/level-shift från PD1.
     */
    ubx_disable_common_nmea();     /* valfritt men rekommenderas (minskar spam) */
    ubx_init_enable_nav_att(1);    /* be M8U skicka NAV-ATT (roll/pitch/yaw) */

    uint16_t hb = 0;
    
    while (1) {
        uint8_t b;

                /* Läs alla bytes från UART ringbuffer och mata UBX-parsern */
                while (uart0_read_nonblocking(&b)) {
                    ubx_parse_byte(b);
                    g_nav_att_count++; 
                }

                /*
                 * ubx.c uppdaterar g_roll_1e5/g_pitch_1e5/g_yaw_1e5 när NAV-ATT kommer.
                 * g_att_valid används här som “ny-data-flagga”.
                 */
                if (g_att_valid) {
                    g_att_valid = 0;       /* kvittera så vi får en puls varje gång */
                    
                    roll_deg  = ubx_deg_from_1e5(g_roll_1e5);
                    pitch_deg = ubx_deg_from_1e5(g_pitch_1e5);
                    yaw_deg   = ubx_deg_from_1e5(g_yaw_1e5);
                    
                    led_toggle(PD2);       /* PD2 blink = fick NAV-ATT */
                    test = 5;
                }
                


                /* Heartbeat PD6 ~200ms */
                hb++;
                if (hb >= 200) {
                    hb = 0;
                    led_toggle(PD6);
                }

        _delay_ms(1);
    }
}