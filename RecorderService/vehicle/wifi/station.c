#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "utils.h"
#include "wifi.h"
#include "system_param.h"
#include "pnt_log.h"

#define HOSTAP_DBG      PNT_LOGI

void configStation(char* ssid, char* passwd)
{
    char conntent[1024] = { 0 };

    int fd = open(STATION_CONF_FILE, O_CREAT | O_TRUNC | O_WRONLY | O_NOFOLLOW, 0660);
    if (fd < 0)
    {
        return;
    }

    sprintf(conntent, "ctrl_interface=/var/run/wpa_supplicant\n"
						"network={\n"
						"	ssid=\"%s\"\n"
						"	psk=\"%s\"\n"
						"}", ssid, passwd);
    
    write(fd, conntent, strlen(conntent));
    
    fchmod(fd, 0660);

    close(fd);
}

void WifiStationReboot(int reboot)
{
	char cmd[128] = { 0 };

	int stationPid = getProcessPID("wpa_supplicant", "wpa_supplicant.conf");

	if (-1 != stationPid)
	{
		HOSTAP_DBG("kill wpa_supplicant pid(%d).\n", stationPid);

		sprintf(cmd, "kill -9 %d", stationPid);
		system(cmd);
	}
	
	int udhcpdPid = getProcessPID("wlan0", "udhcpc -i wlan0");

	if (-1 != udhcpdPid)
	{
		HOSTAP_DBG("kill udhcpc -i wlan0 pid(%d).\n", udhcpdPid);

		sprintf(cmd, "kill -9 %d", udhcpdPid);
		system(cmd);
	}

	if (reboot)
	{
		HOSTAP_DBG("start wifi %s %s.\n", gGlobalSysParam->wifiSSIDEx, gGlobalSysParam->wifiPswdEx);

	    configStation(gGlobalSysParam->wifiSSIDEx, gGlobalSysParam->wifiPswdEx);
		
	    if (access(STATION_CONF_FILE, F_OK) != -1)
	    {
	        system("wpa_supplicant -i wlan0 -B -c "STATION_CONF_FILE);
	        system("udhcpc -i wlan0 & ");
	    }
	}
}


