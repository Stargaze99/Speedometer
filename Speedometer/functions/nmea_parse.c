/*
 * nmea_parse.c
 *
 * Created: 2026-03-24 13:00:32
 *  Author: Admin
 */ 

#include "nmea_parse.h"
#include "data.h"

#include <stdint.h>
#include <string.h>

#define NMEA_MAX_FIELDS       20

static uint8_t split_nmea_fields(char *line, char *fields[], uint8_t max_fields);
static uint8_t sentence_is_type(const char *type, const char *suffix);

static uint8_t parse_u8(const char *s);
static uint16_t parse_decimal_x1000(const char *s);
static uint16_t parse_decimal_rounded_u16(const char *s);
static int16_t parse_decimal_rounded_s16(const char *s);

static void parse_time_hms(const char *s, gps_time_t *time);
static void parse_date_dmy(const char *s, gps_date_t *date);
static void parse_lat_dmm(const char *value, const char *dir, coord_dmm_t *lat);
static void parse_lon_dmm(const char *value, const char *dir, coord_dmm_t *lon);

static void parse_rmc_fields(char *fields[], uint8_t count);
static void parse_gga_fields(char *fields[], uint8_t count);

void nmea_parse_line(char *line)
{
    char *fields[NMEA_MAX_FIELDS];
    uint8_t count;

    if (line == 0)
    {
        return;
    }

    count = split_nmea_fields(line, fields, NMEA_MAX_FIELDS);

    if (count == 0)
    {
        return;
    }

    if (sentence_is_type(fields[0], "RMC"))
    {
        parse_rmc_fields(fields, count);
    }
    else if (sentence_is_type(fields[0], "GGA"))
    {
        parse_gga_fields(fields, count);
    }
}

static uint8_t split_nmea_fields(char *line, char *fields[], uint8_t max_fields)
{
    uint8_t count = 0;
    char *p;

    if ((line == 0) || (fields == 0) || (max_fields == 0))
    {
        return 0;
    }

    p = line;

    if (*p == '$')
    {
        p++;
    }

    fields[count++] = p;

    while (*p != '\0')
    {
        if ((*p == ',') || (*p == '*') || (*p == '\r') || (*p == '\n'))
        {
            *p = '\0';

            if (count < max_fields)
            {
                fields[count++] = p + 1;
            }
        }

        p++;
    }

    return count;
}

static uint8_t sentence_is_type(const char *type, const char *suffix)
{
    uint8_t len_type;
    uint8_t len_suffix;

    if ((type == 0) || (suffix == 0))
    {
        return 0;
    }

    len_type = (uint8_t)strlen(type);
    len_suffix = (uint8_t)strlen(suffix);

    if (len_type < len_suffix)
    {
        return 0;
    }

    return (uint8_t)(strcmp(&type[len_type - len_suffix], suffix) == 0);
}

static uint8_t parse_u8(const char *s)
{
    uint16_t value = 0;

    if (s == 0)
    {
        return 0;
    }

    while ((*s >= '0') && (*s <= '9'))
    {
        value = (uint16_t)(value * 10U + (uint16_t)(*s - '0'));
        s++;
    }

    return (uint8_t)value;
}


static uint16_t parse_decimal_x1000(const char *s)
{
    uint32_t integer_part = 0;
    uint16_t frac_part = 0;
    uint8_t frac_count = 0;
    uint8_t round_digit = 0;

    if (s == 0)
    {
        return 0;
    }

    while ((*s >= '0') && (*s <= '9'))
    {
        integer_part = (integer_part * 10UL) + (uint32_t)(*s - '0');
        s++;
    }

    if (*s == '.')
    {
        s++;

        while ((*s >= '0') && (*s <= '9'))
        {
            if (frac_count < 3)
            {
                frac_part = (uint16_t)(frac_part * 10U + (uint16_t)(*s - '0'));
                frac_count++;
            }
            else
            {
                round_digit = (uint8_t)(*s - '0');
                break;
            }

            s++;
        }
    }

    while (frac_count < 3)
    {
        frac_part = (uint16_t)(frac_part * 10U);
        frac_count++;
    }

    if (round_digit >= 5)
    {
        frac_part++;

        if (frac_part >= 1000U)
        {
            frac_part = 0;
            integer_part++;
        }
    }

    return (uint16_t)(integer_part * 1000UL + frac_part);
}

static uint16_t parse_decimal_rounded_u16(const char *s)
{
    uint32_t integer_part = 0;
    uint8_t first_decimal = 0;

    if (s == 0)
    {
        return 0;
    }

    while ((*s >= '0') && (*s <= '9'))
    {
        integer_part = (integer_part * 10UL) + (uint32_t)(*s - '0');
        s++;
    }

    if (*s == '.')
    {
        s++;

        if ((*s >= '0') && (*s <= '9'))
        {
            first_decimal = (uint8_t)(*s - '0');
        }
    }

    if (first_decimal >= 5)
    {
        integer_part++;
    }

    return (uint16_t)integer_part;
}

