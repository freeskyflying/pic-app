#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <math.h>

#include "utils.h"
#include "pnt_log.h"
#include "queue_list.h"
#include "system_param.h"
#include "alsa_utils.h"

#define EARTH_RADIUS    6378137
#define PI 3.14159265358979323846

int gTimezoneOffset = (8*60*60);
int gTimeIsDst = 0;
int gCPUTemprature = 0;

#ifndef MIN
#  define MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#  define MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif

double radian(int d)
{
   return d * PI / (180.0*1000000);
}

float computeDistanceAndBearing(double lat1, double lon1, double lat2, double lon2)
{
    int MAXITERS = 20;
    // Convert lat/long to radians
    lat1 *= PI / 180.0;
    lat2 *= PI / 180.0;
    lon1 *= PI / 180.0;
    lon2 *= PI / 180.0;

    double a = 6378137.0; // WGS84 major axis
    double b = 6356752.3142; // WGS84 semi-major axis
    double f = (a - b) / a;
    double aSqMinusBSqOverBSq = (a * a - b * b) / (b * b);

    double L = lon2 - lon1;
    double A = 0.0;
    double U1 = atan((1.0 - f) * tan(lat1));
    double U2 = atan((1.0 - f) * tan(lat2));

    double cosU1 = cos(U1);
    double cosU2 = cos(U2);
    double sinU1 = sin(U1);
    double sinU2 = sin(U2);
    double cosU1cosU2 = cosU1 * cosU2;
    double sinU1sinU2 = sinU1 * sinU2;

    double sigma = 0.0;
    double deltaSigma = 0.0;
    double cosSqAlpha = 0.0;
    double cos2SM = 0.0;
    double cosSigma = 0.0;
    double sinSigma = 0.0;
    double cosLambda = 0.0;
    double sinLambda = 0.0;

    double lambda = L; // initial guess
    for (int iter = 0; iter < MAXITERS; iter++)
	{
        double lambdaOrig = lambda;
        cosLambda = cos(lambda);
        sinLambda = sin(lambda);
        double t1 = cosU2 * sinLambda;
        double t2 = cosU1 * sinU2 - sinU1 * cosU2 * cosLambda;
        double sinSqSigma = t1 * t1 + t2 * t2; // (14)
        sinSigma = sqrt(sinSqSigma);
        cosSigma = sinU1sinU2 + cosU1cosU2 * cosLambda; // (15)
        sigma = atan2(sinSigma, cosSigma); // (16)
        double sinAlpha = (sinSigma == 0) ? 0.0 :
                          cosU1cosU2 * sinLambda / sinSigma; // (17)
        cosSqAlpha = 1.0 - sinAlpha * sinAlpha;
        cos2SM = (cosSqAlpha == 0) ? 0.0 :
                 cosSigma - 2.0 * sinU1sinU2 / cosSqAlpha; // (18)

        double uSquared = cosSqAlpha * aSqMinusBSqOverBSq; // defn
        A = 1 + (uSquared / 16384.0) * // (3)
                (4096.0 + uSquared *
                          (-768 + uSquared * (320.0 - 175.0 * uSquared)));
        double B = (uSquared / 1024.0) * // (4)
                   (256.0 + uSquared *
                            (-128.0 + uSquared * (74.0 - 47.0 * uSquared)));
        double C = (f / 16.0) *
                   cosSqAlpha *
                   (4.0 + f * (4.0 - 3.0 * cosSqAlpha)); // (10)
        double cos2SMSq = cos2SM * cos2SM;
        deltaSigma = B * sinSigma * // (6)
                     (cos2SM + (B / 4.0) *
                               (cosSigma * (-1.0 + 2.0 * cos2SMSq) -
                                (B / 6.0) * cos2SM *
                                (-3.0 + 4.0 * sinSigma * sinSigma) *
                                (-3.0 + 4.0 * cos2SMSq)));

        lambda = L +
                 (1.0 - C) * f * sinAlpha *
                 (sigma + C * sinSigma *
                          (cos2SM + C * cosSigma *
                                    (-1.0 + 2.0 * cos2SM * cos2SM))); // (11)

        double delta = (lambda - lambdaOrig) / lambda;
        if (fabs(delta) < 1.0e-12)
		{
            break;
        }
    }

    return (float) (b * A * (sigma - deltaSigma));
}

int get_distance_by_lat_long(int longitude1, int latitude1, int longitude2, int latitude2)
{
	double radLat1 = radian(latitude1);
	double radLat2 = radian(latitude2);
	double a = radLat1 - radLat2;
	double b = radian(longitude1) - radian(longitude2);
		
	double dst = 2 * asin((sqrt(pow(sin(a / 2), 2) + cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2) )));

	dst = dst * EARTH_RADIUS;
	dst= round(dst * 10000) / 10000;

	int distance = (int)dst;
	return distance;
}

int get_distance_point_to_line(int longitude1, int latitude1, int longitude2, int latitude2, LatLng_t p)
{
	double a,b,c;
	
	a = get_distance_by_lat_long(longitude1,latitude1,longitude2,latitude2);//经纬坐标系中求两点的距离公式
	b = get_distance_by_lat_long(longitude2,latitude2,p.m_lng,p.m_lat);//经纬坐标系中求两点的距离公式
	c = get_distance_by_lat_long(longitude1,latitude1,p.m_lng,p.m_lat);//经纬坐标系中求两点的距离公式
	
	if(b*b>=c*c+a*a)return c;
	if(c*c>=b*b+a*a)return b;
	
	double l=(a+b+c)/2;     //周长的一半   
	double s=sqrt(l*(l-a)*(l-b)*(l-c));  //海伦公式求面积
	
	return 2*s/a;
}

