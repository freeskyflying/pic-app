#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <errno.h>
#include <sys/prctl.h>
#include <linux/falloc.h>
#include <sys/mman.h>

#include "pnt_log.h"
#include "utils.h"

char gLogOut[2048] = { 0 };
char gLogOutTemp[2048] = { 0 };

#define LOGFILE_PATH	"/mnt/card/Log"//"/data/Log"

int gLogFileFd = -1;
int gWriteSize = 0;
char* gLogFileBuffer = NULL;
int gBufferSize = 0;
pthread_mutex_t gLogFileMutex;

char gLogLevel = LOG_LEVEL;

void LogFileCreateNew(void)
{
    DIR *dirp;
    struct dirent *dp;
	char filename[64] = "33";
	int count = 0;
	
    dirp = opendir(LOGFILE_PATH);
	while ((dp = readdir(dirp)) != NULL)
	{
		if(strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0)    ///current dir OR parrent dir
        {
        	continue;
        }
		// 删除时间最早的文件，直接使用文件名判断，因为文件名即是时间
		if (strcmp(dp->d_name, filename) < 0)
		{
			strcpy(filename, dp->d_name);
		}
		count ++;
    }
	
    (void) closedir(dirp);

	char newFilename[128] = { 0 };
	char bcd[6] = {0};
    getLocalBCDTime(bcd);
	char timestr[64] = { 0 };
    sprintf(newFilename, "%s/20%02X%02X%02X-%02X%02X%02X.log", LOGFILE_PATH, bcd[0], bcd[1], bcd[2], bcd[3], bcd[4], bcd[5]);

	if (count >= 5)
	{
		printf("log file count full, rewrite %s\n", filename);
		char fullPath[128] = { 0 };
		sprintf(fullPath, "%s/%s", LOGFILE_PATH, filename);
		rename(fullPath, newFilename);
	}

	gLogFileFd = open(newFilename, O_WRONLY | O_CREAT);
	gWriteSize = 0;
}

void LogSaveInit(void)
{
	if (access(LOGFILE_PATH, F_OK))
	{
		mkdir(LOGFILE_PATH, 0666);
	}

	pthread_mutex_init(&gLogFileMutex, NULL);

	gLogFileBuffer = (char*)malloc(10*1024);
	if (NULL == gLogFileBuffer)
	{
		return;
	}
	gBufferSize = 0;
	LogFileCreateNew();
}

void LogSaveToFile(char* str, int flush)
{
	if (gLogFileFd >= 0)
	{
		pthread_mutex_lock(&gLogFileMutex);

		if (strlen(str) + gBufferSize >= 10*1024 || flush)
		{
			write(gLogFileFd, gLogFileBuffer, strlen(gLogFileBuffer));
			gWriteSize += strlen(gLogFileBuffer);

			if (gWriteSize > 2*1024*1024)
			{
				close(gLogFileFd);
				gLogFileFd = -1;
				if (!flush)
				{
					LogFileCreateNew();
				}
				else
				{
					sync();
					sleep(1);
				}
			}
			gBufferSize = 0;
		}

		if (strlen(str) + gBufferSize < 10*1024)
		{
			memcpy(gLogFileBuffer+gBufferSize, str, strlen(str));
			gBufferSize += strlen(str);	
		}
		
		pthread_mutex_unlock(&gLogFileMutex);
	}
}

void LogW(int log_level, const char* func, int line, const char* format, ...)
{
	va_list valist;
    char bcd[6] = {0};

	if (access("/tmp/log_level", F_OK) == 0)
	{
		int fd = open("/tmp/log_level", O_RDONLY);

		char value[2] = { 0 };
		read(fd, value, 1);

		gLogLevel = atoi(value);
		
		close(fd);
	}
	
	if (log_level > gLogLevel)
	{
		return;
	}

    getLocalBCDTime(bcd);
	char timestr[64] = { 0 };
    sprintf(timestr, "LIBPNT 20%02X-%02X-%02X %02X:%02X:%02X", bcd[0], bcd[1], bcd[2], bcd[3], bcd[4], bcd[5]);

#ifdef ANDROID_LOGCAT

	int pro = ANDROID_LOG_DEBUG;
	if (log_level == lev_error)
	{
		pro = ANDROID_LOG_ERROR;
	}
	else if (log_level == lev_info)
	{
		pro = ANDROID_LOG_INFO;
	}
#endif

	memset(gLogOut, 0, sizeof(gLogOut));
	va_start(valist, format);

	vsnprintf(gLogOut, sizeof(gLogOut) - 1, format, valist);
	va_end(valist);
	
#ifdef ANDROID_LOGCAT

    __android_log_print(pro, timestr, "%s-%d %s", func, line, gLogOut);

#else

    char bcd[6] = {0};

#endif

	if (gLogFileFd >= 0)
	{
		snprintf(gLogOutTemp, sizeof(gLogOutTemp) - 1, "20%02X-%02X-%02X %02X:%02X:%02X %s-%d %s\n", bcd[0], bcd[1], bcd[2], bcd[3], bcd[4], bcd[5], func, line, gLogOut);
		LogSaveToFile(gLogOutTemp, 0);
	}
}
