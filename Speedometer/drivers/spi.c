/*
 * spi.c
 *
 * Created: 2026-03-26 15:41:19
 *  Author: Admin
 */ 

#include "spi.h"

#include <avr/io.h>

void spi_init(void)
{
    /* PB5 = SCK output */
    DDRB |= (1 << PB5);

    /* PB3 = MOSI output */
    DDRB |= (1 << PB3);

    /* PB2 = SS/CS output */
    DDRB |= (1 << PB2);


    /* CS  */
    PORTB |= (1 << PB2);

    /*
     * Enable SPI
     * Master mode
     * Clock = F_CPU / 16
     *
     * 16 MHz / 16 = 1 MHz SPI clock
     */
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);

    /* Normal speed */
    SPSR = 0x00;
}

uint8_t spi_transfer(uint8_t data)
{
    /* Start transmission */
    SPDR = data;

    /* Wait until transmission is complete */
    while (!(SPSR & (1 << SPIF)))
    {
    }

    /* Return received byte */
    return SPDR;
}