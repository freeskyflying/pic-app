#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>
#include <stdarg.h>
#include <assert.h>

#include "gpsReader.h"
#include "utils.h"
#include "rild.h"
#include "gpio_led.h"
#include "upgrade.h"

#include "wifi.h"

extern BoardSysInfo_t gBoardInfo;

int vehicle_main()
{
    setLedStatus(NET4G_LED, 0);
    setLedStatus(GPS_LED, 0);
    setLedStatus(REC_LED1, 0);
    setLedStatus(REC_LED2, 0);

	hostApdStart();

	return 0;
}
