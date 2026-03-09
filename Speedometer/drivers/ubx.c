/*
 * ubx.c
 *
 * Created: 2026-03-09 10:32:38
 *  Author: Admin
 */ 

#include "ubx.h"
#include "uart.h"

#include <string.h>

/*Globala variabler för testing*/
volatile int32_t g_roll_1e5  = 0;
volatile int32_t g_pitch_1e5 = 0;
volatile int32_t g_yaw_1e5   = 0; /* yaw ≈ heading */
volatile uint32_t g_att_iTOW_ms = 0;
volatile uint8_t  g_att_valid   = 0; /* 1 när senaste värdena är uppdaterade */

/* ------------------------------------------------------------
   UBX checksum (CK_A / CK_B)
   Checksum beräknas över: class, id, lenL, lenH, payload...
   (inte över sync-bytes 0xB5 0x62)
   ------------------------------------------------------------ */
static void ubx_ck_update(uint8_t b, uint8_t *ck_a, uint8_t *ck_b)
{
    *ck_a = (uint8_t)(*ck_a + b);
    *ck_b = (uint8_t)(*ck_b + *ck_a);
}

/* ------------------------------------------------------------
   Skicka ett UBX-paket
   Format:
     B5 62 <class> <id> <lenL> <lenH> <payload...> <CK_A> <CK_B>
   ------------------------------------------------------------ */
static void ubx_send(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len)
{
    uint8_t ck_a = 0;
    uint8_t ck_b = 0;

    /* Sync */
    uart0_write(0xB5);
    uart0_write(0x62);

    /* Header */
    uart0_write(cls); ubx_ck_update(cls, &ck_a, &ck_b);
    uart0_write(id);  ubx_ck_update(id,  &ck_a, &ck_b);

    uart0_write((uint8_t)(len & 0xFF)); ubx_ck_update((uint8_t)(len & 0xFF), &ck_a, &ck_b);
    uart0_write((uint8_t)(len >> 8));   ubx_ck_update((uint8_t)(len >> 8),   &ck_a, &ck_b);

    /* Payload */
    for (uint16_t i = 0; i < len; i++) {
        uart0_write(payload[i]);
        ubx_ck_update(payload[i], &ck_a, &ck_b);
    }

    /* Checksum */
    uart0_write(ck_a);
    uart0_write(ck_b);
}

/* ------------------------------------------------------------
   UBX-CFG-MSG (0x06 0x01)
   Variant "current port": payload = { msgClass, msgID, rate } (3 bytes)

   Ex:
   - Aktivera NAV-ATT: msgClass=0x01 (NAV), msgID=0x05 (ATT), rate=1
   ------------------------------------------------------------ */
static void ubx_cfg_msg_rate_current_port(uint8_t msgClass, uint8_t msgID, uint8_t rate)
{
    uint8_t p[3];
    p[0] = msgClass;
    p[1] = msgID;
    p[2] = rate;
    ubx_send(0x06, 0x01, p, 3);
}

/* ------------------------------------------------------------
   stäng av vanliga NMEA-meningar så UBX får mer bandbredd.
   u-blox ha NMEA som "class 0xF0" med dessa IDs:
     0x00 GGA, 0x01 GLL, 0x02 GSA, 0x03 GSV, 0x04 RMC, 0x05 VTG
   ------------------------------------------------------------ */
void ubx_disable_common_nmea(void)
{
    ubx_cfg_msg_rate_current_port(0xF0, 0x00, 0);
    ubx_cfg_msg_rate_current_port(0xF0, 0x01, 0);
    ubx_cfg_msg_rate_current_port(0xF0, 0x02, 0);
    ubx_cfg_msg_rate_current_port(0xF0, 0x03, 0);
    ubx_cfg_msg_rate_current_port(0xF0, 0x04, 0);
    ubx_cfg_msg_rate_current_port(0xF0, 0x05, 0);
}

/* ------------------------------------------------------------
   Init: slå på NAV-ATT med önskad rate (t.ex. 1)
   ------------------------------------------------------------ */
void ubx_init_enable_nav_att(uint8_t rate)
{
    /* NAV-ATT = class 0x01, id 0x05 */
    ubx_cfg_msg_rate_current_port(0x01, 0x05, rate); 
}

/* ------------------------------------------------------------
   Little-endian helpers
   ------------------------------------------------------------ */
static int32_t le_i32(const uint8_t *p)
{
    return (int32_t)(
    ((uint32_t)p[0]) |
    ((uint32_t)p[1] << 8) |
    ((uint32_t)p[2] << 16) |
    ((uint32_t)p[3] << 24)
    );
}

