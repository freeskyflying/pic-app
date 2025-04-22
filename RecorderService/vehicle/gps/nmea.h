//
// Created by fengyin on 20-3-9.
//

#ifndef _NMEA_H_
#define _NMEA_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include "typedef.h"

#if 0
#define NMEA_GPRMC_STR		"$GNRMC"
#define NMEA_GNTXT_STR		"$GNTXT"
#define NMEA_GPGGA_STR		"$GNGGA"
#define NMEA_BDGGA_STR		"$BDGGA"
#define NMEA_GLONASS_STR	"$GLONASS"
#define NMEA_GNGSA_STR      "$GNGSA"
#define NMEA_GPTXT_STR       "$GPTXT"
#define NMEA_GPGSA_STR       "$GPGSA"
#else
#define NMEA_GPRMC_STR      "RMC"
#define NMEA_GNTXT_STR      "GNTXT"
#define NMEA_GPGGA_STR      "GGA"
#define NMEA_BDGGA_STR      "GGA"
#define NMEA_GLONASS_STR    "$GLONASS"
#define NMEA_GNGSA_STR      "GSA"
#define NMEA_GPTXT_STR       "GPTXT"
#define NMEA_GPGSA_STR       "$GPGSA"

#endif

//type
#define NMEA_UNKNOWN	0x00
#define NMEA_GPRMC		0x01
#define NMEA_GPGGA		0x02
#define NMEA_GNTXT		0x03
#define NMEA_BDGGA		0x04
#define NMEA_GLONASS	0x05
#define NMEA_GPGSA      0x06
#define NMEA_GPTXT      0x07
#define NMEA_GNGSA      0x08

#define _EMPTY 0x00
#define _COMPLETED 0x03
#define NMEA_CHECKSUM_ERR 0x80
#define NMEA_MESSAGE_ERR 0xC0

struct gngsa{
    /**
     * 1:no location.
     * 2:2D location.
     * 3:3D location.
     */
    uint8_t type;
    float pdop;
    float hdop;
    float vdop;
};
typedef struct gngsa gngsa_t;

struct gpgga {
    // Latitude eg: 4124.8963 (XXYY.ZZKK.. DEG, MIN, SEC.SS)
    double latitude;
    // Latitude eg: N
    char lat;
    // Longitude eg: 08151.6838 (XXXYY.ZZKK.. DEG, MIN, SEC.SS)
    double longitude;
    // Longitude eg: W
    char lon;
    // Quality 0, 1, 2
    uint8_t quality;
    // Number of satellites: 1,2,3,4,5...
    uint8_t satellites;
    // Altitude eg: 280.2 (Meters above mean sea level)
    double altitude;

    struct tm time;
};
typedef struct gpgga gpgga_t;



struct gprmc {
    bool_t valid;
    double latitude;
    char lat;
    double longitude;
    char lon;
    double speed;
    double course;
    struct tm time;
};
typedef struct gprmc gprmc_t;

uint8_t nmea_get_message_type(const char *);
uint8_t nmea_valid_checksum(const char *);
int nmea_parse_gpgga(char *, gpgga_t *);
int nmea_parse_gprmc(char *, gprmc_t *);
int nmea_parse_gntxt(char*, uint8_t *);
int nmea_parse_gpgsa(char*, gngsa_t *);
int nmea_parse_gngsa(char*, gngsa_t *);

#endif
