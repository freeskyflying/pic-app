#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <time.h>


#include "nmea.h"

#define NMEA_TIME_FORMAT    "%H%M%S"
#define NMEA_DATE_FORMAT    "%d%m%y"
#define ANTENNA_OPEN "ANTENNA OPEN"
#define ANTENNA_SHORT "ANTENNA SHORT"
#define ANTENNA_OK "ANTENNA OK"
#define check_pointer(p) if(p-1==NULL)return (-1)
int nmea_parse_gpgga(char *nmea, gpgga_t *loc) {
    char *p = nmea;

    p = strchr(p, ',') + 1; //skip time
    check_pointer(p); 
    strptime(p, NMEA_TIME_FORMAT, &loc->time);

    //loc->valid = (p[0]='A'?1:0);
    p = strchr(p, ',') + 1;
    check_pointer(p);
    loc->latitude = atof(p);

    p = strchr(p, ',') + 1;
    check_pointer(p);
    switch (p[0]) {
        case 'N':
            loc->lat = 'N';
            break;
        case 'S':
            loc->lat = 'S';
            break;
        case ',':
            loc->lat = '\0';
            break;
    }

    p = strchr(p, ',') + 1;
    check_pointer(p);
    loc->longitude = atof(p);


    p = strchr(p, ',') + 1;
    check_pointer(p);
    switch (p[0]) {
        case 'W':
            loc->lon = 'W';
            break;
        case 'E':
            loc->lon = 'E';
            break;
        case ',':
            loc->lon = '\0';
            break;
    }

    p = strchr(p, ',') + 1;
    check_pointer(p);
    loc->quality = (uint8_t) atoi(p);

    p = strchr(p, ',') + 1;
    check_pointer(p);
    loc->satellites = (uint8_t) atoi(p);

    p = strchr(p, ',') + 1;
    check_pointer(p);

    p = strchr(p, ',') + 1;
    check_pointer(p);
    loc->altitude = atof(p);
    return 0;
}


int nmea_parse_gprmc(char *nmea, gprmc_t *loc) {
    char *p = nmea;

    p = strchr(p, ',') + 1;
    check_pointer(p);
    strptime(p, NMEA_TIME_FORMAT, &loc->time);

    p = strchr(p, ',') + 1; //skip status
    check_pointer(p);
    loc->valid = (p[0] == 'A' ? 1 : 0);

    p = strchr(p, ',') + 1;
    check_pointer(p);
//    loc->latitude = atof(p);

    p = strchr(p, ',') + 1;
    check_pointer(p);
//    switch (p[0]) {
//        case 'N':
//            loc->lat = 'N';
//            break;
//        case 'S':
//            loc->lat = 'S';
//            break;
//        case ',':
//            loc->lat = '\0';
//            break;
//    }

    p = strchr(p, ',') + 1;
    check_pointer(p);
//    loc->longitude = atof(p);

    p = strchr(p, ',') + 1;
    check_pointer(p);
//    switch (p[0]) {
//        case 'W':
//            loc->lon = 'W';
//            break;
//        case 'E':
//            loc->lon = 'E';
//            break;
//        case ',':
//            loc->lon = '\0';
//            break;
//    }

    p = strchr(p, ',') + 1;
    check_pointer(p);
    loc->speed = atof(p);

    p = strchr(p, ',') + 1;
    check_pointer(p);
    loc->course = atof(p);
    p = strchr(p, ',') + 1;
    check_pointer(p);
    strptime(p, NMEA_DATE_FORMAT, &loc->time);
    return 0;
}

int nmea_parse_gntxt(char *nmea, uint8_t *status) {
    char *p = nmea;
    p = strchr(p, ',') + 1;
    check_pointer(p);
    p = strchr(p, ',') + 1;//skip text count;
    check_pointer(p);
    p = strchr(p, ',') + 1;//skip text num;
    check_pointer(p);
    p = strchr(p, ',') + 1;//skip text flag;
    check_pointer(p);
    if (!memcmp(p, ANTENNA_OK, strlen(ANTENNA_OK))) {
        *status = 0;
    } else if (!memcmp(p, ANTENNA_OPEN, strlen(ANTENNA_OPEN))) {
        *status = 1;
    } else if (!memcmp(p, ANTENNA_SHORT, strlen(ANTENNA_SHORT))) {
        *status = 2;
    }
    return 0;
}

