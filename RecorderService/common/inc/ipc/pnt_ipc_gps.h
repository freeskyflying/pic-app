#ifndef _PNT_IPC_GPS_H_
#define _PNT_PIC_GPS_H_

#include "typedef.h"

#define PNT_IPC_GPS_NAME        "gps_ipc"

/** Milliseconds since January 1, 1970 */
typedef int64_t GpsUtcTime;


/** Represents a location. */
typedef struct {
    /** set to sizeof(GpsLocation) */
    size_t          size;
    /** Contains GpsLocationFlags bits. */
    uint16_t        flags;
    /** Represents latitude in degrees. */
    double          latitude;
    /** Represents longitude in degrees. */
    double          longitude;
    /** Represents altitude in meters above the WGS 84 reference
     * ellipsoid. */
    double          altitude;
    /** Represents speed in meters per second. */
    float           speed;
    /** Represents heading in degrees. */
    float           bearing;
    /** Represents expected accuracy in meters. */
    float           accuracy;
    /** Timestamp for the location fix. */
    GpsUtcTime      timestamp;

    uint8_t valid; //location valid. 1:valid; 0:invalid
    char lat;
    char lon;
    // Quality 0, 1, 2
    uint8_t quality;
    // Number of satellites: 1,2,3,4,5...
    uint8_t satellites;

    uint8_t antenna;//0 normaled, 1 opened, 2 shorted,

    /**
     * 0.5-99.9
     */
    float pdop;
    /**
     * 0.5-99.9
     */
    float hdop;
    /**
     * 0.5-99.9
     */
    float vdop;
    /**
     * Location
     * 1:no location.
     * 2:2D location.
     * 3:3D location.
     */
    uint8_t locationtype;
} GpsLocation_t;


#endif
