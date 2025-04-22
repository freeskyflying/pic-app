#ifndef _WIFI_H_
#define _WIFI_H_

#define HOSTAPD_CONF_FILE       "/data/etc/hostapd.conf"
#define CTRL_INTERFACE_PATH     "/data/etc/hostapd"
#define UDHCPD_CONF_FILE        "/data/etc/udhcpd.conf"

#define STATION_CONF_FILE		"/data/etc/wpa_supplicant.conf"


void WifiApReboot(int reboot);

void WifiStationReboot(int reboot);

void wifiStart(void);

void wifiStop(void);

#endif