int is_inside_polygon(LatLng_t *polygon,int N,LatLng_t p)
{
    int counter = 0;
    int i;
    double xinters;
    LatLng_t p1,p2;

    p1 = polygon[0];
    for (i=1;i<=N;i++) 
	{
        p2 = polygon[i % N];
        if (p.m_lng > MIN(p1.m_lng,p2.m_lng)) 
		{
            if (p.m_lng <= MAX(p1.m_lng,p2.m_lng)) 
			{
                if (p.m_lat <= MAX(p1.m_lat,p2.m_lat)) 
				{
                    if (p1.m_lng != p2.m_lng) 
					{
                        xinters = (double)((double)p.m_lng-(double)p1.m_lng)*((double)p2.m_lat-(double)p1.m_lat)/(double)((double)p2.m_lng-(double)p1.m_lng)+(double)p1.m_lat;
                        if (p1.m_lat == p2.m_lat || (double)p.m_lat <= xinters)
                        {
                        	counter++;
                        }
					}
				}
			}
		}
		p1 = p2;
	}

    if (counter % 2 == 0)
    {
    	return(0);
    }
    else
    {
    	return(1);
    }
}

int is_in_rectangle(int lt_lat, int lt_lng, int rb_lat, int rb_lng, LatLng_t p)
{
	int max_long = 0, min_long = 0, max_lat = 0, min_lat = 0;
	
	if (lt_lng > rb_lng)
	{
		max_long = lt_lng;
		min_long = rb_lng;
	}
	else
	{
		min_long = lt_lng;
		max_long = rb_lng;
	}

	if (rb_lat > lt_lat)
	{
		max_lat = rb_lat;
		min_lat = lt_lat;
	}
	else
	{
		min_lat = rb_lat;
		max_lat = lt_lat;
	}
	
	if (p.m_lng >= min_long && p.m_lng <= max_long && p.m_lat >= min_lat && p.m_lat <= max_lat)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

unsigned char bcdToDec(unsigned char data)
{
    unsigned char temp;
    temp = ((data >> 4) * 10 + (data & 0x0f));
    return temp;
}

static int byteToBcd(int originVal)
{
    return (originVal / 10 << 4) | originVal % 10;
}

int setNonBlocking(int fd)
{
    int opts;
    opts= fcntl(fd, F_GETFL);
    if(opts<0)
    {
        printf("fcntl(sock,GETFL) %d\n", fd);
        return RET_FAILED;
    }
    opts = opts | O_NONBLOCK;
    if(fcntl(fd, F_SETFL,opts)<0)
    {
        printf("fcntl(sock,SETFL,opts) %d\n", fd);
        return RET_FAILED;
    }
    return RET_SUCCESS;
}

long long currentTimeMillis()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    long long when = now.tv_sec * 1000LL + now.tv_usec / 1000;
    return when;
}

unsigned long getUptime()
{
    unsigned long uptime = 0;
    FILE *fp = fopen("/proc/uptime", "r");
    if(fp)
    {
        double runTime = 0;
        fscanf(fp, "%lf", &runTime);
        uptime = (unsigned long)runTime;
        fclose(fp);
    }
    return uptime;
}

long long str2num(const char *str)
{
    return atoll(str);
}

