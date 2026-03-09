/*
 * ubx.h
 *
 * Created: 2026-03-09 11:30:33
 *  Author: Admin
 */ 


#ifndef UBX_H_
#define UBX_H_

#include <stdint.h>

/* ------------------------------------------------------------
   Globala variabler (enkla att watch:a i debuggern)
   NAV-ATT skalar vinklar i 1e-5 grader.
   ------------------------------------------------------------ */
extern volatile int32_t  g_roll_1e5;
extern volatile int32_t  g_pitch_1e5;
extern volatile int32_t  g_yaw_1e5;       /* yaw ≈ heading  */
extern volatile uint32_t g_att_iTOW_ms;   /* GPS time of week (ms) */
extern volatile uint8_t  g_att_valid;     /* 1 när värden uppdaterats */

/* ------------------------------------------------------------
   API
   ------------------------------------------------------------ */

/* Valfritt: stäng av NMEA (GGA/GLL/GSA/GSV/RMC/VTG) för att få mer “luft” för UBX */
void ubx_disable_common_nmea(void);

/* Aktivera UBX-NAV-ATT (roll/pitch/heading) med rate (t.ex. 1) */
void ubx_init_enable_nav_att(uint8_t rate);

/* Mata in en mottagen byte från UART */
void ubx_parse_byte(uint8_t b);

/* ------------------------------------------------------------
   Små hjälpfunktioner (utan float)
   ------------------------------------------------------------ */

/* 1e-5 deg -> heltal grader (avrundat) */
static inline int16_t ubx_deg_from_1e5(int32_t v_1e5)
{
    if (v_1e5 >= 0) v_1e5 += 50000;
    else            v_1e5 -= 50000;
    return (int16_t)(v_1e5 / 100000);
}

/* 1e-5 deg -> grader*100 (x.xx), avrundat.
   OBS: resultat kan nå ~36000 för heading, så använd helst int32_t om du vill lagra det. */
static inline int32_t ubx_x100_from_1e5(int32_t v_1e5)
{
    if (v_1e5 >= 0) v_1e5 += 500;
    else            v_1e5 -= 500;
    return (int32_t)(v_1e5 / 1000);
}

#endif /* UBX_H_ */