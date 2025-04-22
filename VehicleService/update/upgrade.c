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

#include "osal.h"

#include "utils.h"
#include "upgrade.h"
#include "pnt_log.h"

typedef struct 
{

	unsigned int magic;
	unsigned int cmd;
	unsigned int rsv1[6];
	unsigned int status;
	unsigned int rsv2[7];

} bcb_info_t;

extern BoardSysInfo_t gBoardInfo;

volatile uint8_t gUpdateTFcardScanFlag = 0;

#define UPDATE_DBG      PNT_LOGI
#define UPDATE_INFO     PNT_LOGI
#define UPDATE_ERR      PNT_LOGE

void reboot2Upgrade(void)
{
	int fd = 0;
	int ret = 0;
	int i;
	bcb_info_t bcb_info = {0};

	bcb_info.magic = 0x20545055;
	bcb_info.cmd = 0x1;
	bcb_info.status = 0x0;

	UPDATE_DBG("open /dev/block/misc!\n");
	fd = open ("/dev/block/misc", O_RDWR);
	if (fd < 0) {
		UPDATE_ERR( "error:open block/misc failed!\n");
		return;
	}
	
	lseek(fd, 0, SEEK_SET);
	
	write(fd, &bcb_info, sizeof(bcb_info_t));

	close(fd);
	
	int recPid = getProcessPID("RecorderService", "RecorderService");
	if (recPid > 0)
	{
		UPDATE_ERR( "Stop Recorde!\n");
		char cmd[128] = { 0 };
		sprintf(cmd, "kill -2 %d", recPid);
		system(cmd);
	}
	else
	{
		UPDATE_ERR( "donot find RecorderService!\n");
	}
	
	sleep(4);
	
	system("reboot -f");
}

#define SETTINGTOOL_PATH	"/mnt/card/map"
#define SETTINGTOOL_SCRIPT	"/mnt/card/map/map.sh"

int checkSettings(void)
{
	if (access(SETTINGTOOL_PATH, R_OK))
	{
		return RET_FAILED;
	}
	
	if (access(SETTINGTOOL_SCRIPT, R_OK))
	{
		return RET_FAILED;
	}

	chmod(SETTINGTOOL_SCRIPT, S_IRUSR | S_IXUSR);

	system(SETTINGTOOL_SCRIPT);
	
	return RET_SUCCESS;
}

int parseFileName(char* fileName, UpgradeFileInfo_t* info)
{
    strcpy(info->fileName, fileName);

    int i=0, j=0, strLen = strlen(fileName);
    while (i<strLen)
    {
        if (fileName[i] == '_')
        {
            i++;
            info->productModel[j] = 0;
            break;
        }
        info->productModel[j++] = fileName[i];
        i++;
    }

    j=0;
    while (i<strLen)
    {
        if (fileName[i] == '_')
        {
            i++;
            info->fileVersion[j] = 0;
            break;
        }
        info->fileVersion[j++] = fileName[i];
        i++;
    }

    j=0;
    while (i<strLen)
    {
        if (fileName[i] == '_')
        {
            i++;
            info->targetVersion[j] = 0;
            break;
        }
        info->targetVersion[j++] = fileName[i];
        i++;
    }

    j=0;
    while (i<strLen)
    {
        if (j == 32)
        {
            info->md5[j] = 0;
            break;
        }
        info->md5[j++] = fileName[i];
        i++;
    }
    memcpy(info->suffix, &fileName[i], strLen-i);
    info->suffix[strLen-i] = 0;

    UPDATE_DBG("productModel: %s \nfileVersion: %s\ntargetVersion: %s\nmd5: %s\nsuffix: %s\n", info->productModel,
        info->fileVersion,
        info->targetVersion,
        info->md5,
        info->suffix);

    return RET_SUCCESS;
}

int getUpgradeType(UpgradeFileInfo_t* info)
{
    if (strcmp(info->productModel, gBoardInfo.mProductModel))
    {
        UPDATE_DBG("error file product modelname.\n");
        return 0;
    }

    if (strstr(info->suffix, ".zip") || strstr(info->fileVersion, ".zip"))
    {
        return 1;
    }
    else if (!strcmp(info->suffix, ".pkg"))
    {
	    if (strcmp(info->fileVersion, gBoardInfo.mSwVersion) <= 0)
	    {
	        UPDATE_DBG("current %s is new version, no need upgrade.\n", gBoardInfo.mSwVersion);
	        return 0;
	    }
		
        return 2;
    }

    return 0;
}