bool_t deleteFile(const char *szFileName)
{
    if (remove(szFileName) < 0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void getLocalTime(struct tm* t)
{
    if(t != NULL)
    {
        time_t tNow = time(NULL);
        localtime_r(&tNow, t);
    }
}

time_t convertBCDToSysTime(unsigned char* bcd)
{
    struct tm timeVal = {0};

    int year = bcdToDec(bcd[0]);
    int month = bcdToDec(bcd[1]);
    int day = bcdToDec(bcd[2]);
    int hour = bcdToDec(bcd[3]);
    int minute = bcdToDec(bcd[4]);
    int second = bcdToDec(bcd[5]);

    timeVal.tm_year = 2000 + year - 1900;
    timeVal.tm_mon = month - 1;
    timeVal.tm_mday = day;
    timeVal.tm_hour = hour;
    timeVal.tm_min = minute;
    timeVal.tm_sec = second;
	timeVal.tm_isdst = gTimeIsDst;

	return mktime(&timeVal);
}

void convertSysTimeToBCD(time_t timev, unsigned char* bcd)
{
	struct tm * timeVal = localtime(&timev);
    const int year = timeVal->tm_year + 1900;
    const int yearWant =
    year % 2000; // if the year are 2019, then what we truly need are 19(just the last two bit are need)
    const int month = timeVal->tm_mon + 1;
    const int day = timeVal->tm_mday;
    const int hour = timeVal->tm_hour;
    const int minute = timeVal->tm_min;
    const int second = timeVal->tm_sec;

    // 我们在上传报警信息时，报警信息当中的日期时间需要是明码
    bcd[0] = byteToBcd(yearWant);
    bcd[1] = byteToBcd(month);
    bcd[2] = byteToBcd(day);
    bcd[3] = byteToBcd(hour);
    bcd[4] = byteToBcd(minute);
    bcd[5] = byteToBcd(second);
}

int getLocalBCDTime(char* bcd_time)
{
	int year_now = 0;
	time_t t;
	struct tm * lt;
	time(&t);
	lt = localtime(&t);
	
	lt->tm_mon += 1;

	if ((lt->tm_year + 1900)<2000)
	{
		year_now = 0;
	}
	else
	{
		year_now = 1;
	}

	lt->tm_year = (lt->tm_year + 1900) % 100;
	bcd_time[0] = ((char)(lt->tm_year / 10) << 4) | (char)(lt->tm_year % 10);
	bcd_time[1] = ((char)(lt->tm_mon / 10) << 4) | (char)(lt->tm_mon % 10);
	bcd_time[2] = ((char)(lt->tm_mday / 10) << 4) | (char)(lt->tm_mday % 10);
	bcd_time[3] = ((char)(lt->tm_hour / 10) << 4) | (char)(lt->tm_hour % 10);
	bcd_time[4] = ((char)(lt->tm_min / 10) << 4) | (char)(lt->tm_min % 10);
	bcd_time[5] = ((char)(lt->tm_sec / 10) << 4) | (char)(lt->tm_sec % 10);

	return year_now;
}

int getTimeBCDOfDay(void)
{
	char bcdTime[6] = { 0 };
	getLocalBCDTime(bcdTime);

	bcdTime[0] = 0;
	bcdTime[1] = 0;
	bcdTime[2] = 0;

	return convertBCDToSysTime((unsigned char*)bcdTime);
}

/* 睡眠 */
void SleepMs(unsigned int ms)
{
	poll(0, 0, ms);
}

/* 睡眠 (us) */
void SleepUs(unsigned int us)
{
	usleep(us);
}

/* 睡眠 (se) */
void SleepSs(unsigned int se)
{
	sleep(se);
}

int ExecSql(sqlite3* db, const char* sql, SQLExeCallBack cb, void* param)
{
	int ret = -1;
	
	if(db != NULL)
	{
		char* zErrMsg = 0;
		
		if(sqlite3_exec(db, sql, cb, param, &zErrMsg) < 0)
		{
			PNT_LOGE("sqlite3_exec [%s] error", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		else
		{
			ret = 1;
		}
	}
	
	return ret;
}

int checkDBIntegrity(sqlite3* db)
{
	if (db == NULL)
	{
		return -1;
	}
	
	sqlite3_stmt *integrity = NULL;
	int isIntegrity = -1;
	
	if (sqlite3_prepare_v2(db, "PRAGMA quick_check;", -1, &integrity, NULL) == SQLITE_OK)
	{
		while (sqlite3_step(integrity) == SQLITE_ROW)
		{
			const unsigned char *result = sqlite3_column_text(integrity, 0);
			if (result && strncmp((const char *) result, (const char *)"ok", strlen("ok")) == 0)
			{
				isIntegrity = 0;
				break;
			}
		}
	}
	if (integrity != NULL)
	{
		sqlite3_finalize(integrity);
	}

	return isIntegrity;
}

int getFileSize(const char* path)
{
    int size = 0;

    FILE* pfile = fopen(path, "rb");
    if (NULL != pfile)
    {
        fseek(pfile, 0, SEEK_END);
        size = ftell(pfile);
        fclose(pfile);
    }

    return size;
}

bool_t ensureTargetDirExist(const char *path)
{
    DIR *directory = NULL;
    directory = opendir(path);
    if (directory == NULL)
	{
        PNT_LOGD("trying to make the base directory : %s", path);
        if (mkdir(path, 077) == -1)
		{
            PNT_LOGE("fail to create base directory %s, caused by %s", path, strerror(errno));
            return FALSE;
        }
    }
	else
	{
        if (closedir(directory) != 0)
		{
            PNT_LOGE("fail to close the %s, caused by %s", path, strerror(errno));
        }
    }
	if (RET_FAILED == checkTFCard(path))
	{
		return FALSE;
	}
    return TRUE;
}

int computeStrMd5(unsigned char *destStr, unsigned int destLen, char *targetMd5)
{

    int i = 0;
    unsigned char md5Value[MD5_SIZE];
    MD5Context md5;
    // init md5
    MD5Init(&md5);
    MD5Update(&md5, destStr, destLen);
    MD5Final(&md5, md5Value);
    // convert md5 value to md5 string
    for(i = 0; i < MD5_SIZE; i++)
    {
        snprintf(targetMd5 + i*2, 2+1, "%02x", md5Value[i]);
    }
    return 0;
}

int computeFileMd5(const char *filePath, char *targetMd5)
{
    int i;
    int fd;
    int ret;
    unsigned char data[READ_DATA_SIZE];
    unsigned char md5_value[MD5_SIZE];
    MD5Context md5;

    fd = open(filePath, O_RDONLY);
    if (-1 == fd)
    {
    	PNT_LOGE("open failed %s.", filePath);
        return -1;
    }

    MD5Init(&md5);

    while (1)
    {
        ret = read(fd, data, READ_DATA_SIZE);
        if (-1 == ret)
        {
    		PNT_LOGE("read failed %s.", filePath);
            close(fd);
            return -1;
        }

        MD5Update(&md5, data, ret);

        if (0 == ret || ret < READ_DATA_SIZE)
        {
            break;
        }
		usleep(300);
    }
    close(fd);
    MD5Final(&md5, md5_value);

    // convert md5 value to md5 string
    for(i = 0; i < MD5_SIZE; i++)
    {
        snprintf(targetMd5 + i*2, 2+1, "%02x", md5_value[i]);
    }

    return 0;
}

void clearTargetDir(const char *dir)
{
    DIR *directory = NULL;
    struct dirent *dirEntry;
    directory = opendir(dir);
    if (directory == NULL)
	{
        PNT_LOGE("FATAL ERROR!!! fail to open the %s, caused by %s", dir, strerror(errno));
        return;
    }

    while ((dirEntry = readdir(directory)) != NULL)
	{
        if (strcmp(dirEntry->d_name, "..") == 0)
		{
            continue;
        }
        if (strcmp(dirEntry->d_name, ".") == 0)
		{
            continue;
        }
        char targetDirName[512] = {0};
        sprintf(targetDirName, "%s/%s", dir, dirEntry->d_name);
        PNT_LOGE("remove file of %s", targetDirName);
        remove(targetDirName);
    }
    closedir(directory);
}

void clearTargetDirExt(const char *dir, const char* filename)
{
    DIR *directory = NULL;
    struct dirent *dirEntry;
    directory = opendir(dir);
    if (directory == NULL)
	{
        PNT_LOGE("FATAL ERROR!!! fail to open the %s, caused by %s", dir, strerror(errno));
        return;
    }

    while ((dirEntry = readdir(directory)) != NULL)
	{
        if (strcmp(dirEntry->d_name, "..") == 0)
		{
            continue;
        }
        if (strcmp(dirEntry->d_name, ".") == 0)
		{
            continue;
        }
		if (strcmp(filename, dirEntry->d_name))
		{
	        char targetDirName[512] = {0};
	        sprintf(targetDirName, "%s/%s", dir, dirEntry->d_name);
	        PNT_LOGE("remove file of %s", targetDirName);
	        remove(targetDirName);
		}
    }
    closedir(directory);
}

int copyFile(const char *targetFile, const char *originFile)
{
    int savedErrno;
    int fdTarget, fdOrigin;
    char buf[4096];
    ssize_t readCount;
    fdOrigin = open(originFile, O_RDONLY);
    if (fdOrigin < 0)
	{
        PNT_LOGE("CopyFile failed!!!fail to open %s, caused by %s", originFile, strerror(errno));
        return -1;
    }
    fdTarget = open(targetFile, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fdTarget < 0)
	{
        goto error;
    }
	PNT_LOGI("open %s fdTarget(%d)", targetFile, fdTarget);
    while (readCount = read(fdOrigin, buf, sizeof(buf)), readCount > 0)
	{
        char *outPtr = buf;
        ssize_t writeCount;
        // the write may do not success at once, and we need to try multiple times
        do
		{
            writeCount = write(fdTarget, outPtr, readCount);
            if (writeCount > 0)
			{
                readCount -= writeCount;
                outPtr += writeCount;
            }
			else if (errno != EINTR)
			{
				PNT_LOGE("write %s fdTarget(%d) failed", targetFile, fdTarget);
                goto error;
            }
        } while (readCount > 0);
    }

    if (readCount == 0)
	{
        if (close(fdTarget) < 0)
		{
			PNT_LOGE("close %s fdTarget(%d) failed", targetFile, fdTarget);
            fdTarget = -1;
            goto error;
        }
        close(fdOrigin);
        PNT_LOGE("copy %s --> %s success", originFile, targetFile);
        return 0;
    }

    error:
    savedErrno = errno;
    close(fdTarget);
    if (fdTarget >= 0)
	{
        close(fdTarget);
    }
    errno = savedErrno;
    return -1;
}

int strSplit(char* str, char ch, queue_t** result)
{
    int i = 0, j = 0, start = 0;
    queue_t* head = NULL, *newNode = NULL, *lastNode = NULL;
    char* tmp = NULL;

    while (1)
    {
        if (str[i] == '\0' || str[i] >= 0x80)
        {
            if (j > 0)
            {
                tmp = (char*)malloc(j+1);
                memset(tmp, 0, j+1);
                memcpy(tmp, str+start, j);
                
                if (NULL == head)
                {
                    head = (queue_t*)malloc(sizeof(queue_t));
                    memset(head, 0, sizeof(queue_t));
                    head->m_content = tmp;

                    lastNode = head;
                }
                else
                {
                    newNode = (queue_t*)malloc(sizeof(queue_t));
                    memset(newNode, 0, sizeof(queue_t));
                    newNode->m_content = tmp;
                    lastNode->next = newNode;
                    lastNode= newNode;
                }
            }
            break;
        }

        if (str[i] == ch)
        {
            if (j > 0)
            {
                tmp = (char*)malloc(j+1);
                memset(tmp, 0, j+1);
                memcpy(tmp, str+start, j);
                if (NULL == head)
                {
                    head = (queue_t*)malloc(sizeof(queue_t));
                    memset(head, 0, sizeof(queue_t));
                    head->m_content = tmp;

                    lastNode = head;
                }
                else
                {
                    newNode = (queue_t*)malloc(sizeof(queue_t));
                    memset(newNode, 0, sizeof(queue_t));
                    newNode->m_content = tmp;
                    lastNode->next = newNode;
                    lastNode= newNode;
                }

                start = i+1;
                j = 0;
            }
        }
        else
        {
            j ++;
        }

        i++;
    }

    *result = head;
    
    return RET_SUCCESS;
}

int getBoardSysInfo(BoardSysInfo_t* info)
{
	int fd = 0;
	int ret = 0;

    char buff[128] = { 0 };

	fd = open ("/dev/block/pkg_ver", O_RDONLY);
	if (fd < 0) {
		printf( "error:open block/pkg_ver failed!\n");
		return ret;
	}

	lseek(fd, 0, SEEK_SET);
    read(fd, buff, sizeof(buff));

    close(fd);

    queue_t* result = NULL, *tmp = NULL;
    strSplit(buff, '&', &result);

    memset(info, 0, sizeof(BoardSysInfo_t));

    if (NULL != result)
    {
        tmp = result->next;
        strncpy(info->mProductModel, (char*)result->m_content, sizeof(info->mProductModel)-1);
        free(result->m_content);
        free(result);
        result = tmp;
        if (NULL != result)
        {
            tmp = result->next;
            strncpy(info->mHwVersion, (char*)result->m_content, sizeof(info->mHwVersion)-1);
            free(result->m_content);
            free(result);
            result = tmp;
            if (NULL != result)
            {
                tmp = result->next;
                strncpy(info->mSwVersion, (char*)result->m_content, sizeof(info->mSwVersion)-1);
                free(result->m_content);
                free(result);
                result = tmp;
                for(int i=strlen(info->mSwVersion); i>0; i--)
                {
                    if (info->mSwVersion[i-1] < '0' || info->mSwVersion[i-1] > '9')
                    {
                        info->mSwVersion[i-1] = '\0';
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        while (result)
        {
            tmp = result;
            result = result->next;
            free(tmp->m_content);
            free(tmp);
        }
    }

	char chstr[16] = ".";
	
#if !(defined DISABLECH1)
	strcat(chstr, "1");
#endif
#if !(defined DISABLECH2)
	strcat(chstr, "2");
#endif
#if !(defined DISABLECH3)
	strcat(chstr, "3");
#endif

	strcat(info->mSwVersion, chstr);

	PNT_LOGI("model:%s.\n", info->mProductModel);
	PNT_LOGI("hwver:%s.\n", info->mHwVersion);
	PNT_LOGI("swver:%s.\n", info->mSwVersion);

    return RET_SUCCESS;
}

static queue_list_t gAudioFileList = { 0 };

void* playAudioThread(void* parg)
{
	char* fullname = (char*)malloc(128);

	while (1)
	{
		char* filename = queue_list_popup(&gAudioFileList);
		if (NULL == filename)
		{
			usleep(50*1000);
			continue;
		}

		memset(fullname, 0, 128);
		if (LANGUAGE_en_US == gGlobalSysParam->language)
		{
			sprintf(fullname, "%s/%s", VOICE_FILE_PATH_ENGLISH, filename);
		}
		else if (LANGUAGE_Spanish == gGlobalSysParam->language)
		{
			sprintf(fullname, "%s/%s", VOICE_FILE_PATH_SPANISH, filename);
		}
		else
		{
			sprintf(fullname, "%s/%s", VOICE_FILE_PATH_CHINESE, filename);
		}
		free(filename);
		
		PNT_LOGE("play audio %s.", fullname);
	    char cmd[1024] = { 0 };

	    if (!access(fullname, R_OK))
	    {
	        sprintf(cmd, "aplay -r 8000 -c 1 -f S16_LE %s ", fullname);
	        system(cmd);
	    }
	}

	return NULL;
}

int playAudio(char* file)
{
#if 0
	if (!gAudioFileList.m_init)
	{
		pthread_t pid;
		
		queue_list_create(&gAudioFileList, 5);
		
		pthread_create(&pid, 0, playAudioThread, NULL);
	}
	
	if (strlen(file)>0)
	{
		char* filename = malloc(strlen(file) + 1);
		strcpy(filename, file);
		filename[strlen(file)] = 0;
		if (queue_list_push(&gAudioFileList, filename))
		{
			free(filename);
		}
    }
#else
	char fullname[128] = { 0 };

	if (strlen(file)<=0)
	{
		return RET_SUCCESS;
	}
	
	memset(fullname, 0, 128);
	if (LANGUAGE_en_US == gGlobalSysParam->language)
	{
		sprintf(fullname, "%s/%s", VOICE_FILE_PATH_ENGLISH, file);
	}
	else if (LANGUAGE_Spanish == gGlobalSysParam->language)
	{
		sprintf(fullname, "%s/%s", VOICE_FILE_PATH_SPANISH, file);
	}
	else
	{
		sprintf(fullname, "%s/%s", VOICE_FILE_PATH_CHINESE, file);
	}
	
	PNT_LOGE("play audio %s.", fullname);
    char cmd[1024] = { 0 };

    if (!access(fullname, R_OK))
    {
		alsa_playaudio(fullname, 8000, 1, "S16_LE", 0);
    }
#endif
    return RET_SUCCESS;
}

int playAudioBlock(char* file)
{
	char fullname[128] = { 0 };
	
	memset(fullname, 0, 128);
	if (LANGUAGE_en_US == gGlobalSysParam->language)
	{
		sprintf(fullname, "%s/%s", VOICE_FILE_PATH_ENGLISH, file);
	}
	else if (LANGUAGE_Spanish == gGlobalSysParam->language)
	{
		sprintf(fullname, "%s/%s", VOICE_FILE_PATH_SPANISH, file);
	}
	else
	{
		sprintf(fullname, "%s/%s", VOICE_FILE_PATH_CHINESE, file);
	}
	
	PNT_LOGE("play audio %s.", fullname);
    char cmd[1024] = { 0 };

    if (!access(fullname, R_OK))
    {
#if 0
        sprintf(cmd, "aplay -r 8000 -c 1 -f S16_LE %s ", fullname);
        system(cmd);
#else
		alsa_playaudio(fullname, 8000, 1, "S16_LE", 1);
#endif
    }
	
    return RET_SUCCESS;
}

void setSpeakerVolume(int level)
{
	char cmd[128] = { 0 };
	
	int volume_level[4] = { 0, 63, 74, 84 };

	if (level > 3)
	{
		level = 3;
	}

	sprintf(cmd, "amixer cset numid=7 %d", volume_level[level]);
	system(cmd);
}

void storage_info_get(const char *mount_point, SDCardInfo_t *info)
{
    struct statfs disk_info;
    int blksize;
    FILE *fp;
    char split[] = " ";
    char linebuf[512];
    char *result = NULL;
    int mounted = 0;
    char mount_path[128];
    int len;
    len = strlen(mount_point);
    if (!len)
        return;
    strcpy(mount_path, mount_point);
    if (mount_path[len - 1] == '/')
        mount_path[len - 1] = '\0';
    fp = fopen("/proc/mounts", "r");
    if (fp == NULL)
    {
        PNT_LOGE("open error mount proc");
        info->total = 0;
        info->free = 0;
        return;
    }
    while (fgets(linebuf, 512, fp))
    {
        result = strtok(linebuf, split);
        result = strtok(NULL, split);
        if (result == NULL)
        {
            PNT_LOGE("result is NULL");
            break;
        }
        if (!strncmp(result, mount_path, strlen(mount_path)))
        {
            mounted = 1;
            break;
        }
    }
    fclose(fp);
    if (mounted ==  0)
    {
        info->total = 0;
        info->free = 0;
        return;
    }

    memset(&disk_info, 0, sizeof(struct statfs));
    if (statfs(mount_path, &disk_info) < 0)
        return;
    if (disk_info.f_bsize >= (1 << 10))
    {
        info->total = ((unsigned int)(disk_info.f_bsize >> 10) * disk_info.f_blocks);
        info->free = ((unsigned int)(disk_info.f_bsize >> 10) * disk_info.f_bfree);
    }
    else if (disk_info.f_bsize > 0)
    {
        blksize = ((1 << 10) / disk_info.f_bsize);
        info->total = (disk_info.f_blocks / blksize);
        info->free = (disk_info.f_bfree / blksize);
    }
}

void getWifiMac(char* wifiNode, char* mac)
{
	char path[128] = { 0 };

	sprintf(path, "/sys/class/net/%s/address", wifiNode);

	int fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		return;
	}

	read(fd, mac, 17);

	close(fd);
}

int mountTfCard(const char* path, const char* node)
{
	FILE *pSdmount;
	char buffer[1024] = { 0 };

	if (access(path, F_OK) == 0)
	{
		mkdir(path, 0777);
	}

	sprintf(buffer, "mount -t vfat -o \"defaults,noatime,errors=continue\" %s %s", node, path);
	pSdmount = popen(buffer, "r");
	fgets(buffer, sizeof(buffer), pSdmount);
	printf(buffer);
	pclose(pSdmount);
	pSdmount = NULL;
	if (strstr(buffer, "failed"))
	{
		return -1;
	}

	return 0;
}

int formatAndMountTFCard(const char* path, const char* node)
{
	FILE *pSdFormat;
	int count = 0;
	char buffer[1024] = { 0 };

	PNT_LOGE("format tf card");

	playAudio("start_format_tfcard.pcm");

wait_idle:
	memset(buffer, 0, sizeof(buffer));
	pSdFormat = popen("fuser -m /mnt/card", "r");
	fgets(buffer, sizeof(buffer), pSdFormat);
	pclose(pSdFormat);
	pSdFormat = NULL;
	PNT_LOGI("/mnt/card users: %s", buffer);
	if (strlen(buffer)>0)
	{
		count ++;
		if (count > 5)
		{
			PNT_LOGI("/mnt/card is busy.");
			return -1;
		}
		sleep(2);
		goto wait_idle;
	}

	count = 0;

unmount_tf:
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "umount -f %s", path);
	pSdFormat = popen(buffer, "r");
	memset(buffer, 0, sizeof(buffer));
	fgets(buffer, sizeof(buffer), pSdFormat);
	pclose(pSdFormat);
	pSdFormat = NULL;
	PNT_LOGI("%s", buffer);
	if (strstr(buffer, "busy"))
	{
		count ++;
		if (count > 5)
			return -1;
		sleep(2);
		goto unmount_tf;
	}
	PNT_LOGI("unmount success.");
	
	sprintf(buffer, "mkfs.fat %s", node);
	pSdFormat = popen(buffer, "r");
	memset(buffer, 0, sizeof(buffer));	
	fgets(buffer, sizeof(buffer), pSdFormat);
	PNT_LOGI("%s", buffer);
	pclose(pSdFormat);
	pSdFormat = NULL;
	
	if (strstr(buffer, "failed") || strstr(buffer, "Device or resource busy"))
	{
		return -2;
	}
	
	playAudio("format_success.pcm");

	return mountTfCard(path, node);
}

void remountTFCard(const char* path, const char* node)
{
	FILE *pSdFormat;
	char buffer[1024] = { 0 };

	PNT_LOGE("format tf card");

//	playAudio("start_format_tfcard.pcm");

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "umount -f %s", path);
	pSdFormat = popen(buffer, "r");
	memset(buffer, 0, sizeof(buffer));
	fgets(buffer, sizeof(buffer), pSdFormat);
	pclose(pSdFormat);
	pSdFormat = NULL;
	printf(buffer);
	sleep(1);

	sprintf(buffer, "fsck -y %s", node);
	pSdFormat = popen(buffer, "r");
	memset(buffer, 0, sizeof(buffer));	
	fgets(buffer, sizeof(buffer), pSdFormat);
	printf(buffer);
	pclose(pSdFormat);
	pSdFormat = NULL;
	
	mountTfCard(path, node);
}

void setFlagStatus(const char* name, int status)
{
	char path[128] = { 0 };

	sprintf(path, "/tmp/%s", name);
	int fd = open(path, O_WRONLY | O_CREAT);
	if (0 > fd)
	{
		return;
	}
	write(fd, &status, sizeof(status));
	close(fd);
}

int getFlagStatus(const char* name)
{
	char path[128] = { 0 };
	int status = 0;

	sprintf(path, "/tmp/%s", name);
	int fd = open(path, O_RDONLY);
	if (0 > fd)
	{
		return -1;
	}
	read(fd, &status, sizeof(status));
	close(fd);

	return status;
}

int checkTFCard(const char* sdPath)
{
#if 0
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
#else
	int ret = getFlagStatus("tfcard");
	if (0 == ret)
	{
		return RET_SUCCESS;
	}
	else
	{
		return RET_FAILED;
	}
#endif
}

int getProcessPID(char* filter, char* tag)
{
	char pid[10] = { 0 };
	char buffer[1024] = { 0 };

	FILE *p = NULL;
	memset(buffer, 0, sizeof(buffer));

	sprintf(buffer, "pgrep \"%s\"", filter);

	p = popen(buffer, "r");

	if (NULL == p)
	{
		printf("command failed.\n");
		return -1;
	}
	
	memset(buffer, 0, sizeof(buffer));
	fread(buffer, 1, sizeof(buffer), p);
	pclose(p);
	p = NULL;
	
	int i = 0, j = 0, flag = 0;
	while(buffer[i] != 0)
	{
		if (buffer[i] >= '0' && buffer[i] <= '9')
		{
			pid[j] = buffer[i];
			j++;
			flag = 1;
		}
		else if (flag)
		{
			break;
		}
		i++;
	}
	if (0 == strlen(pid))
	{
		return -1;
	}

	return atoi(pid);
}

int hex2num(char c)
{
    if (c>='0' && c<='9') return c - '0';
    if (c>='a' && c<='z') return c - 'a' + 10;//杩欓噷+10鐨勫師鍥犳槸:姣斿??16杩涘埗鐨刟鍊间负10
    if (c>='A' && c<='Z') return c - 'A' + 10;
    
    return '0';
}

int URLDecode(const char* str, const int strSize, char* result, const int resultSize)
{
    char ch,ch1,ch2;
    int i;
    int j = 0;//record result index


    if ((str==NULL) || (result==NULL) || (strSize<=0) || (resultSize<=0)) {
        return 0;
    }


    for ( i=0; (i<strSize) && (j<resultSize); ++i) {
        ch = str[i];
        switch (ch) {
            case '+':
                result[j++] = ' ';
                break;
            case '%':
                if (i+2<strSize) {
                    ch1 = hex2num(str[i+1]);//楂?浣?
                    ch2 = hex2num(str[i+2]);//浣?浣?
                    if ((ch1!='0') && (ch2!='0'))
                        result[j++] = (char)((ch1<<4) | ch2);
                    i += 2;
                    break;
                } else {
                    break;
                }
            default:
                result[j++] = ch;
                break;
        }
    }
    
    result[j] = 0;
    return j;
}

void getAppPlatformParams(AppPlatformParams_t* param, int group)
{
	memset(param, 0, sizeof(AppPlatformParams_t));

	char filename[128] = { 0 };
	if (0 == group) // 兼容旧版本
	{
		strcpy(filename, "/data/etc/appPlatformParams");
		if (access(filename, F_OK))
		{
			strcpy(param->ip, "192.168.2.107");
			param->port = 6608;
			strcpy(param->phoneNum, "746573743031");
			param->protocalType = PROTOCAL_808_2013;
			
			setAppPlatformParams(param, 0);
			return;
		}
	}
	else
	{
		sprintf(filename, "/data/etc/appPlatformParams_%d", group);
	}

	int fd = open(filename, O_RDONLY);
	if (0 > fd)
	{
		return;
	}

	read(fd, param, sizeof(AppPlatformParams_t));

	close(fd);
}

void setAppPlatformParamItems(int item, char* value, int group)
{
	AppPlatformParams_t param = { 0 };

	char filename[128] = { 0 };
	if (0 == group) // 兼容旧版本
	{
		strcpy(filename, "/data/etc/appPlatformParams");
	}
	else
	{
		sprintf(filename, "/data/etc/appPlatformParams_%d", group);
	}

	int fd = open(filename, O_RDWR | O_CREAT);

	if (0 > fd)
	{
		return;
	}

	read(fd, &param, sizeof(AppPlatformParams_t));

	lseek(fd, 0, SEEK_SET);

	if (0 == item)
	{
		strncpy(param.ip, value, sizeof(param.ip) - 1);
	}
	else if (1 == item)
	{
		param.port = atoi(value);
	}
	else if (2 == item)
	{
		char temp[128] = { 0 };
		URLDecode(value, strlen(value), temp, sizeof(temp));
		strncpy(param.plateNum, temp, sizeof(param.plateNum) - 1);
	}
	else if (3 == item)
	{
		strncpy(param.phoneNum, value, sizeof(param.phoneNum) - 1);
	}
	else if (4 == item)
	{
		param.plateColor = atoi(value);
	}
	else if (5 == item)
	{
		param.protocalType = atoi(value);
	}
	
	write(fd, &param, sizeof(AppPlatformParams_t));

	close(fd);

	sync();
}

void setAppPlatformParams(AppPlatformParams_t* param, int group)
{
	char filename[128] = { 0 };
	if (0 == group) // 兼容旧版本
	{
		strcpy(filename, "/data/etc/appPlatformParams");
	}
	else
	{
		sprintf(filename, "/data/etc/appPlatformParams_%d", group);
	}

	int fd = open(filename, O_RDWR | O_CREAT);

	if (0 > fd)
	{
		return;
	}
	
	write(fd, param, sizeof(AppPlatformParams_t));

	close(fd);
	
	sync();
}

void setUpgradeFileFlag(char* version)
{
	FILE* pFile = fopen(UPGRADE_FLAG_RESULT_FLAG, "wb");
	if (NULL == pFile)
	{
		return;
	}
	fwrite(version, 1, strlen(version), pFile);
	fclose(pFile);
	sync();
}

int getUpgradeResult(char* curVersion)
{
	char data[128] = { 0 };
	
	FILE* pFile = fopen(UPGRADE_FLAG_RESULT_FLAG, "rb");
	if (NULL == pFile)
	{
		return -1;
	}
	fread(data, 1, sizeof(data), pFile);
	fclose(pFile);

	if (strlen(data) > 2)
	{
		if (strcmp(data, curVersion))
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return atoi(data);
	}
}

void trigUpgrade(void)
{
	int recPid = getProcessPID("VehicleService", "VehicleService");
	if (recPid > 0)
	{
		PNT_LOGI( "trig upgrade!\n");
		char cmd[128] = { 0 };
		sprintf(cmd, "kill -2 %d", recPid);
		system(cmd);
	}
}

unsigned int getDirStorageSize(const char* path)
{
    struct dirent *dp;
	char filename[512] = { 0 };
	struct stat sta = { 0 };
	unsigned int dirSize = 0;
    DIR *dirp = opendir(path);

	while ((dp = readdir(dirp)) != NULL)
	{
		if(strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0)    ///current dir OR parrent dir
        {
        	continue;
        }

		sprintf(filename, "%s/%s", path, dp->d_name);
		
		memset(&sta, 0, sizeof(struct stat));
		stat(filename, &sta);

		dirSize += sta.st_size;
    }
	
    (void) closedir(dirp);

	return dirSize;
}

void set_tz(void)
{
	time_t ts = time(NULL);
	struct tm tUTC ,tLocal;
	gmtime_r(&ts, &tUTC);
	time_t tsUtc = mktime(&tUTC);

	gTimezoneOffset = ts - tsUtc;

	if (tUTC.tm_isdst>0)
	{
		gTimezoneOffset += 3600;
		gTimeIsDst = 1;
	}
	
	PNT_LOGI("local:%d utc:%d offset:%d %d\n", ts, tsUtc, gTimezoneOffset,tUTC.tm_isdst);
}

int getStringByFile(char* path, char* value, int len)
{
	FILE* pfile=fopen(path, "rb");
	
	if (NULL != pfile)
	{
		fgets(value, len, pfile);
		fclose(pfile);

		int i=strlen(value);
		while (i>0)
		{
			if (value[i-1] != '\r' && value[i-1] != '\n')
			{
				break;
			}
			else
			{
				value[i-1] = '\0';
			}
			i--;
		}
		
		return 0;
	}
	
	return -1;
}

int setStringToFile(char* path, char* value)
{
	FILE* pfile=fopen(path, "wb");
	
	if (NULL != pfile)
	{
		fwrite(value, 1, strlen(value), pfile);
		fclose(pfile);
		return 0;
	}
	
	return -1;
}

void loadSysLastTime(void)
{
	unsigned int timeLast = 0, timeLast2 = 0;
	
	FILE* pfile=fopen("/data/systm.save", "rb");
	
	if (NULL != pfile)
	{
		fread(&timeLast, 1, sizeof(timeLast), pfile);
		fclose(pfile);
	}

	pfile=fopen("/data/systm2.save", "rb");
	if (NULL != pfile)
	{
		fread(&timeLast2, 1, sizeof(timeLast2), pfile);
		fclose(pfile);
	}

	time_t ts = time(NULL);

	if (1660000000 > ts)
	{
		if (timeLast2 > 1660000000)
		{
			timeLast = timeLast2;
		}

		if (timeLast > 1660000000)
		{
			struct timeval tv;
			tv.tv_sec = timeLast;
			tv.tv_usec = 0;
			settimeofday(&tv, NULL);
			system("hwclock -w");

			set_tz();
		}
	}
}

void updateSysLastTime(void)
{
	static unsigned int lastUpdateTime = 0;
	static unsigned char updateSwitch = 0;

	time_t ts = time(NULL);
	
	if (1660000000 > ts)
	{
		return;
	}

	if (ts - lastUpdateTime >= 5)
	{
		set_tz();
	
		if (0 == updateSwitch)
		{
			FILE* pfile = pfile = fopen("/data/systm.save", "wb");
			
			if (NULL != pfile)
			{
				fwrite(&ts, 1, sizeof(ts), pfile);
				fclose(pfile);
				sync();
			}
			updateSwitch = 1;
		}
		else
		{
			FILE* pfile = pfile = fopen("/data/systm2.save", "wb");
			
			if (NULL != pfile)
			{
				fwrite(&ts, 1, sizeof(ts), pfile);
				fclose(pfile);
				sync();
			}
			updateSwitch = 0;
		}

		lastUpdateTime = ts;
	}
}

void get_product_name(char* product_name)
{
    FILE *fp;
	char* result = NULL;
    char split[] = "=";
    char linebuf[512];
	
    fp = fopen("/build.prop", "r");
    if (fp == NULL)
    {
        PNT_LOGE("open build.prop failed");
        return;
    }
    while (fgets(linebuf, 512, fp))
    {
    	if (strstr(linebuf, "ro.product.name"))
    	{
	        result = strtok(linebuf, split);
	        result = strtok(NULL, "\n");
	        if (result == NULL)
	        {
	            PNT_LOGE("ro.product.name is NULL");
	        }
			else
			{
				char* temp = strrchr(result, '_');
				strcpy(product_name, temp+1);
			}
	        break;
    	}
    }
    fclose(fp);
}

int getCpuTemprature(void)
{
	int temp = 0;
	char buffer[128] = { 0 };
	
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "rb");
	if (fp == NULL)
	{
		return temp;
	}
	fread(buffer, 1, sizeof(buffer), fp);
	fclose(fp);

	temp = atoi(buffer);
#if 0
	memset(buffer, 0, sizeof(buffer));
	
    FILE *fp = fopen("/sys/class/thermal/thermal_zone1/temp", "rb");
	if (fp == NULL)
	{
		return temp;
	}
	fread(buffer, 1, sizeof(buffer), fp);
	fclose(fp);

	temp += atoi(buffer);
#endif
	return temp/1000;
}

