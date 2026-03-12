/*
 * ubx_ver.h
 *
 * Created: 2026-03-12 10:40:46
 *  Author: Admin
 */ 


#ifndef UBX_VER_H_
#define UBX_VER_H_

#include <stdint.h>
#include <stdbool.h>

/* u-blox MON-VER: swVersion[30], hwVersion[10], extension[30]*N */
#define UBX_SW_LEN   30
#define UBX_HW_LEN   10
#define UBX_EXT_LEN  30
#define UBX_EXT_MAX  6   /* spara upp till 6 extension-rader (räcker oftast) */

/* Debug-vänliga buffers (lägg i Watch i debuggern) */
extern volatile uint8_t ubx_ver_ready;

extern char ubx_sw_version[UBX_SW_LEN + 1];
extern char ubx_hw_version[UBX_HW_LEN + 1];
extern char ubx_ext[UBX_EXT_MAX][UBX_EXT_LEN + 1];
extern volatile uint8_t ubx_ext_count;

/* Skicka poll för UBX-MON-VER (0x0A 0x04) */
void ubx_ver_poll(void);

/* Mata in bytes från UART (kalla för varje mottaget byte) */
void ubx_ver_parse_byte(uint8_t b);

#endif /* UBX_VER_H_ */