int checkUpdateFile(const char *args)
{
    DIR *dirp;
    struct dirent *dp;
    UpgradeFileInfo_t info = { 0 };
    int type = 0;

    dirp = opendir(args); //打开目录指针
    if (dirp == NULL)
    {
        UPDATE_ERR("checkUpdateFile --> cannot open %s\n", args);
        return -1;
    }

    while ((dp = readdir(dirp)) != NULL)
    {
        if (0 == strncmp(dp->d_name, (const char *)gBoardInfo.mProductModel, strlen(gBoardInfo.mProductModel)))
        {
            UPDATE_DBG("%s\n", dp->d_name);
            memset(&info, 0, sizeof(info));

            parseFileName(dp->d_name, &info);
            type = getUpgradeType(&info);
            if (type > 0)
            {
		        char originFile[256] = { 0 };
		        sprintf(originFile, "%s/%s", args, info.fileName);
				
			    if (type == 2)
			    {
			        char md5[33] = { 0 };

			        UPDATE_DBG("calc md5 %s.\n", originFile);
			        computeFileMd5(originFile, md5);

			        if (!strcmp(md5, info.md5))
			        {
			            UPDATE_DBG("check md5 success, start to copy file.\n");
			        	clearTargetDir("/cache");
			            if (RET_SUCCESS == copyFile(UPGRADE_TARGET_FILE, originFile))
			            {
					        playAudio("start_update.pcm");

							setUpgradeFileFlag(info.fileVersion);
							system("umount /cache");
			                reboot2Upgrade();
			            }
			        }
			    }
				else if (1 == type)
				{
					char cmd[1024] = { 0 };
					sprintf(cmd, "unzip -o %s -d "TFCARD_PATH, originFile);
					system(cmd);

					remove(originFile);
				}
            }
        }
    }
    (void) closedir(dirp);

	checkSettings();
    
    return 0;
}

int checkTFCardStatus(const char* sdPath)
{
    FILE* fp=NULL;
    char buf[1024];
    char command[150];

    sprintf(command, "mount | grep /mnt/card");

    if((fp = popen(command,"r")) != NULL)
    {
	    if( (fgets(buf, sizeof(buf), fp)) == NULL ) // mount failed
	    {
			pclose(fp);
	    	return RET_FAILED;
	    }
		else
    	{
    		if (strstr(buf, "(rw"))
    		{
				pclose(fp);
		    	return RET_SUCCESS;
    		}
			else
			{
				pclose(fp);
		    	return RET_FAILED;
			}
    	}
		pclose(fp);
    }
	
	return RET_FAILED;
}

void* upgradeDetect(void* arg)
{
    sleep(2);

    if (access("/cache", W_OK) == -1)
    {
        UPDATE_DBG("/cache is readonly.\n");
        system("mkfs.vfat /dev/block/cache");
        system("mount -t vfat -o \"defaults,noatime,errors=remount-ro\" /dev/block/cache /cache");
    }
    else
    {
        if (access(UPGRADE_TARGET_FILE, F_OK) != -1)
        {
            remove(UPGRADE_TARGET_FILE);
        }
    }


	int lastResult = getUpgradeResult(gBoardInfo.mSwVersion);
	if (lastResult >= 0)
	{
		UPDATE_ERR("last upgrade result : %d.", lastResult);
		if (0 == lastResult)
		{
			setUpgradeFileFlag("2");
        	playAudio("upgrade_failed.pcm");
		}
		else if (1 == lastResult)
		{
			setUpgradeFileFlag("3");
        	playAudio("upgrade_success.pcm");
		}
	}

	int cnt = 0, rebootflag = 1;

retry_tfcard:

	cnt = 0;

	setFlagStatus("tfcard", 2);
    while (RET_FAILED == checkTFCardStatus("/mnt/card"))
    {
        sleep(1);
    	if (cnt++ == 15 && rebootflag)
		{
	       	playAudio("no_tfcard.pcm");
    	}
    }

	gUpdateTFcardScanFlag = 1;
	setFlagStatus("tfcard", 1);

	cnt = 0;
    while (gUpdateTFcardScanFlag)
    {
    	if (cnt++ == 100)
		{ 
			remountTFCard(TFCARD_PATH, "/dev/mmcblk0p1");
     	}
        if (RET_SUCCESS == checkTFCardStatus(NULL))
        {
            break;
        }
        usleep(1000*100);
    }
	
	rebootflag = 0;
	
	setFlagStatus("tfcard", 0);
	
	sleep(2);
	checkUpdateFile(TFCARD_PATH);

    while (1)
    {
        if (RET_FAILED == checkTFCardStatus("/mnt/card"))
        {
			goto retry_tfcard;
		}
        sleep(1);
    }

    return NULL;
}

void SignalUpgrade(int signo)
{	
	PNT_LOGI("start to scan upgrade file.");
	
	StopWatchdog();
	
	checkUpdateFile(TFCARD_PATH);
	
	StartWatchdog();
}

void upgradeProcessStart(void)
{
    pthread_t pid;

	signal(SIGINT, SignalUpgrade);

    int ret = pthread_create(&pid, 0, upgradeDetect, NULL);
    if (ret)
    {
        UPDATE_ERR("errno=%d, reason(%s)\n", errno, strerror(errno));
    }

	ftpUpdateCreate();
}