static int16_t parse_decimal_rounded_s16(const char *s)
{
    int32_t integer_part = 0;
    uint8_t first_decimal = 0;
    int8_t sign = 1;

    if (s == 0)
    {
        return 0;
    }

    if (*s == '-')
    {
        sign = -1;
        s++;
    }

    while ((*s >= '0') && (*s <= '9'))
    {
        integer_part = (integer_part * 10L) + (int32_t)(*s - '0');
        s++;
    }

    if (*s == '.')
    {
        s++;

        if ((*s >= '0') && (*s <= '9'))
        {
            first_decimal = (uint8_t)(*s - '0');
        }
    }

    if (first_decimal >= 5)
    {
        integer_part++;
    }

    return (int16_t)(sign * integer_part);
}

static void parse_time_hms(const char *s, gps_time_t *time)
{
    if ((s == 0) || (time == 0))
    {
        return;
    }

    if (strlen(s) < 6)
    {
        return;
    }

    time->hour   = (uint8_t)(((s[0] - '0') * 10) + (s[1] - '0'));
    time->minute = (uint8_t)(((s[2] - '0') * 10) + (s[3] - '0'));
    time->second = (uint8_t)(((s[4] - '0') * 10) + (s[5] - '0'));
}

static void parse_date_dmy(const char *s, gps_date_t *date)
{
    uint8_t day;
    uint8_t month;
    uint8_t year;

    if ((s == 0) || (date == 0))
    {
        return;
    }

    if (strlen(s) < 6)
    {
        return;
    }

    day   = (uint8_t)(((s[0] - '0') * 10) + (s[1] - '0'));
    month = (uint8_t)(((s[2] - '0') * 10) + (s[3] - '0'));
    year  = (uint8_t)(((s[4] - '0') * 10) + (s[5] - '0'));

    date->year  = (uint16_t)(2000U + year);
    date->month = month;
    date->day   = day;
}

static void parse_lat_dmm(const char *value, const char *dir, coord_dmm_t *lat)
{
    if ((value == 0) || (dir == 0) || (lat == 0))
    {
        return;
    }

    if (strlen(value) < 4)
    {
        return;
    }

    lat->deg = (uint8_t)(((value[0] - '0') * 10) + (value[1] - '0'));
    lat->min_x1000 = parse_decimal_x1000(&value[2]);
    lat->dir = dir[0];
}

static void parse_lon_dmm(const char *value, const char *dir, coord_dmm_t *lon)
{
    if ((value == 0) || (dir == 0) || (lon == 0))
    {
        return;
    }

    if (strlen(value) < 5)
    {
        return;
    }

    lon->deg = (uint8_t)(((value[0] - '0') * 100) +
                         ((value[1] - '0') * 10) +
                         (value[2] - '0'));

    lon->min_x1000 = parse_decimal_x1000(&value[3]);
    lon->dir = dir[0];
}

static void parse_rmc_fields(char *fields[], uint8_t count)
{
    uint32_t speed_knots_x1000;

    if ((fields == 0) || (count < 10))
    {
        return;
    }

    parse_time_hms(fields[1], &gps_data.time);

    if (fields[2][0] == 'A')
    {
        gps_data.gps_valid = 1;
    }
    else if (fields[2][0] == 'V')
    {
        gps_data.gps_valid = 0;
    }

    if (fields[7][0] != '\0')
    {
        speed_knots_x1000 = parse_decimal_x1000(fields[7]);

        gps_data.speed_kmh_x10 = (uint16_t)((speed_knots_x1000 * 1852UL + 50000UL) / 100000UL);

        if (gps_data.speed_kmh_x10 < 10)
        {
            gps_data.speed_kmh_x10 = 0;
        }
    }

    if (fields[8][0] != '\0')
    {
        gps_data.course_deg = parse_decimal_rounded_u16(fields[8]);
    }

    if (fields[9][0] != '\0')
    {
        parse_date_dmy(fields[9], &gps_data.date);
    }
    
    /* Count parsed RMC message */
    gps_data.debug.rmc.total_count++;
    
}

static void parse_gga_fields(char *fields[], uint8_t count)
{
    if ((fields == 0) || (count < 10))
    {
        return;
    }

    gps_data.fix_quality = parse_u8(fields[6]);
    gps_data.satellites  = parse_u8(fields[7]);

    if (gps_data.fix_quality > 0)
    {
        parse_lat_dmm(fields[2], fields[3], &gps_data.lat);
        parse_lon_dmm(fields[4], fields[5], &gps_data.lon);
    }

    if (fields[9][0] != '\0')
    {
        gps_data.altitude_m = parse_decimal_rounded_s16(fields[9]);
    }
    
    /* Count parsed GGA message */
    gps_data.debug.gga.total_count++;
    
}
