/*
 * uart.c
 *
 * Created: 2026-02-12 16:04:51
 *  Author: Admin
 */ 
#include "config.h"
#include "uart.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>


/* RX ringbuffer-storlek: måste vara 2^n (64, 128, 256...) */
#define RX_BUF_SIZE 512

static volatile uint8_t rx_buf[RX_BUF_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

static inline uint8_t rx_next(uint8_t i)
{
    return (uint8_t)((i + 1) & (RX_BUF_SIZE - 1));
}

void uart0_init(uint32_t baud)
{
    /* Normal speed (U2X0 = 0)
       UBRR = (F_CPU/(16*baud)) - 1
       Med avrundning: (F_CPU + 8*baud)/(16*baud) - 1 */
    uint16_t ubrr = (uint16_t)((F_CPU + 8UL * baud) / (16UL * baud) - 1UL);

    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)(ubrr);

    /* UCSR0A: normal speed */
    UCSR0A = 0;

    /* UCSR0B: enable RX, TX och RX interrupt */
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);

    /* UCSR0C: 8N1 */
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

/* RX interrupt: triggas när ett byte mottagits */
ISR(USART_RX_vect)
{
    uint8_t b = UDR0;
    uint8_t next = rx_next(rx_head);

    /* Om bufferten inte är full: spara */
    if (next != rx_tail) {
        rx_buf[rx_head] = b;
        rx_head = next;
    }
    /* Om full: kasta byte (enkel policy) */
}

bool uart0_available(void)
{
    return (rx_head != rx_tail);
}

bool uart0_read_nonblocking(uint8_t *out)
{
    if (!uart0_available()) {
        return false;
    }

    *out = rx_buf[rx_tail];
    rx_tail = rx_next(rx_tail);
    return true;
}

void uart0_write(uint8_t b)
{
    /* Vänta tills sändbufferten är tom */
    while (!(UCSR0A & (1 << UDRE0))) {
        /* väntar */
    }
    UDR0 = b;
}
