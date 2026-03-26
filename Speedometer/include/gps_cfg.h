/*
 * gps_cfg.h
 *
 * Created: 2026-03-11 15:24:24
 *  Author: Admin
 */ 



#ifndef GPS_CFG_H_
#define GPS_CFG_H_

#include <stdint.h>
#include "uart.h"
#include "ubx_tx.h"

void gps_cfg_profile(uint8_t profile);

static void ubx_cfg_rate(uint16_t measRate, uint16_t navRate, uint16_t timeRef);


#endif /* GPS_CFG_H_ */