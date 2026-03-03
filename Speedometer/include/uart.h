/*
 * uart.h
 *
 * Created: 2026-02-12 15:55:48
 *  Author: Admin
 */ 


#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include <stdbool.h>

/* Initiera UART0 (USART0) med vald baudrate */
void uart0_init(uint32_t baud);

/* True om det finns minst en byte i RX-bufferten */
bool uart0_available(void);

/* Läs en byte om det finns data, annars false */
bool uart0_read_nonblocking(uint8_t *out);

/* (Valfritt) Skicka en byte */
void uart0_write(uint8_t b);

#endif /* UART_H_ */