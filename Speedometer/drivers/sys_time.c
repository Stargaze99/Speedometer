/*
 * sys_time.c
 *
 * Created: 2026-03-25 16:11:54
 *  Author: Admin
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#include "sys_time.h"

/* Global millisecond counter */
/* Increased every 1 ms in Timer0 compare interrupt */
volatile uint32_t sys_ms = 0;

void sys_time_init(void)
{
    /* Timer0 in CTC mode */
    TCCR0A = (1 << WGM01);
    TCCR0B = 0;

    /* Prescaler 64 */
    TCCR0B |= (1 << CS01) | (1 << CS00);

    /* 16 MHz / 64 = 250 kHz */
    /* 250 ticks = 1 ms */
    OCR0A = 249;

    /* Enable Timer0 compare match A interrupt */
    TIMSK0 |= (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect)
{
    /* 1 ms has passed */
    sys_ms++;
}

bool elapsed_ms(uint32_t *last_time, uint32_t duration_ms)
{
    uint32_t now = sys_ms;

    if ((now - *last_time) >= duration_ms)
    {
        *last_time = now;
        return true;
    }

    return false;
}