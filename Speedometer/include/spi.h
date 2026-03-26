/*
 * spi.h
 *
 * Created: 2026-03-26 15:45:23
 *  Author: Admin
 */ 


#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>

/* Initialize AVR hardware SPI as master */
void spi_init(void);

/* Send one byte over SPI */
uint8_t spi_transfer(uint8_t data);

#endif /* SPI_H_ */