static uint32_t le_u32(const uint8_t *p)
{
    return (uint32_t)(
    ((uint32_t)p[0]) |
    ((uint32_t)p[1] << 8) |
    ((uint32_t)p[2] << 16) |
    ((uint32_t)p[3] << 24)
    );
}

/* ------------------------------------------------------------
   UBX Parser (state machine)
   Vi läser byte-för-byte från UART och försöker bygga ett UBX-paket.
   ------------------------------------------------------------ */
#define UBX_PAYLOAD_MAX 64

typedef enum {
    S_SYNC1,
    S_SYNC2,
    S_CLASS,
    S_ID,
    S_LEN1,
    S_LEN2,
    S_PAYLOAD,
    S_CK_A,
    S_CK_B
} ubx_state_t;

static ubx_state_t st = S_SYNC1;

static uint8_t  msg_class = 0;
static uint8_t  msg_id    = 0;
static uint16_t payload_len = 0;
static uint16_t payload_pos = 0;

static uint8_t ck_a = 0, ck_b = 0;
static uint8_t rx_ck_a = 0, rx_ck_b = 0;

static uint8_t payload[UBX_PAYLOAD_MAX];

static void ubx_reset_parser(void)
{
    st = S_SYNC1;
    msg_class = 0;
    msg_id = 0;
    payload_len = 0;
    payload_pos = 0;
    ck_a = 0;
    ck_b = 0;
}

/* ------------------------------------------------------------
   Hantera UBX-meddelanden vi bryr oss om
   NAV-ATT layout (len = 32):
     0..3  iTOW (ms)
     4     version
     5..7  reserved
     8..11 roll    (1e-5 deg)
     12..15 pitch  (1e-5 deg)
     16..19 heading(1e-5 deg)  -> yaw ≈ heading
     ... (resten finns men vi behöver inte här)
   ------------------------------------------------------------ */
static void ubx_handle(uint8_t cls, uint8_t id, const uint8_t *pl, uint16_t len)
{
    if (cls == 0x01 && id == 0x05 && len == 32) {
        g_att_iTOW_ms = le_u32(&pl[0]);
        g_roll_1e5    = le_i32(&pl[8]);
        g_pitch_1e5   = le_i32(&pl[12]);
        g_yaw_1e5     = le_i32(&pl[16]); /* heading */
        g_att_valid   = 1;
    }
}

/* ------------------------------------------------------------
   Public API: mata in en mottagen byte härifrån din main-loop.
   ------------------------------------------------------------ */
void ubx_parse_byte(uint8_t b)
{
    switch (st) {

        case S_SYNC1:
            if (b == 0xB5) {
                st = S_SYNC2;
            }
            break;

        case S_SYNC2:
            if (b == 0x62) {
                st = S_CLASS;
            } else {
                st = S_SYNC1;
            }
            break;

        case S_CLASS:
            msg_class = b;
            ck_a = 0;
            ck_b = 0;
            ubx_ck_update(b, &ck_a, &ck_b);
            st = S_ID;
            break;

        case S_ID:
            msg_id = b;
            ubx_ck_update(b, &ck_a, &ck_b);
            st = S_LEN1;
            break;

        case S_LEN1:
            payload_len = b;
            ubx_ck_update(b, &ck_a, &ck_b);
            st = S_LEN2;
            break;

        case S_LEN2:
            payload_len |= ((uint16_t)b << 8);
            ubx_ck_update(b, &ck_a, &ck_b);

            payload_pos = 0;

            if (payload_len > UBX_PAYLOAD_MAX) {
                /* För långt payload: droppa paketet */
                ubx_reset_parser();
            } else {
                st = (payload_len == 0) ? S_CK_A : S_PAYLOAD;
            }
            break;

        case S_PAYLOAD:
            payload[payload_pos++] = b;
            ubx_ck_update(b, &ck_a, &ck_b);

            if (payload_pos >= payload_len) {
                st = S_CK_A;
            }
            break;

        case S_CK_A:
            rx_ck_a = b;
            st = S_CK_B;
            break;

        case S_CK_B:
            rx_ck_b = b;

            if (rx_ck_a == ck_a && rx_ck_b == ck_b) {
                ubx_handle(msg_class, msg_id, payload, payload_len);
            }

            ubx_reset_parser();
            break;

        default:
            ubx_reset_parser();
            break;
    }
}