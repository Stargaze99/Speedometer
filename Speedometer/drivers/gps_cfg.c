/*
 * gps_cfg.c
 *
 * Created: 2026-03-11 13:08:56
 *  Author: Admin
 */ 

#include "gps_cfg.h"


/* u-blox NMEA message class */
#define UBX_NMEA_CLASS  0xF0

/* u-blox NMEA message IDs (msgID när msgClass = 0xF0)
   0x00 = GGA  Global Positioning System Fix Data (tid, fix quality, alt)
   0x01 = GLL  Geographic Position – Latitude/Longitude
   0x02 = GSA  GNSS DOP and Active Satellites (fix type, DOP)
   0x03 = GSV  GNSS Satellites in View (långa, många rader)
   0x04 = RMC  Recommended Minimum (tid, datum, speed, course)
   0x05 = VTG  Course Over Ground and Ground Speed
*/
#define NMEA_GGA_ID 0x00
#define NMEA_GLL_ID 0x01
#define NMEA_GSA_ID 0x02
#define NMEA_GSV_ID 0x03
#define NMEA_RMC_ID 0x04
#define NMEA_VTG_ID 0x05

/*
static void ubx_checksum(uint8_t a, uint8_t *cka, uint8_t *ckb){
    
    *cka = (uint8_t) (*cka + a);
    *ckb = (uint8_t) (*cka + *ckb);
}*/
/*

/ *Exmpel UBX: B5 62 <CLS> <ID> <LEN_L> <LEN_H> <PAYLOAD...> <CK_A> <CK_B>* /
static void ubx_send(uint8_t cls, uint8_t id, uint16_t len, const uint8_t *payload){
    
    / *init checksum* /
    uint8_t cka = 0;
    uint8_t ckb = 0;
    
    / *Start sync* /
    uart0_write(0xB5);
    uart0_write(0x62);
    
    uart0_write(cls);
    ubx_checksum(cls, &cka, &ckb);
    
    uart0_write(id);
    ubx_checksum(id, &cka, &ckb);
    
    / *Låg* /
    uart0_write((uint8_t) (len & 0xFF));
    ubx_checksum((uint8_t) (len & 0xFF), &cka, &ckb);
    
    / *hög* /
    uart0_write((uint8_t) (len >> 8));
    ubx_checksum((uint8_t) (len >> 8), &cka, &ckb);
    
    for (uint16_t i = 0; i < len; i++)
    {
        uart0_write(payload[i]);
        ubx_checksum(payload[i], &cka, &ckb);
    }
    
    uart0_write(cka);
    uart0_write(ckb);
    
}*/


static void ubx_cfg_msg(uint8_t msgClass, uint8_t msgID, uint8_t rate)
{
    uint8_t p[3] = { msgClass, msgID, rate };
    ubx_send(0x06, 0x01, 3, p);
}

void ubx_ver_check(void){
    /* UBX-MON-VER poll: class 0x0A id 0x04, length=0, no payload */
    ubx_send(0x0A, 0x04, 0, 0);
}

static void ubx_cfg_rate(uint16_t measRate, uint16_t navRate, uint16_t timeRef){
    uint8_t p[6];

    p[0] = (uint8_t)(measRate & 0xFF);
    p[1] = (uint8_t)(measRate >> 8);
    p[2] = (uint8_t)(navRate & 0xFF);
    p[3] = (uint8_t)(navRate >> 8);
    p[4] = (uint8_t)(timeRef & 0xFF);
    p[5] = (uint8_t)(timeRef >> 8);

    ubx_send(0x06, 0x08, 6, p);
}

void gps_cfg_profile(uint8_t profile){
    
    if (profile == 1)
    {
        /* 1) Stäng av "spam" först så UART blir lugn */
        ubx_cfg_msg(UBX_NMEA_CLASS, NMEA_GSV_ID, 0); /* GSV OFF */
        ubx_cfg_msg(UBX_NMEA_CLASS, NMEA_GSA_ID, 0); /* GSA OFF */
        ubx_cfg_msg(UBX_NMEA_CLASS, NMEA_GLL_ID, 0); /* GLL OFF */
        ubx_cfg_msg(UBX_NMEA_CLASS, NMEA_VTG_ID, 0); /* VTG OFF */

        /* 2) Slå på de meningar vi faktiskt vill använda */
        ubx_cfg_msg(UBX_NMEA_CLASS, NMEA_RMC_ID, 1); /* RMC ON */
        ubx_cfg_msg(UBX_NMEA_CLASS, NMEA_GGA_ID, 1); /* GGA ON */
        
        /*2 Hz*/
        ubx_cfg_rate(500,1,1);
    }
}

