#include "common_test.h"
#include <time.h>
#include <linux/rtc.h>
#include <sys/time.h>

void* RTCTestThread(void* pArg)
{
	pthread_detach(pthread_self());

	int fd, retval;
    struct rtc_time rtc_tm;
    time_t timep;
    struct tm *p;
 
    fd = open("/dev/rtc0", O_RDONLY);
    if (fd == -1)
	{
		gTestResults[ITEM_RTC].result = 0;
		strcpy(gTestResults[ITEM_RTC].strInfo, "RTC异常");
    }
 
    /* Read the RTC time/date */
    retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
    if (retval == -1)
	{
		gTestResults[ITEM_RTC].result = 0;
		strcpy(gTestResults[ITEM_RTC].strInfo, "RTC异常");
    }
 
    close(fd);
 
    fprintf(stderr, "RTC date/time: %d/%d/%d %02d:%02d:%02d\n",
            rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
            rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
    time(&timep);
    p = gmtime(&timep);
    fprintf(stderr, "OS date/time(UTC): %d/%d/%d %02d:%02d:%02d\n",
            p->tm_mday, p->tm_mon + 1, p->tm_year + 1900,
            p->tm_hour, p->tm_min, p->tm_sec);
    p = localtime(&timep);
    fprintf(stderr, "OS date/time(Local): %d/%d/%d %02d:%02d:%02d\n",
            p->tm_mday, p->tm_mon + 1, p->tm_year + 1900,
            p->tm_hour, p->tm_min, p->tm_sec);
	
	gTestResults[ITEM_RTC].result = 1;
	gTestResults[ITEM_RTC].strInfo[0] = 0;

	return NULL;
}

void RTCTest_Start(void)
{
	pthread_t pid;

	pthread_create(&pid, 0, RTCTestThread, NULL);
}

