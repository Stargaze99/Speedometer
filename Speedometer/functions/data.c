/*
 * data.c
 *
 * Created: 2026-03-24 12:49:51
 *  Author: Admin
 */ 


#include "data.h"

gps_data_t gps_data;

void data_init(void)
{
    /* GPS status */
    gps_data.gps_valid   = 0;
    gps_data.fix_quality = 0;
    gps_data.satellites  = 0;

    /* UTC time */
    gps_data.time.hour   = 0;
    gps_data.time.minute = 0;
    gps_data.time.second = 0;

    /* UTC date */
    gps_data.date.year  = 0;
    gps_data.date.month = 0;
    gps_data.date.day   = 0;

    /* Latitude */
    gps_data.lat.deg       = 0;
    gps_data.lat.min_x1000 = 0;
    gps_data.lat.dir       = '-';

    /* Longitude */
    gps_data.lon.deg       = 0;
    gps_data.lon.min_x1000 = 0;
    gps_data.lon.dir       = '-';

    /* Speed */
    gps_data.speed_kmh_x10 = 0;

    /* Altitude */
    gps_data.altitude_m = 0;

    /* Course */
    gps_data.course_deg = 0;

    /* Debug counters for RMC */
    gps_data.debug.rmc.total_count = 0;
    gps_data.debug.rmc.prev_count  = 0;
    gps_data.debug.rmc.per_second  = 0;

    /* Debug counters for GGA */
    gps_data.debug.gga.total_count = 0;
    gps_data.debug.gga.prev_count  = 0;
    gps_data.debug.gga.per_second  = 0;
}