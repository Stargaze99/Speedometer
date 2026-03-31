/*
 * data.h
 *
 * Created: 2026-03-24 13:29:37
 *  Author: Admin
 */ 


#ifndef DATA_H
#define DATA_H

#include <stdint.h>

typedef struct
{
    /* Exempel: 14:37:52 */
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

} gps_time_t;

typedef struct
{
    /* Exempel: 2026.03.24 */
    uint16_t year;
    uint8_t month;
    uint8_t day;

} gps_date_t;

typedef struct
{
    /* DMM format */
    /* Exempel: 57 deg 43.123 min N */
    uint8_t  deg;
    uint16_t min_x1000;
    char     dir;

} coord_dmm_t;

typedef struct
{
    /* Total number of parsed messages */
    uint32_t total_count;

    /* Previous total count, used later by timer */
    /* to calculate messages per second */
    uint32_t prev_count;

    /* Number of messages received during last 1 second */
    uint8_t per_second;

} nmea_count_t;

typedef struct
{
    /* Total number of main loop cycles */
    uint32_t total_count;

    /* Previous total count, used to calculate cycles per second */
    uint32_t prev_count;

    /* Number of main loop cycles during last 1 second */
    uint32_t per_second;

} loop_count_t;

typedef struct
{
    /* Separate debug counters for each message type */
    nmea_count_t rmc;
    nmea_count_t gga;
    
    /* Main loop cycle counter */
    loop_count_t main_loop;
} gps_debug_t;

typedef struct
{
    /* GPS status */
    uint8_t gps_valid;
    uint8_t fix_quality;
    uint8_t satellites;

    /* UTC time and date */
    gps_time_t time;
    gps_date_t date;

    /* Position */
    coord_dmm_t lat;
    coord_dmm_t lon;

    /* Speed */
    /* 123 = 12.3 km/h */
    uint16_t speed_kmh_x10;

    /* Altitude in meters */
    int16_t altitude_m;

    /* Course in degrees */
    uint16_t course_deg;
    
    gps_debug_t debug;

} gps_data_t;

extern gps_data_t gps_data;

void data_init(void);

#endif