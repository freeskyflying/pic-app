#ifndef _GPIO_LED_H_
#define _GPIO_LED_H_

#define NET4G_LED       "/sys/class/leds/led4"

#define GPS_LED         "/sys/class/leds/led3"

#define REC_LED1        "/sys/class/leds/rgbled1"
#define REC_LED2        "/sys/class/leds/rgbled2"

void setLedStatus(char* path, char brightness);

#endif
