#include "common_test.h"
#include "cJSON.h"

#define HOSTAP_DBG      PNT_LOGI

#define HOSTAPD_CONF_FILE       "/data/etc/hostapd.conf"
#define CTRL_INTERFACE_PATH     "/data/etc/hostapd"
#define UDHCPD_CONF_FILE        "/data/etc/udhcpd.conf"

#define STATION_CONF_FILE		"/data/etc/wpa_supplicant.conf"

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
            "hw_mode=g\nwpa_passphrase=%s\nwpa=0\nwpa_key_mgmt=WPA-PSK\n"
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

void WifiApReboot(int reboot, char* ssid, char* pswd)
{
	char cmd[128] = { 0 };

	int hostapdPid = getProcessPID("/usr/bin/hostapd", "/data/etc/hostapd.conf");

	if (-1 != hostapdPid)
	{
		HOSTAP_DBG("kill hostapd pid(%d).\n", hostapdPid);

		sprintf(cmd, "kill -9 %d", hostapdPid);
		system(cmd);
	}
	
	int udhcpdPid = getProcessPID("/usr/sbin/udhcpd", "/data/etc/udhcpd.conf");

	if (-1 != udhcpdPid)
	{
		HOSTAP_DBG("kill udhcpd pid(%d).\n", udhcpdPid);

		sprintf(cmd, "kill -9 %d", udhcpdPid);
		system(cmd);
	}

	if (reboot)
	{
        system("ifconfig wlan0 192.168.169.1 netmask 255.255.255.0 up");
			
		HOSTAP_DBG("start wifi %s %s.\n", ssid, pswd);

	    configHostAp(ssid, pswd);
		
	    if (access(HOSTAPD_CONF_FILE, F_OK) != -1)
	    {
	        system("/usr/bin/hostapd -B "HOSTAPD_CONF_FILE);
	        system("/usr/sbin/udhcpd "UDHCPD_CONF_FILE);
	    }
	}
}

cJSON * parseCJSONFile(char* filename)
{
	FILE *cfg_fd;
	long len;
	char *data = NULL;
	cJSON *json = NULL;

	cfg_fd = fopen(filename, "rb");
	if (cfg_fd == NULL)
	{
		HOSTAP_DBG("open file failed %s", filename);
		return NULL;
	}
	fseek(cfg_fd, 0, SEEK_END);
	len = ftell(cfg_fd);
	fseek(cfg_fd, 0, SEEK_SET);

	data = (char *)malloc(len + 1);

	if (data)
	{
		memset(data, 0, len + 1);
		fread(data, 1, len, cfg_fd);
		data[len] = '\0';
		json = cJSON_Parse(data);
		if (!json)
		{
			HOSTAP_DBG("Error before: [%s]", cJSON_GetErrorPtr());
		}
	}
	else
	{
		HOSTAP_DBG("malloc failed.");
		return NULL;
	}
	
	free(data);

	fclose(cfg_fd);
	
	return json;
}

void* WifiTestThread(void* pArg)
{
	pthread_detach(pthread_self());

	system("insmod /lib/modules/4.19.73/8189fs.ko");

	int count = 0;

    while (1)
    {
        if (checkWifiInterface())
        {
            break;
        }

		if (count > 5)
		{
			while (1)
			{
				setLedStatus(REC_LED1, 0);
				setLedStatus(REC_LED2, 1);
				sleep(1);
				setLedStatus(REC_LED1, 1);
				setLedStatus(REC_LED2, 0);
				sleep(1);
			}
		}

		count ++;
		sleep(1);
    }
	
	setLedStatus(REC_LED1, 1);
	setLedStatus(REC_LED2, 1);

	char wifiSSID[128] = { 0 };

	cJSON *para_root = parseCJSONFile("/mnt/card/P-DC-1_Factory/wifi.json");
	if (NULL != para_root)
	{
		cJSON* item = cJSON_GetObjectItem(para_root, "ssid");
		if (NULL != item)
		{
			strcpy(wifiSSID, item->valuestring);
		}
		else
		{
			strcpy(wifiSSID, "P-DC-1_31127001");
		}
	}
	else
	{
		strcpy(wifiSSID, "P-DC-1_31127001");
	}

	WifiApReboot(1, wifiSSID, "12345678");

	return NULL;
}

void WifiTest_Start(void)
{
	pthread_t pid;

	pthread_create(&pid, 0, WifiTestThread, NULL);
}

