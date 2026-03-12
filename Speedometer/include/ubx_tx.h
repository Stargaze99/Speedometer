/*
 * ubx_tx.h
 *
 * Created: 2026-03-12 10:25:15
 *  Author: Admin
 */ 


#ifndef UBX_TX_H_
#define UBX_TX_H_

#include <stdint.h>
#include "uart.h"

void ubx_send(uint8_t cls, uint8_t id, uint16_t len, const uint8_t *payload);


#endif /* UBX_TX_H_ */