/*
 * ubx_ver.c
 *
 * Created: 2026-03-12 10:40:08
 *  Author: Admin
 */ 


#include "ubx_ver.h"
#include "ubx_tx.h"

#include <string.h>

/* Globala buffers */
volatile uint8_t ubx_ver_ready = 0;

char ubx_sw_version[UBX_SW_LEN + 1];
char ubx_hw_version[UBX_HW_LEN + 1];
char ubx_ext[UBX_EXT_MAX][UBX_EXT_LEN + 1];
volatile uint8_t ubx_ext_count = 0;

/* UBX checksum update (8-bit Fletcher) */
static void ck_update(uint8_t b, uint8_t *cka, uint8_t *ckb)
{
    *cka = (uint8_t)(*cka + b);
    *ckb = (uint8_t)(*ckb + *cka);
}

void ubx_ver_poll(void)
{
    /* UBX-MON-VER poll: class 0x0A, id 0x04, len=0, payload=NULL */
    ubx_send(0x0A, 0x04, 0, 0);

    /* reset buffers */
    ubx_ver_ready = 0;
    ubx_ext_count = 0;
    memset(ubx_sw_version, 0, sizeof(ubx_sw_version));
    memset(ubx_hw_version, 0, sizeof(ubx_hw_version));
    memset(ubx_ext, 0, sizeof(ubx_ext));
}

/* Parser state machine */
typedef enum {
    S_SYNC1, S_SYNC2, S_CLASS, S_ID, S_LEN1, S_LEN2, S_PAYLOAD, S_CKA, S_CKB
} st_t;

static st_t st = S_SYNC1;

static uint8_t cls = 0, id = 0;
static uint16_t len = 0, pos = 0;
static uint8_t cka = 0, ckb = 0;
static uint8_t rx_cka = 0, rx_ckb = 0;
static uint8_t is_monver = 0;

static void reset_parser(void)
{
    st = S_SYNC1;
    cls = 0; id = 0;
    len = 0; pos = 0;
    cka = 0; ckb = 0;
    rx_cka = 0; rx_ckb = 0;
    is_monver = 0;
}

static void store_monver_byte(uint16_t p, uint8_t b)
{
    /* Payload layout:
       0..29  swVersion (30)
       30..39 hwVersion (10)
       40..   extension blocks (30 bytes each)
    */
    if (p < UBX_SW_LEN) {
        ubx_sw_version[p] = (char)b;
    } else if (p < (UBX_SW_LEN + UBX_HW_LEN)) {
        ubx_hw_version[p - UBX_SW_LEN] = (char)b;
    } else if (p >= 40) {
        uint16_t epos = p - 40;
        uint8_t ei = (uint8_t)(epos / UBX_EXT_LEN);
        uint8_t ej = (uint8_t)(epos % UBX_EXT_LEN);

        if (ei < UBX_EXT_MAX) {
            ubx_ext[ei][ej] = (char)b;
            if ((uint8_t)(ei + 1) > ubx_ext_count) ubx_ext_count = (uint8_t)(ei + 1);
        }
    }
}

static void finalize_strings(void)
{
    ubx_sw_version[UBX_SW_LEN] = '\0';
    ubx_hw_version[UBX_HW_LEN] = '\0';
    for (uint8_t i = 0; i < UBX_EXT_MAX; i++) {
        ubx_ext[i][UBX_EXT_LEN] = '\0';
    }
}

void ubx_ver_parse_byte(uint8_t b)
{
    switch (st) {
        case S_SYNC1:
            if (b == 0xB5) st = S_SYNC2;
            break;

        case S_SYNC2:
            st = (b == 0x62) ? S_CLASS : S_SYNC1;
            break;

        case S_CLASS:
            cls = b;
            cka = 0; ckb = 0;
            ck_update(b, &cka, &ckb);
            st = S_ID;
            break;

        case S_ID:
            id = b;
            ck_update(b, &cka, &ckb);
            st = S_LEN1;
            break;

        case S_LEN1:
            len = b;
            ck_update(b, &cka, &ckb);
            st = S_LEN2;
            break;

        case S_LEN2:
            len |= ((uint16_t)b << 8);
            ck_update(b, &cka, &ckb);

            pos = 0;
            is_monver = (cls == 0x0A && id == 0x04) ? 1 : 0;

            st = (len == 0) ? S_CKA : S_PAYLOAD;
            break;

        case S_PAYLOAD:
            if (is_monver) {
                store_monver_byte(pos, b);
            }
            ck_update(b, &cka, &ckb);
            pos++;
            if (pos >= len) st = S_CKA;
            break;

        case S_CKA:
            rx_cka = b;
            st = S_CKB;
            break;

        case S_CKB:
            rx_ckb = b;

            if (rx_cka == cka && rx_ckb == ckb) {
                if (is_monver && len >= 40) {
                    finalize_strings();
                    ubx_ver_ready = 1;
                }
            }
            reset_parser();
            break;

        default:
            reset_parser();
            break;
    }
}