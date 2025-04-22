#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <ctype.h>
#include <signal.h>

#include "updateHandler.h"
#include "utils.h"
#include "upgrade.h"
#include "ftp_update.h"

#define LOGE		printf

int updateHandler_MCUUpdate(char* filename)
{
	LOGE("device didn't have mcu.\n");

	return 0;
}

int updateHandler_SOCUpdate(char* filename)
{
	clearTargetDir("/cache");
    if (RET_SUCCESS == copyFile(UPGRADE_TARGET_FILE, filename))
    {
		playAudio("start_update.pcm");

		remove(filename);

        reboot2Upgrade();
    }

	remove(filename);

	return 0;
}

int updateHandler_ParamUpdate(char* filename)
{
	char cmd[1024] = { 0 };

	ensureTargetDirExist("/data/param");

	sprintf(cmd, "unzip -o %s -d /data/param", filename);
	system(cmd);

	
	DIR *dirp;
	struct dirent *dp;
	dirp = opendir("/data/param/map"); //打开目录指针
	if (dirp == NULL) {
		LOGE("not open ");
	}
	else
	{
		while ((dp = readdir(dirp)) != NULL) { //通过目录指针读目录
			if (NULL != strstr(dp->d_name, ".sh")) {
				memset(cmd,0, sizeof(cmd));
				sprintf(cmd,"chmod +x /data/param/map/%s",dp->d_name);
				system(cmd);

				memset(cmd,0, sizeof(cmd));
				sprintf(cmd,"/data/param/map/%s",dp->d_name);
				LOGE("electricmap find %s",cmd);
				system(cmd);
			}
		}
	}

	remove(filename);
	system("rm -rf /data/param/map");

	return 0;
}


