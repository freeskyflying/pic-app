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

#ifdef NO_MEDIA
#else
#include "lombo_system_sound.h"
#endif

#include "utils.h"
#include "pnt_log.h"
#include "queue_list.h"

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
        perror("open");
        return -1;
    }

    MD5Init(&md5);

    while (1)
    {
        ret = read(fd, data, READ_DATA_SIZE);
        if (-1 == ret)
        {
            perror("read");
            close(fd);
            return -1;
        }

        MD5Update(&md5, data, ret);

        if (0 == ret || ret < READ_DATA_SIZE)
        {
            break;
        }
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
                goto error;
            }
        } while (readCount > 0);
    }

    if (readCount == 0)
	{
        if (close(fdTarget) < 0)
		{
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
		if (0 == access("/data/etc/eng", F_OK))
		{
			sprintf(fullname, "%s/%s", VOICE_FILE_PATH_ENGLISH, filename);
		}
		else
		{
			sprintf(fullname, "%s/%s", VOICE_FILE_PATH_CHINESE, filename);
		}
		free(filename);
#ifdef NO_MEDIA
	    char cmd[1024] = { 0 };

	    if (!access(filename, R_OK))
	    {
	        sprintf(cmd, "aplay -r 8000 -c 1 -f S16_LE %s ", filename);
	        system(cmd);
	    }
#else
		PNT_LOGE("play audio %s.", fullname);
		//extern int lombo_system_sound_play(const char *file_name);
		//lombo_system_sound_play(fullname);
	    char cmd[1024] = { 0 };

	    if (!access(fullname, R_OK))
	    {
	        sprintf(cmd, "aplay -r 8000 -c 1 -f S16_LE %s ", fullname);
	        system(cmd);
	    }
#endif
	}

	return NULL;
}

int playAudio(char* file)
{
	if (!gAudioFileList.m_init)
	{
		pthread_t pid;
		
		queue_list_create(&gAudioFileList, 5);
		
		pthread_create(&pid, 0, playAudioThread, NULL);
	}

//    if (!access(file, R_OK))
    {
		char* filename = malloc(strlen(file) + 1);
		strcpy(filename, file);
		filename[strlen(file)] = 0;
		if (queue_list_push(&gAudioFileList, filename))
		{
			free(filename);
		}
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

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "fsck.fat -t %s", node);
	pSdmount = popen(buffer, "r");
	fgets(buffer, sizeof(buffer), pSdmount);
	printf(buffer);
	pclose(pSdmount);
	pSdmount = NULL;

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

int remountTfCard(const char* path, const char* node)
{
	FILE *pSdmount;
	char buffer[1024] = { 0 };

	if (access(path, F_OK) == 0)
	{
		mkdir(path, 0777);
	}

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "umount %s", path);
	pSdmount = popen(buffer, "r");
	fgets(buffer, sizeof(buffer), pSdmount);
	pclose(pSdmount);
	pSdmount = NULL;
	printf("umount result:");
	printf(buffer);
	printf(" *********\n");
	sleep(1);

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "fsck.fat -t %s", node);
	pSdmount = popen(buffer, "r");
	fgets(buffer, sizeof(buffer), pSdmount);
	pclose(pSdmount);
	pSdmount = NULL;
	printf("fsck.fat result:");
	printf(buffer);
	printf(" *********\n");

	if (strstr(buffer, "failed"))
	{
		return -1;
	}
	sleep(1);

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "mount -t vfat -o \"defaults,noatime,errors=continue\" %s %s", node, path);
	pSdmount = popen(buffer, "r");

	memset(buffer, 0, sizeof(buffer));
	fgets(buffer, sizeof(buffer), pSdmount);
	pclose(pSdmount);
	pSdmount = NULL;
	printf("mount result:");
	printf(buffer);
	printf(" *********\n");

	if (strstr(buffer, "failed"))
	{
		return -1;
	}

	return 0;
}

int formatAndMountTFCard(const char* path, const char* node)
{
	FILE *pSdFormat;
	char buffer[1024] = { 0 };
	
	sprintf(buffer, "mkfs.fat %s", node);
	pSdFormat = popen(buffer, "r");
	
	fgets(buffer, sizeof(buffer), pSdFormat);
	printf(buffer);
	
	pclose(pSdFormat);
	pSdFormat = NULL;
	
	if (strstr(buffer, "failed"))
	{
		return -2;
	}

	return mountTfCard(path, node);
}

void setFlagStatus(const char* name, char status)
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

char getFlagStatus(const char* name)
{
	char path[128] = { 0 };
	char status = 0;

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
	char path[128] = { 0 };

	if (NULL == sdPath)
	{
		sprintf(path, "/mnt/card/.check");
	}
	else
	{
		sprintf(path, "/%s/.check", sdPath);
	}

	int fd = open(path, O_WRONLY | O_CREAT);
	if (0 > fd)
	{
		return RET_FAILED;
	}
	if (write(fd, &fd, 1) < 1)
	{
		close(fd);
		return RET_FAILED;
	}
	close(fd);

	return RET_SUCCESS;
}

int getProcessPID(char* filter, char* tag)
{
	char pid[10] = { 0 };
	char buffer[1024] = { 0 };

	FILE *p = NULL;
	memset(buffer, 0, sizeof(buffer));
	
	sprintf(buffer, "ps | grep \"%s\"", filter);

	p = popen(buffer, "r");

	if (NULL == p)
	{
		printf("command failed.\n");
		return -1;
	}
	
	fread(buffer, 1, sizeof(buffer), p);
	pclose(p);
	p = NULL;

	char* tmp = strstr(buffer, tag);
	if (NULL != tmp && *(tmp-2) != 'p' && *(tmp-3) != 'p')
	{
		int i = 0, j = 0;
		while (buffer[i])
		{
			if (buffer[i] == ' ')
			{
				if (j == 0)
				{
					i++;
					continue;
				}
				else
				{
					break;
				}
			}
			pid[j++] = buffer[i];
			i ++;
		}
	}
	else
	{
		return -1;
	}

	return atoi(pid);
}

int hex2num(char c)
{
    if (c>='0' && c<='9') return c - '0';
    if (c>='a' && c<='z') return c - 'a' + 10;//杩欓噷+10鐨勫師鍥犳槸:姣斿16杩涘埗鐨刟鍊间负10
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

	int fd = open(filename, O_RDWR);

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


