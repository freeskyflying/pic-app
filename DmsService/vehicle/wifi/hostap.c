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

void configHostAp(char* ssid, char* passwd)
{
    char conntent[1024] = { 0 };

    if (access(CTRL_INTERFACE_PATH, F_OK) == -1)
    {
        mkdir(CTRL_INTERFACE_PATH, 0660);
    }

    int fd = open(HOSTAPD_CONF_FILE, O_CREAT | O_TRUNC | O_WRONLY | O_NOFOLLOW, 0660);
    if (fd < 0)
    {
        return;
    }

    sprintf(conntent, "interface=wlan0\ndriver=nl80211\nctrl_interface="
            CTRL_INTERFACE_PATH"\nssid=%s\nchannel=6\nieee80211n=1\n"
            "hw_mode=g\nwpa_passphrase=%s\nwpa=2\nwpa_key_mgmt=WPA-PSK\n"
            "max_num_sta=5\nrsn_pairwise=CCMP TKIP\nwpa_pairwise=TKIP CCMP\n", ssid, passwd);
    
    write(fd, conntent, strlen(conntent));
    
    fchmod(fd, 0660);

    close(fd);

    
    fd = open(UDHCPD_CONF_FILE, O_CREAT | O_TRUNC | O_WRONLY | O_NOFOLLOW, 0660);
    if (fd < 0)
    {
        return;
    }

    sprintf(conntent, "start 192.168.169.2\nend 192.168.169.254\ninterface wlan0\nmax_leases 234\nopt router 192.168.169.1");
    
    write(fd, conntent, strlen(conntent));
    
    fchmod(fd, 0660);

    close(fd);    
}

int checkWifiInterface(void)
{
    int exist = 0;
    FILE* fp=NULL;
    int BUFSZ=100;
    char buf[BUFSZ];
    char command[150];

    sprintf(command, "ifconfig -a | grep wlan0");

    if((fp = popen(command,"r")) == NULL)
    {
        return 0;
    }

    if( (fgets(buf,BUFSZ,fp))!= NULL )
    {
        exist = 1;
    }

    pclose(fp);

    return exist;
}

void WifiReboot(void)
{
	char cmd[128] = { 0 };

	int hostapdPid = getProcessPID("/usr/bin/hostapd", "/data/etc/hostapd.conf");

	if (-1 != hostapdPid)
	{
		HOSTAP_DBG("kill hostapd pid(%d).\n", hostapdPid);

		sprintf(cmd, "kill -9 %d", hostapdPid);
		system(cmd);
	}
	
	int udhcpdPid = getProcessPID("/data/etc/udhcpd.conf", "udhcpd /data/etc/udhcpd.conf");

	if (-1 != udhcpdPid)
	{
		HOSTAP_DBG("kill udhcpd pid(%d).\n", udhcpdPid);

		sprintf(cmd, "kill -9 %d", udhcpdPid);
		system(cmd);
	}
	
	HOSTAP_DBG("start wifi %s %s.\n", gGlobalSysParam->wifiSSID, gGlobalSysParam->wifiPswd);

    configHostAp(gGlobalSysParam->wifiSSID, gGlobalSysParam->wifiPswd);
	
    if (access(HOSTAPD_CONF_FILE, F_OK) != -1)
    {
        system("/usr/bin/hostapd -B "HOSTAPD_CONF_FILE);
        system("udhcpd "UDHCPD_CONF_FILE);
    }
}

void* hostApdTask(void* arg)
{
    while (1)
    {
        if (checkWifiInterface())
        {
            system("ifconfig wlan0 192.168.169.1 netmask 255.255.255.0 up");
            break;
        }
    }

    HOSTAP_DBG("start hostapd.\n");

	WifiReboot();

    while (1)
    {
        sleep(1);
    }

    return NULL;
}

void hostApdStart(void)
{
    pthread_t pid;

    int ret = pthread_create(&pid, 0, hostApdTask, NULL);
    if (ret)
    {
        printf("errno=%d, reason(%s)\n", errno, strerror(errno));
    }
}