int nmea_parse_gpgsa(char *nmea, gngsa_t *loc) {
    char *p = nmea;

    p = strchr(p, ',') + 1;//1.skip model.
    check_pointer(p);
    p = strchr(p, ',') + 1;//2.location type.
    check_pointer(p);
    loc->type = atoi(p);
    p = strchr(p, ',') + 1;//3.
    check_pointer(p);
    p = strchr(p, ',') + 1;//4.
    check_pointer(p);
    p = strchr(p, ',') + 1;//5.
    check_pointer(p);
    p = strchr(p, ',') + 1;//6.
    check_pointer(p);
    p = strchr(p, ',') + 1;//7.
    check_pointer(p);
    p = strchr(p, ',') + 1;//8.
    check_pointer(p);
    p = strchr(p, ',') + 1;//9.
    check_pointer(p);
    p = strchr(p, ',') + 1;//10.
    check_pointer(p);
    p = strchr(p, ',') + 1;//11.
    check_pointer(p);
    p = strchr(p, ',') + 1;//12.
    check_pointer(p);
    p = strchr(p, ',') + 1;//13.
    check_pointer(p);
    p = strchr(p, ',') + 1;//14.
    check_pointer(p);
    p = strchr(p, ',') + 1;//pdop
    check_pointer(p);
    loc->pdop = atof(p);
    p = strchr(p, ',') + 1;//hdop
    check_pointer(p);
    loc->hdop = atof(p);
    check_pointer(p);
    p = strchr(p, ',') + 1;//vdop
    check_pointer(p);
    loc->vdop = atof(p);
    return 0;
}

int nmea_parse_gngsa(char *nmea, gngsa_t *loc) {
    char *p = nmea;

    p = strchr(p, ',') + 1;//1.skip model.
    check_pointer(p);
    p = strchr(p, ',') + 1;//2.location type.
    check_pointer(p);
    loc->type = atoi(p);
    p = strchr(p, ',') + 1;//3.
    check_pointer(p);
    p = strchr(p, ',') + 1;//4.
    check_pointer(p);
    p = strchr(p, ',') + 1;//5.
    check_pointer(p);
    p = strchr(p, ',') + 1;//6.
    check_pointer(p);
    p = strchr(p, ',') + 1;//7.
    check_pointer(p);
    p = strchr(p, ',') + 1;//8.
    check_pointer(p);
    p = strchr(p, ',') + 1;//9.
    check_pointer(p);
    p = strchr(p, ',') + 1;//10.
    check_pointer(p);
    p = strchr(p, ',') + 1;//11.
    check_pointer(p);
    p = strchr(p, ',') + 1;//12.
    check_pointer(p);
    p = strchr(p, ',') + 1;//13.
    check_pointer(p);
    p = strchr(p, ',') + 1;//14.
    check_pointer(p);
    p = strchr(p, ',') + 1;//15.
    check_pointer(p);
    p = strchr(p, ',') + 1;//16.
    check_pointer(p);
    p = strchr(p, ',') + 1;//17.
    check_pointer(p);
    p = strchr(p, ',') + 1;//pdop
    check_pointer(p);
    loc->pdop = atof(p);
    p = strchr(p, ',') + 1;//hdop
    check_pointer(p);
    loc->hdop = atof(p);
    p = strchr(p, ',') + 1;//vdop
    check_pointer(p);
    loc->vdop = atof(p);
    return 0;
}

/**
 * Get the message type (GPGGA, GPRMC, etc..)
 *
 * This function filters out also wrong packages (invalid checksum)
 *
 * @param message The NMEA message
 * @return The type of message if it is valid
 */
uint8_t nmea_get_message_type(const char *message) {
    uint8_t checksum = 0;
    if ((checksum = nmea_valid_checksum(message)) != _EMPTY) {
        return checksum;
    }

    if (strstr(message, NMEA_GPGGA_STR) != NULL) {
        return NMEA_GPGGA;
    } else if (strstr(message, NMEA_BDGGA_STR) != NULL) { //same as NMEA_GPGGA_STR and never enter
        return NMEA_BDGGA;
    } else if (strstr(message, NMEA_GLONASS_STR) != NULL) {
        return NMEA_GLONASS;
    } else if (strstr(message, NMEA_GPRMC_STR) != NULL) {
        return NMEA_GPRMC;
    } else if (strstr(message, NMEA_GNTXT_STR) != NULL) {
        return NMEA_GNTXT;
    } else if (strstr(message, NMEA_GNGSA_STR) != NULL) {
        return NMEA_GNGSA;
    }else if (strstr(message, NMEA_GPTXT_STR) != NULL) {
        return NMEA_GPTXT;
    }else if (strstr(message, NMEA_GPGSA_STR) != NULL) {
        return NMEA_GPGSA;
    }
    return NMEA_UNKNOWN;
}

uint8_t nmea_valid_checksum(const char *message) {

    if (message == NULL) {
        return NMEA_CHECKSUM_ERR;
    }


    const char *str = strchr(message, '*');
    if (str == NULL) {
        return NMEA_CHECKSUM_ERR;
    }

    uint8_t checksum = (uint8_t) strtol(strchr(message, '*') + 1, NULL, 16);

    char p;
    uint8_t sum = 0;
    if (strlen(message) < 10) {            //无效数据  $GPGSV,*46
        return NMEA_CHECKSUM_ERR;
    }

    ++message;
    while ((p = *message++) != '*') {
        sum ^= p;
    }

    if (sum != checksum) {
        return NMEA_CHECKSUM_ERR;
    }

    return _EMPTY;
}

