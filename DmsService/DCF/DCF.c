/*
* DCF.c - Sigmastar
*
* Copyright (C) 2019 Sigmastar Technology Corp.
*
* Author: jeff.li <jeff.li@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

//==============================================================================
//
//                              INCLUDE FILES
//
//==============================================================================

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <dirent.h>
#include <fnmatch.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include "DCF.h"

//==============================================================================
//
//                              MACRO DEFINES
//
//==============================================================================

#ifndef MALLOC
#define MALLOC(s) malloc(s)
#endif

#ifndef FREEIF
#define FREEIF(m) \
    if (m != 0)   \
    {             \
        free(m);  \
        m = NULL; \
    }
#endif

#define YEAR_OFFSET (1900)
#define MON_OFFSET (1)
#define RESERVED_SPACE (0x100000) // 1MB
#define BLOCK_SIZE (512)

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

struct file_node
{
    char          fileName[272];    /* include file path */
    char          subFileName[272]; /* include file path */
    int           fileNameIdx;
    int           camId;
    unsigned int  time;
    unsigned long size;        /* total size, in bytes */
    unsigned long subFileSize; /* total size, in bytes */
    mode_t        mode;        /* protection */
    TAILQ_ENTRY(file_node) tailq_entry;
};

struct file_formatfree_node
{
    char fileName[272]; /* include file path */
    TAILQ_ENTRY(file_formatfree_node) tailq_entry;
};

struct sDB
{
    const char *       pDBFolderName;
    const char *       pDBFilePrefixName;
    const bool         bDBFormatFree;
    const char *       pCamFolderName[CAM_NUM];
    bool               bCamSubFileEnable[CAM_NUM];
    int                fileNameIdx[CAM_NUM];
    int                fileCnt[CAM_NUM];
    unsigned long long fileTotalSize[CAM_NUM];
    unsigned long long fileFormatFreeSize[CAM_NUM];
    int                fileFormatFreeUsage;
    int                fileFormatFreeCnt[CAM_NUM];
};

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

TAILQ_HEAD(tailq_head, file_node) g_QueueHead[DB_NUM][CAM_NUM];
TAILQ_HEAD(tailq_formatfree_head, file_formatfree_node) g_QueueFormatFreeHead[DB_NUM][CAM_NUM];

struct sDB g_sDB[DB_NUM] = {DCF_INFO};

static char mnt_name[256] = {0};
static int camNum = 9;
static int chnNum = 0;
static int chn[10] = {0};

//==============================================================================
//
//                              LOCAL FUNCTIONS
//
//==============================================================================

static unsigned int _DCF_TmToTime(struct tm *tm)
{
    // struct tm {
    //     int tm_sec;    /* Seconds (0-60) */
    //     int tm_min;    /* Minutes (0-59) */
    //     int tm_hour;   /* Hours (0-23) */
    //     int tm_mday;   /* Day of the month (1-31) */
    //     int tm_mon;    /* Month (0-11) */
    //     int tm_year;   /* Year - 1900 */
    //     int tm_wday;   /* Day of the week (0-6, Sunday = 0) */
    //     int tm_yday;   /* Day in the year (0-365, 1 Jan = 0) */
    //     int tm_isdst;  /* Daylight saving time */
    // };

    return ((tm->tm_sec & 0x3F) | (tm->tm_min & 0x3F) << 6 | (tm->tm_hour & 0x1F) << 12 | (tm->tm_mday & 0x1F) << 17
            | (tm->tm_mon & 0x0F) << 22 | (tm->tm_year & 0x3F) << 26);
}

static int _DCF_MatchFileName(char *fileName)
{
    // sync DB_FILE_NAME      "%2s%1d-%4d%02d%02d-%02d%02d%02d-%d.%s"
    // sync DB_SUB_FILE_NAME  "%3s%4d%02d%02d-%02d%02d%02d-%d-s.%s"

    int fileNameLen = strlen(fileName);
    if (fileNameLen < 22)
        return -1;

    if (fileName[12] == '-' && fileName[19] == '-')
    {
        for (int i = 20; i < fileNameLen; i++)
        {
            if (fileName[i] >= '0' && fileName[i] <= '9')
                continue;
            if (fileName[i] == '.' && i + 1 < fileNameLen)
                return 0;
        }
    }
    return -1;
}

static int _DCF_CreateDBFolder(int DB)
{
    int  ret      = 0;
    int  fd       = 0;
    char path[64] = {
        0,
    };

    sprintf(path, "%s/%s", mnt_name, g_sDB[DB].pDBFolderName);

    ret = mkdir(path, S_IRWXU);
    if (ret != 0)
    {
        LOGC("create DB fail");
        return -1;
    }

    if (g_sDB[DB].bDBFormatFree)
    {
        sprintf(path, "%s/%s/%s", mnt_name, g_sDB[DB].pDBFolderName, FORMAT_FF_FLAG);
    }
    else
    {
        sprintf(path, "%s/%s/%s", mnt_name, g_sDB[DB].pDBFolderName, FORMAT_FLAG);
    }

    fd = open(path, O_WRONLY | O_CREAT);
    if (fd < 0)
    {
        LOGC("error: open failed\n");
        return -1;
    }

    close(fd);

    return 0;
}

static int _DCF_CreateCamFolder(int DB, int camId)
{
    int  ret      = 0;
    char path[32] = {
        0,
    };

    sprintf(path, "%s/%s/%s", mnt_name, g_sDB[DB].pDBFolderName, g_sDB[DB].pCamFolderName[camId]);

    ret = mkdir(path, S_IRWXU);
    if (ret != 0)
    {
        LOGC("create Cam fail%d", ret);
        return -1;
    }

    if (g_sDB[DB].bCamSubFileEnable[camId])
    {
        sprintf(path, "%s/%s/" SUB_CAM_PREFIX "%s", mnt_name, g_sDB[DB].pDBFolderName, g_sDB[DB].pCamFolderName[camId]);

        ret = mkdir(path, S_IRWXU);
        if (ret != 0)
        {
            LOGC("create Sub Cam fail");
            return -1;
        }
    }

    return ret;
}

static int _DCF_CheckDBFormat(int DB, int camId)
{
    int  ret      = 0;
    char file[64] = {
        0,
    };

    if (g_sDB[DB].bDBFormatFree && g_sDB[DB].bCamSubFileEnable[camId])
    {
        LOGC("DCF Not Support! Check DCF.h\n");
        assert(-1);
    }

    if (g_sDB[DB].bDBFormatFree)
    {
        sprintf(file, "%s/%s/%s", mnt_name, g_sDB[DB].pDBFolderName, FORMAT_FF_FLAG);
    }
    else
    {
        sprintf(file, "%s/%s/%s", mnt_name, g_sDB[DB].pDBFolderName, FORMAT_FLAG);
    }

    ret = access(file, F_OK);
    if (ret < 0)
        return ret;

    if (g_sDB[DB].bCamSubFileEnable[camId])
    {
        sprintf(file, "%s/%s/" SUB_CAM_PREFIX "%s", mnt_name, g_sDB[DB].pDBFolderName, g_sDB[DB].pCamFolderName[camId]);
        ret = access(file, F_OK);
        if (ret < 0)
            return ret;
    }

    return ret;
}

static void _DCF_ResetParam(int DB, int camId)
{
    g_sDB[DB].fileNameIdx[camId]       = 0;
    g_sDB[DB].fileCnt[camId]           = 0;
    g_sDB[DB].fileTotalSize[camId]     = 0;
    g_sDB[DB].fileFormatFreeCnt[camId] = 0;
}

static int _DCF_ParseFolder(int DB, int camId)
{
    int            ret = 0;
    DIR *          dp  = NULL;
    struct dirent *entry;
    struct tm      tmbuf;
    struct stat    statbuf;
    int            index;
    int            chId;
    char           path[32] = {
        0,
    };
    char prefix[4] = {
        0,
    };
    char suffix[8] = {
        0,
    };
    struct file_node *           fileNode           = NULL;
    struct file_formatfree_node *fileFormatFreeNode = NULL;
    char                         szFilePathName[64];
    char *                       pFileName;

    _DCF_ResetParam(DB, camId);

    sprintf(path, "%s/%s/%s", mnt_name, g_sDB[DB].pDBFolderName, g_sDB[DB].pCamFolderName[camId]);

    if ((dp = opendir(path)) == NULL)
        return -1;
    LOGB("---camdir:%s", path);
    while ((entry = readdir(dp)) != NULL)
    {
        // LOGB("filename:%s", entry->d_name);
        if (g_sDB[DB].bDBFormatFree && strncmp(entry->d_name, FORMATFREE_PREFIX, strlen(FORMATFREE_PREFIX)) == 0)
        {
            fileFormatFreeNode = (struct file_formatfree_node *)MALLOC(sizeof(struct file_formatfree_node));
            if (!fileFormatFreeNode)
                continue;
            sprintf(fileFormatFreeNode->fileName, "%s/%s/%s/%s", mnt_name, g_sDB[DB].pDBFolderName,
                    g_sDB[DB].pCamFolderName[camId], entry->d_name);
            TAILQ_INSERT_TAIL(&g_QueueFormatFreeHead[DB][camId], fileFormatFreeNode, tailq_entry);
            g_sDB[DB].fileTotalSize[camId] += g_sDB[DB].fileFormatFreeSize[camId];
            g_sDB[DB].fileFormatFreeCnt[camId]++;
            continue;
        }

        if (_DCF_MatchFileName(entry->d_name) < 0)
            continue;

        sscanf(entry->d_name, DB_FILE_NAME, prefix, &chId, &tmbuf.tm_year, &tmbuf.tm_mon, &tmbuf.tm_mday, &tmbuf.tm_hour,
               &tmbuf.tm_min, &tmbuf.tm_sec, &index, suffix);

        if (strcmp(prefix, g_sDB[DB].pDBFilePrefixName) != 0)
            continue;

        fileNode = (struct file_node *)MALLOC(sizeof(struct file_node));
        if (!fileNode)
            continue;

        fileNode->fileNameIdx    = index;
        fileNode->camId          = camId;
        fileNode->time           = _DCF_TmToTime(&tmbuf);
        fileNode->subFileName[0] = '\0';
        fileNode->subFileSize    = 0;
        sprintf(fileNode->fileName, "%s/%s/%s/%s", mnt_name, g_sDB[DB].pDBFolderName, g_sDB[DB].pCamFolderName[camId],
                entry->d_name);

        ret = stat(fileNode->fileName, &statbuf);
        if (ret < 0)
        {
            printf("stat [%s] fail\n", entry->d_name);
            FREEIF(fileNode);
            goto L_STAT_ERR;
        }
        else
        {
            fileNode->size = BLOCK_SIZE * statbuf.st_blocks;
            fileNode->mode = statbuf.st_mode;
            // LOGB("File [%s] Mode [%x]\n", fileNode->fileName, fileNode->mode);
        }

        if (TAILQ_EMPTY(&g_QueueHead[DB][camId]))
        {
            TAILQ_INSERT_TAIL(&g_QueueHead[DB][camId], fileNode, tailq_entry);
            g_sDB[DB].fileCnt[camId]++;
            g_sDB[DB].fileTotalSize[camId] += fileNode->size;
            if (g_sDB[DB].fileNameIdx[camId] <= fileNode->fileNameIdx)
                g_sDB[DB].fileNameIdx[camId] = fileNode->fileNameIdx + 1;
        }
        else
        {
            bool              bInsert = false;
            struct file_node *item;
            // Search from tail
            TAILQ_FOREACH_REVERSE(item, &g_QueueHead[DB][camId], tailq_head, tailq_entry)
            {
                if (item->fileNameIdx <= fileNode->fileNameIdx)
                {
                    TAILQ_INSERT_AFTER(&g_QueueHead[DB][camId], item, fileNode, tailq_entry);
                    g_sDB[DB].fileCnt[camId]++;
                    g_sDB[DB].fileTotalSize[camId] += fileNode->size;
                    if (g_sDB[DB].fileNameIdx[camId] <= fileNode->fileNameIdx)
                        g_sDB[DB].fileNameIdx[camId] = fileNode->fileNameIdx + 1;
                    bInsert                          = true;
                    break;
                }
            }

            if (bInsert == false)
            {
                TAILQ_INSERT_HEAD(&g_QueueHead[DB][camId], fileNode, tailq_entry);
                g_sDB[DB].fileCnt[camId]++;
                g_sDB[DB].fileTotalSize[camId] += fileNode->size;
            }
        }
    }

    if (g_sDB[DB].bCamSubFileEnable[camId])
    {
        sprintf(path, "%s/%s/" SUB_CAM_PREFIX "%s", mnt_name, g_sDB[DB].pDBFolderName, g_sDB[DB].pCamFolderName[camId]);

        // Search from head
        TAILQ_FOREACH(fileNode, &g_QueueHead[DB][camId], tailq_entry)
        {
            char fileName[64];
            sscanf(fileNode->fileName, "%s/%s", mnt_name, fileName);
            strtok(fileName, "/");
            strtok(NULL, "/");
            pFileName = strtok(NULL, "\0");
            sprintf(szFilePathName, "%s/%s/" SUB_CAM_PREFIX "%s/%s", mnt_name, g_sDB[DB].pDBFolderName,
                    g_sDB[DB].pCamFolderName[camId], pFileName);
            if (access(szFilePathName, F_OK) == 0)
            {
                ret = stat(szFilePathName, &statbuf);
                if (ret < 0)
                {
                    printf("stat [%s] fail\n", entry->d_name);
                    goto L_STAT_ERR;
                }
                else
                {
                    sprintf(fileNode->subFileName, "%s", szFilePathName);
                    fileNode->subFileSize = BLOCK_SIZE * statbuf.st_blocks;
                    g_sDB[DB].fileTotalSize[camId] += fileNode->subFileSize;
                }
            }
        }
    }

    closedir(dp);
    LOGC("parse filecnt[camid]:%d, freecnt:%d, %d", g_sDB[DB].fileCnt[camId], g_sDB[DB].fileFormatFreeCnt[camId], camId);
    return 0;

L_STAT_ERR:
    while ((fileNode = TAILQ_FIRST(&g_QueueHead[DB][camId])))
    {
        TAILQ_REMOVE(&g_QueueHead[DB][camId], fileNode, tailq_entry);
        FREEIF(fileNode);
    }
    _DCF_ResetParam(DB, camId);
    closedir(dp);
    return ret;
}

static int _DCF_Mount(int DB, int camId)
{
    int ret = 0;
    TAILQ_INIT(&g_QueueHead[DB][camId]);
    if (g_sDB[DB].bDBFormatFree)
        TAILQ_INIT(&g_QueueFormatFreeHead[DB][camId]);

    ret = _DCF_CheckDBFormat(DB, camId);
    if (ret < 0)
        return ret;

    return _DCF_ParseFolder(DB, camId);
}

static void _DCF_UnMount(int DB, int camId)
{
    struct file_node *           fileNode;
    struct file_formatfree_node *fileFormatFreeNode;

    if (g_sDB[DB].bDBFormatFree)
    {
        while ((fileFormatFreeNode = TAILQ_FIRST(&g_QueueFormatFreeHead[DB][camId])))
        {
            TAILQ_REMOVE(&g_QueueFormatFreeHead[DB][camId], fileFormatFreeNode, tailq_entry);
            FREEIF(fileFormatFreeNode);
        }
    }

    while ((fileNode = TAILQ_FIRST(&g_QueueHead[DB][camId])))
    {
        TAILQ_REMOVE(&g_QueueHead[DB][camId], fileNode, tailq_entry);
        FREEIF(fileNode);
    }

    _DCF_ResetParam(DB, camId);
}

static int _DCF_ProtectFileByFileNode(struct file_node *fileNode, bool bProtect)
{
    int         ret = 0;
    struct stat statbuf;

    if ((fileNode->mode & S_IWUSR) == 0 && bProtect)
        return 0;
    if ((fileNode->mode & S_IWUSR) && !bProtect)
        return 0;

    ret = stat(fileNode->fileName, &statbuf);
    if (ret < 0)
        return ret;

    if (bProtect)
        statbuf.st_mode &= ~S_IWUSR;
    else
        statbuf.st_mode |= S_IWUSR;
    ret = chmod(fileNode->fileName, statbuf.st_mode);
    if (ret < 0)
        return ret;
    sync();
    fileNode->mode = statbuf.st_mode;
    return ret;
}

static int _DCF_DeleteFileByFileNode(struct file_node *fileNode, int DB, int camId)
{
    int ret = 0;

    if (g_sDB[DB].bDBFormatFree)
    {
        struct file_formatfree_node *fileFormatFreeNode = NULL;

        if ((fileNode->mode & S_IWUSR) == 0)
        {
            // _DCF_ProtectFileByFileNode(fileNode->mode, false);
            LOGC("Can't Delete Protect File [%s]\n", fileNode->fileName);
            return -1;
        }

        fileFormatFreeNode = (struct file_formatfree_node *)MALLOC(sizeof(struct file_formatfree_node));
        if (!fileFormatFreeNode)
            return -1;

        sprintf(fileFormatFreeNode->fileName, "%s/%s/%s/%s%d", mnt_name, g_sDB[DB].pDBFolderName,
                g_sDB[DB].pCamFolderName[camId], FORMATFREE_PREFIX, g_sDB[DB].fileFormatFreeCnt[camId]);

        ret = rename(fileNode->fileName, fileFormatFreeNode->fileName);
        // LOGC("mv [%s] to [%s] ret [%d]\n", fileNode->fileName, fileFormatFreeNode->fileName, ret);
        if (ret < 0)
        {
            FREEIF(fileFormatFreeNode);
            return ret;
        }

        TAILQ_INSERT_TAIL(&g_QueueFormatFreeHead[DB][camId], fileFormatFreeNode, tailq_entry);
        g_sDB[DB].fileFormatFreeCnt[camId]++;
    }
    else
    {
        if (access(fileNode->fileName, F_OK) == 0)
        {
            if ((fileNode->mode & S_IWUSR) == 0)
            {
                printf("Can't Delete Protect File [%s]\n", fileNode->fileName);
                return -1;
            }

            ret = unlink(fileNode->fileName);
            printf("rm [%s] ret [%d]\n", fileNode->fileName, ret);
            if (ret < 0)
            {
                return ret;
            }
        }

        if (g_sDB[DB].bCamSubFileEnable[camId] && fileNode->subFileName[0] != '\0')
        {
            if (access(fileNode->subFileName, F_OK) == 0)
            {
                ret = unlink(fileNode->subFileName);
                printf("rm sub [%s] ret [%d]\n", fileNode->subFileName, ret);
                if (ret < 0)
                {
                    return ret;
                }
                g_sDB[DB].fileTotalSize[camId] -= fileNode->subFileSize;
            }
        }
        g_sDB[DB].fileTotalSize[camId] -= fileNode->size;
    }

    TAILQ_REMOVE(&g_QueueHead[DB][camId], fileNode, tailq_entry);
    g_sDB[DB].fileCnt[camId]--;
    FREEIF(fileNode);
    return ret;
}

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

int DCF_Lock(void)
{
    return pthread_mutex_lock(&mtx);
}

int DCF_UnLock(void)
{
    return pthread_mutex_unlock(&mtx);
}

int DCF_CheckDBFormat(int DB, int camId) {
    _DCF_CheckDBFormat(DB, camId);
}

int DCF_Format(long long totalSize)
{
    int ret = 0;
    for (int DB = 0; DB < DB_NUM; DB++)
    {
        ret = _DCF_CreateDBFolder(DB);
        if (ret < 0) {
            LOGC("create db folder err");
            return ret;
        }
        for (int camId = 0; camId < camNum; camId++)
        {
            if ((chnNum != 0) && (chn[camId] == 0)) {
                continue;
            }
            ret = _DCF_CreateCamFolder(DB, camId);
            if (ret < 0) {
                LOGC("create cam folder err");
                return ret;
            }
        }

        if (g_sDB[DB].bDBFormatFree)
        {
            char path[32] = {
                0,
            };
            char filename[64] = {
                0,
            };
            int       fd            = 0;
            long long TargetDBSize  = g_sDB[DB].fileFormatFreeUsage * totalSize / 100;
            long long CurDBSize     = 0;
            long long AdvanceDBSize = 0;
            int       idx           = 0;

            for (int camId = 0; camId < camNum; camId++) {
                if ((chnNum != 0) && (chn[camId] == 0)) {
                    continue;
                }
                AdvanceDBSize += g_sDB[DB].fileFormatFreeSize[camId];
            }

            do
            {
                // LOGB("cursize:%lld, advancesize:%lld, tarsize:%lld, %d, total:%lld", CurDBSize, AdvanceDBSize, TargetDBSize,g_sDB[DB].fileFormatFreeUsage, totalSize);
                if (CurDBSize + AdvanceDBSize + RESERVED_SPACE > TargetDBSize)
                    break;

                for (int camId = 0; camId < camNum; camId++)
                {
                    if ((chnNum != 0) && (chn[camId] == 0)) {
                        continue;
                    }
                    sprintf(path, "%s/%s/%s", mnt_name, g_sDB[DB].pDBFolderName, g_sDB[DB].pCamFolderName[camId]);

                    sprintf(filename, "%s/%s%d", path, FORMATFREE_PREFIX, idx);
                    fd = open(filename, O_WRONLY | O_CREAT);
                    if (fd < 0)
                    {
                        LOGC("open formatfree file failed\n");
                        return -1;
                    }
                    fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, g_sDB[DB].fileFormatFreeSize[camId]);
                    close(fd);
                    g_sDB[DB].fileFormatFreeCnt[camId]++;
                    CurDBSize += g_sDB[DB].fileFormatFreeSize[camId];
                }
                idx++;
            } while (1);
        }
    }
    return ret;
}

int DCF_Mount(int* chnStatus)
{
    int ret = 0;
    // int chnStatus = 0;
    for (int DB = 0; DB < DB_NUM; DB++)
        for (int camId = 0; camId < camNum; camId++)
        {
            if ((chnNum != 0) && (chn[camId] == 0)) {
                continue;
            }
            ret = _DCF_Mount(DB, camId);
            if (ret == 0) {
                *chnStatus |= 1<<camId;
            }
            // if (ret < 0)
            //     goto L_DCF_ERR;
        }
    return ret;

L_DCF_ERR:
    for (int DB = 0; DB < DB_NUM; DB++)
        for (int camId = 0; camId < camNum; camId++) {
            if ((chnNum != 0) && (chn[camId] == 0)) {
                continue;
            }
            _DCF_UnMount(DB, camId);
        }
    return ret;
}

void DCF_UnMount(void)
{
    for (int DB = 0; DB < DB_NUM; DB++)
        for (int camId = 0; camId < camNum; camId++) {
            if ((chnNum != 0) && (chn[camId] == 0)) {
                continue;
            }
            _DCF_UnMount(DB, camId);
        }
}

bool DCF_IsSubFileEnable(int DB, int camId)
{
    return g_sDB[DB].bCamSubFileEnable[camId];
}

int DCF_CreateFileName(int DB, int camId, char *pszFilePathName, time_t fileTime, const char *suffix_name)
{
    struct tm                  p;
    struct file_formatfree_node *fileFormatFreeNode = NULL;
    int                          ret                = 0;

    // p = localtime(&fileTime);
    localtime_r(&fileTime, &p);
    sprintf(pszFilePathName, "%s/%s/%s/" DB_FILE_NAME, mnt_name, g_sDB[DB].pDBFolderName,
            g_sDB[DB].pCamFolderName[camId], g_sDB[DB].pDBFilePrefixName, camId+1, (p.tm_year + YEAR_OFFSET),
            (p.tm_mon + MON_OFFSET), p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec, g_sDB[DB].fileNameIdx[camId],
            suffix_name);

    // LOGB("---createfile name:%s, %d", pszFilePathName, g_sDB[DB].bDBFormatFree);
    if (g_sDB[DB].bDBFormatFree)
    {
        if (TAILQ_EMPTY(&g_QueueFormatFreeHead[DB][camId]))
        {
            LOGC("create file name error free null DB [%d] Cam [%d]\n", DB, camId);
            return -2;
        }

        fileFormatFreeNode = TAILQ_LAST(&g_QueueFormatFreeHead[DB][camId], tailq_formatfree_head);
        ret                = rename(fileFormatFreeNode->fileName, pszFilePathName);
        LOGC("rename [%s] to [%s] ret [%s]", fileFormatFreeNode->fileName, pszFilePathName, strerror(ret));
        if (ret < 0)
        {
            return ret;
        }

        TAILQ_REMOVE(&g_QueueFormatFreeHead[DB][camId], fileFormatFreeNode, tailq_entry);
        FREEIF(fileFormatFreeNode);
        g_sDB[DB].fileFormatFreeCnt[camId]--;
    }

    return ret;
}

int DCF_CreateSubFileName(int DB, int camId, char *pszFilePathName, time_t fileTime)
{
    struct file_node *fileNode;
    struct tm         tmbuf;
    unsigned int      time;
    char              fileName[64];
    char *            pFileName;
    int               ret = -1;

    localtime_r(&fileTime, &tmbuf);
    tmbuf.tm_gmtoff = tmbuf.tm_year + YEAR_OFFSET;
    tmbuf.tm_mon  = tmbuf.tm_mon + MON_OFFSET;
    time           = _DCF_TmToTime(&tmbuf);
    TAILQ_FOREACH_REVERSE(fileNode, &g_QueueHead[DB][camId], tailq_head, tailq_entry)
    {
        if (time == fileNode->time)
        {
            sscanf(fileNode->fileName, "%s/%s", mnt_name, fileName);
            strtok(fileName, "/");
            strtok(NULL, "/");
            pFileName = strtok(NULL, "\0");
            sprintf(fileNode->subFileName, "%s/%s/" SUB_CAM_PREFIX "%s/%s", mnt_name, g_sDB[DB].pDBFolderName,
                    g_sDB[DB].pCamFolderName[camId], pFileName);
            sprintf(pszFilePathName, "%s", fileNode->subFileName);
            ret = 0;
            break;
        }
    }

    if (g_sDB[DB].bDBFormatFree)
    {
        printf("Not support now DB [%d] Cam [%d]\n", DB, camId);
        ret = -2;
    }

    return ret;
}

void DCF_AddFile(int DB, int camId, char *pszFilePathName)
{
    struct file_node *fileNode = (struct file_node *)MALLOC(sizeof(struct file_node));
    struct tm         tmbuf;
    char              fileName[64];
    char *            pFileName;
    char              prefix[6];
    char              suffix[8];
    int               index;
    int               chId;

    fileNode->camId          = camId;
    fileNode->size           = 0;
    fileNode->mode           = 0;
    fileNode->subFileName[0] = '\0';
    fileNode->subFileSize    = 0;
    sprintf(fileNode->fileName, "%s", pszFilePathName);

    // sscanf(fileNode->fileName, "%s/%s", mnt_name, fileName);
    strcpy(fileName, fileNode->fileName);
    strtok(fileName, "/");
    strtok(NULL, "/");
    pFileName = strtok(NULL, "\0");

    sscanf(pFileName, DB_FILE_NAME, &chId, prefix, &tmbuf.tm_year, &tmbuf.tm_mon, &tmbuf.tm_mday, &tmbuf.tm_hour,
           &tmbuf.tm_min, &tmbuf.tm_sec, &index, suffix);

    fileNode->time        = _DCF_TmToTime(&tmbuf);
    fileNode->fileNameIdx = index;

    TAILQ_INSERT_TAIL(&g_QueueHead[DB][camId], fileNode, tailq_entry);

    g_sDB[DB].fileNameIdx[camId]++;
    g_sDB[DB].fileCnt[camId]++;
}

void DCF_UpdateFileSizeByName(int DB, int camId, char *pszFilePathName)
{
    struct file_node *fileNode;
    struct stat       statbuf;
    int               ret = 0;

    // Search from tail
    TAILQ_FOREACH_REVERSE(fileNode, &g_QueueHead[DB][camId], tailq_head, tailq_entry)
    {
        if (strcmp(pszFilePathName, fileNode->fileName) == 0)
        {
            ret = stat(fileNode->fileName, &statbuf);
            if (ret < 0)
            {
                printf("stat [%s] fail\n", fileNode->fileName);
                fileNode->size = 0;
                fileNode->mode = 0;
            }
            else
            {
                fileNode->size = BLOCK_SIZE * statbuf.st_blocks;
                fileNode->mode = statbuf.st_mode;
                // printf("Update File [%s] Mode [%x] Size [%lu] Blk Alloc [%lu] Cluster [%lu]\n", fileNode->fileName,
                //        fileNode->mode, statbuf.st_size, BLOCK_SIZE * statbuf.st_blocks, statbuf.st_blksize);
                if (!g_sDB[DB].bDBFormatFree)
                    g_sDB[DB].fileTotalSize[camId] += fileNode->size;
            }
            break;
        }
    }
}

void DCF_UpdateSubFileSizeByName(int DB, int camId, char *pszFilePathName)
{
    struct file_node *fileNode;
    struct stat       statbuf;
    int               ret = 0;

    // Search from tail
    TAILQ_FOREACH_REVERSE(fileNode, &g_QueueHead[DB][camId], tailq_head, tailq_entry)
    {
        if (strcmp(pszFilePathName, fileNode->subFileName) == 0)
        {
            ret = stat(fileNode->subFileName, &statbuf);
            if (ret < 0)
            {
                printf("stat [%s] fail\n", fileNode->subFileName);
            }
            else
            {
                fileNode->subFileSize = BLOCK_SIZE * statbuf.st_blocks;
                g_sDB[DB].fileTotalSize[camId] += fileNode->subFileSize;
            }
            break;
        }
    }
}

char *DCF_GetFileNameFromTailByIdx(int DB, int camId, int idx)
{
    struct file_node *fileNode;
    int               cur_idx = 0;

    // Search from tail
    TAILQ_FOREACH_REVERSE(fileNode, &g_QueueHead[DB][camId], tailq_head, tailq_entry)
    {
        if (cur_idx++ == idx)
        {
            return fileNode->fileName;
        }
    }

    return NULL;
}

char *DCF_GetFileNameFromHeadByIdx(int DB, int camId, int idx)
{
    struct file_node *fileNode;
    int               cur_idx = 0;

    // Search from head
    TAILQ_FOREACH(fileNode, &g_QueueHead[DB][camId], tailq_entry)
    {
        if (cur_idx++ == idx)
        {
            return fileNode->fileName;
        }
    }

    return NULL;
}

char *DCF_GetSubFileNameFromTailByIdx(int DB, int camId, int idx)
{
    struct file_node *fileNode;
    int               cur_idx = 0;

    // Search from tail
    TAILQ_FOREACH_REVERSE(fileNode, &g_QueueHead[DB][camId], tailq_head, tailq_entry)
    {
        if (cur_idx++ == idx)
        {
            if (fileNode->subFileSize == 0)
                return NULL;
            return fileNode->subFileName;
        }
    }

    return NULL;
}

char *DCF_GetSubFileNameFromHeadByIdx(int DB, int camId, int idx)
{
    struct file_node *fileNode;
    int               cur_idx = 0;

    // Search from head
    TAILQ_FOREACH(fileNode, &g_QueueHead[DB][camId], tailq_entry)
    {
        if (cur_idx++ == idx)
        {
            if (fileNode->subFileSize == 0)
                return NULL;
            return fileNode->subFileName;
        }
    }

    return NULL;
}

char *DCF_GetPairFileName(char *pszFilePathName, int camId)
{
    struct tm         tmbuf;
    unsigned int      time;
    char              fileName[64];
    char *            pDBFolderName;
    char *            pCamFolderName;
    char *            pFileName;
    char              prefix[4];
    char              suffix[8];
    int               index;
    struct file_node *fileNode;

    if (camId < 0 || camId >= CAM_NUM)
        return NULL;

    sscanf(pszFilePathName, "%s/%s", mnt_name, fileName);
    pDBFolderName  = strtok(fileName, "/");
    pCamFolderName = strtok(NULL, "/");
    pFileName      = strtok(NULL, "\0");

    sscanf(pFileName, DB_FILE_NAME, prefix, &tmbuf.tm_year, &tmbuf.tm_mon, &tmbuf.tm_mday, &tmbuf.tm_hour,
           &tmbuf.tm_min, &tmbuf.tm_sec, &index, suffix);

    for (int DB = 0; DB < DB_NUM; DB++)
    {
        if (strcmp(g_sDB[DB].pDBFolderName, pDBFolderName) != 0)
            continue;

        if (strcmp(g_sDB[DB].pCamFolderName[camId], pCamFolderName) == 0)
        {
            // Same Cam, return self file name.
            return pszFilePathName;
        }

        time = _DCF_TmToTime(&tmbuf);

        // Search from head
        TAILQ_FOREACH(fileNode, &g_QueueHead[DB][camId], tailq_entry)
        {
            if (fileNode->time == time)
            {
                return fileNode->fileName;
            }
        }

        // Match DB Folder Name
        break;
    }

    return NULL;
}

char *DCF_GetPairSubFileName(char *pszFilePathName, int camId)
{
    struct tm         tmbuf;
    unsigned int      time;
    char              fileName[64];
    char *            pDBFolderName;
    char *            pFileName;
    char              prefix[4];
    char              suffix[8];
    int               index;
    struct file_node *fileNode;

    if (camId < 0 || camId >= CAM_NUM)
        return NULL;

    sscanf(pszFilePathName, "%s/%s", mnt_name, fileName);
    pDBFolderName = strtok(fileName, "/");
    strtok(NULL, "/");
    pFileName = strtok(NULL, "\0");

    sscanf(pFileName, DB_FILE_NAME, prefix, &tmbuf.tm_year, &tmbuf.tm_mon, &tmbuf.tm_mday, &tmbuf.tm_hour,
           &tmbuf.tm_min, &tmbuf.tm_sec, &index, suffix);

    for (int DB = 0; DB < DB_NUM; DB++)
    {
        if (strcmp(g_sDB[DB].pDBFolderName, pDBFolderName) != 0)
            continue;

        time = _DCF_TmToTime(&tmbuf);

        // Search from head
        TAILQ_FOREACH(fileNode, &g_QueueHead[DB][camId], tailq_entry)
        {
            if (fileNode->time == time)
            {
                if (fileNode->subFileSize == 0)
                    return NULL;
                return fileNode->subFileName;
            }
        }

        // Match DB Folder Name
        break;
    }

    return NULL;
}

void DCF_MoveLastFile(int srcDB, int dstDB)
{
    struct file_node *fileNode;
    char              dstFileName[64];
    char              fileName[64];
    char *            pFileName;

    if (g_sDB[srcDB].bDBFormatFree || g_sDB[dstDB].bDBFormatFree)
    {
        printf("DCF move file NOT support format free now\n");
        return;
    }

    for (int camId = 0; camId < CAM_NUM; camId++)
    {
        fileNode = TAILQ_LAST(&g_QueueHead[srcDB][camId], tailq_head);
        if (!fileNode)
            continue;

        if (g_sDB[srcDB].bCamSubFileEnable[camId])
        {
            if (!g_sDB[dstDB].bCamSubFileEnable[camId])
            {
                printf("DCF move file ONLY support both enable sub file DB\n");
                return;
            }
        }

        sscanf(fileNode->fileName, "%s/%s", mnt_name, fileName);
        strtok(fileName, "/");
        strtok(NULL, "/");
        pFileName = strtok(NULL, "\0");
        sprintf(dstFileName, "%s/%s/%s/%s", mnt_name, g_sDB[dstDB].pDBFolderName, g_sDB[dstDB].pCamFolderName[camId],
                pFileName);
        printf("mv %s to %s\n", fileNode->fileName, dstFileName);
        rename(fileNode->fileName, dstFileName);
        memcpy(fileNode->fileName, dstFileName, sizeof(dstFileName));

        TAILQ_REMOVE(&g_QueueHead[srcDB][camId], fileNode, tailq_entry);
        TAILQ_INSERT_TAIL(&g_QueueHead[dstDB][camId], fileNode, tailq_entry);

        g_sDB[srcDB].fileCnt[camId]--;
        g_sDB[srcDB].fileTotalSize[camId] -= fileNode->size;

        g_sDB[dstDB].fileCnt[camId]++;
        g_sDB[dstDB].fileTotalSize[camId] += fileNode->size;
        g_sDB[dstDB].fileNameIdx[camId]++;

        if (g_sDB[srcDB].bCamSubFileEnable[camId] && fileNode->subFileName[0])
        {
            sscanf(fileNode->subFileName, "%s/%s", mnt_name, fileName);
            strtok(fileName, "/");
            strtok(NULL, "/");
            pFileName = strtok(NULL, "\0");
            sprintf(dstFileName, "%s/%s/" SUB_CAM_PREFIX "%s/%s", mnt_name, g_sDB[dstDB].pDBFolderName,
                    g_sDB[dstDB].pCamFolderName[camId], pFileName);
            printf("mv sub %s to %s\n", fileNode->subFileName, dstFileName);
            rename(fileNode->subFileName, dstFileName);
            memcpy(fileNode->subFileName, dstFileName, sizeof(dstFileName));
            g_sDB[srcDB].fileTotalSize[camId] -= fileNode->subFileSize;
            g_sDB[dstDB].fileTotalSize[camId] += fileNode->subFileSize;
        }
    }
}

int DCF_DeleteFileByFileName(char *pszFilePathName)
{
    struct tm tmbuf;
    char      fileName[64];
    char *    pDBFolderName;
    char *    pCamFolderName;
    char *    pFileName;
    char      prefix[6];
    char      suffix[8];
    int       index;
    int       chId;

    sscanf(pszFilePathName, "%s/%s", mnt_name, fileName);
    pDBFolderName  = strtok(fileName, "/");
    pCamFolderName = strtok(NULL, "/");
    pFileName      = strtok(NULL, "\0");

    sscanf(pFileName, DB_FILE_NAME, prefix, &chId, &tmbuf.tm_year, &tmbuf.tm_mon, &tmbuf.tm_mday, &tmbuf.tm_hour,
           &tmbuf.tm_min, &tmbuf.tm_sec, &index, suffix);

    // printf("rm DB[%s]Cam[%s]File[%s]Index[%d]\n", pDBFolderName, pCamFolderName, pFileName, index);

    for (int DB = 0; DB < DB_NUM; DB++)
    {
        if (strcmp(g_sDB[DB].pDBFolderName, pDBFolderName) != 0)
            continue;

        for (int camId = 0; camId < camNum; camId++)
        {
            if ((chnNum != 0) && (chn[camId] == 0)) {
                continue;
            }
            struct file_node *fileNode;
            if (strcmp(g_sDB[DB].pCamFolderName[camId], pCamFolderName) != 0)
                continue;

            // Search from head
            TAILQ_FOREACH(fileNode, &g_QueueHead[DB][camId], tailq_entry)
            {
                if (fileNode->fileNameIdx == index)
                {
                    return _DCF_DeleteFileByFileNode(fileNode, DB, camId);
                }
            }
            // Match Cam Folder Name
            break;
        }
        // Match DB Folder Name
        break;
    }

    return -1;
}

int DCF_DeleteFileFromTailByIdx(int DB, int camId, int idx)
{
    struct file_node *fileNode;
    int               cur_idx = 0;

    // Search from tail
    TAILQ_FOREACH_REVERSE(fileNode, &g_QueueHead[DB][camId], tailq_head, tailq_entry)
    {
        if (cur_idx++ == idx)
        {
            return _DCF_DeleteFileByFileNode(fileNode, DB, camId);
        }
    }

    return -1;
}

int DCF_DeleteFileFromHeadByIdx(int DB, int camId, int idx)
{
    struct file_node *fileNode;
    int               cur_idx = 0;

    // Search from head
    TAILQ_FOREACH(fileNode, &g_QueueHead[DB][camId], tailq_entry)
    {
        if (cur_idx++ == idx)
        {
            return _DCF_DeleteFileByFileNode(fileNode, DB, camId);
        }
    }

    return -1;
}

int DCF_ProtectFileFromTailByIdx(int DB, int camId, int idx)
{
    struct file_node *fileNode;
    int               cur_idx = 0;

    // Search from tail
    TAILQ_FOREACH_REVERSE(fileNode, &g_QueueHead[DB][camId], tailq_head, tailq_entry)
    {
        if (cur_idx++ == idx)
        {
            return _DCF_ProtectFileByFileNode(fileNode, true);
        }
    }

    return -1;
}

int DCF_ProtectFileFromHeadByIdx(int DB, int camId, int idx)
{
    struct file_node *fileNode;
    int               cur_idx = 0;

    // Search from head
    TAILQ_FOREACH(fileNode, &g_QueueHead[DB][camId], tailq_entry)
    {
        if (cur_idx++ == idx)
        {
            return _DCF_ProtectFileByFileNode(fileNode, true);
        }
    }

    return -1;
}

int DCF_UnProtectFileFromTailByIdx(int DB, int camId, int idx)
{
    struct file_node *fileNode;
    int               cur_idx = 0;

    // Search from tail
    TAILQ_FOREACH_REVERSE(fileNode, &g_QueueHead[DB][camId], tailq_head, tailq_entry)
    {
        if (cur_idx++ == idx)
        {
            return _DCF_ProtectFileByFileNode(fileNode, false);
        }
    }

    return -1;
}

int DCF_UnProtectFileFromHeadByIdx(int DB, int camId, int idx)
{
    struct file_node *fileNode;
    int               cur_idx = 0;

    // Search from head
    TAILQ_FOREACH(fileNode, &g_QueueHead[DB][camId], tailq_entry)
    {
        if (cur_idx++ == idx)
        {
            return _DCF_ProtectFileByFileNode(fileNode, false);
        }
    }

    return -1;
}

bool DCF_IsProtectFileFromTailByIdx(int DB, int camId, int idx)
{
    struct file_node *fileNode;
    int               cur_idx = 0;

    // Search from tail
    TAILQ_FOREACH_REVERSE(fileNode, &g_QueueHead[DB][camId], tailq_head, tailq_entry)
    {
        if (cur_idx++ == idx)
        {
            if ((fileNode->mode & S_IWUSR) == 0)
                return true;
            else
                return false;
        }
    }

    return false;
}

bool DCF_IsProtectFileFromHeadByIdx(int DB, int camId, int idx)
{
    struct file_node *fileNode;
    int               cur_idx = 0;

    // Search from head
    TAILQ_FOREACH(fileNode, &g_QueueHead[DB][camId], tailq_entry)
    {
        if (cur_idx++ == idx)
        {
            if ((fileNode->mode & S_IWUSR) == 0)
                return true;
            else
                return false;
        }
    }

    return false;
}

unsigned long long DCF_GetUsedSizeByDB(int DB)
{
    unsigned long long usedSize = 0;
    for (int camId = 0; camId < camNum; camId++)
    {
        if ((chnNum != 0) && (chn[camId] == 0)) {
            continue;
        }
        usedSize += g_sDB[DB].fileTotalSize[camId];
    }
    return usedSize;
}

unsigned long long DCF_GetUsedSize(void)
{
    unsigned long long usedSize = 0;
    for (int DB = 0; DB < DB_NUM; DB++)
    {
        usedSize += DCF_GetUsedSizeByDB(DB);
    }
    return usedSize;
}

const char *DCF_GetDBFolderName(int DB)
{
    return g_sDB[DB].pDBFolderName;
}

int DCF_GetDBFileCnt(int DB, int camId)
{
    return g_sDB[DB].fileCnt[camId];
}

int DCF_GetDBFileCntByDB(int DB)
{
    int fileCnt = 0;
    for (int camId = 0; camId < camNum; camId++)
    {
        if ((chnNum != 0) && (chn[camId] == 0)) {
            continue;
        }
        fileCnt += g_sDB[DB].fileCnt[camId];
    }
    return fileCnt;
}

bool DCF_IsDBFormatFree(int DB)
{
    return g_sDB[DB].bDBFormatFree;
}

bool DCF_IsDBFormatFreeEmpty(int DB, int camId)
{
    if (!g_sDB[DB].bDBFormatFree)
    {
        printf("DCF error : check app flow\n");
        return false;
    }

    if (TAILQ_EMPTY(&g_QueueFormatFreeHead[DB][camId]))
        return true;
    else
        return false;
}

int DCF_GetDBFormatFreeReservedCnt(int DB, int camId)
{
    if (!g_sDB[DB].bDBFormatFree)
    {
        printf("DCF error : check app flow\n");
        return 0;
    }

    if (TAILQ_EMPTY(&g_QueueFormatFreeHead[DB][camId]))
        return 0;
    else
    {
        struct file_formatfree_node *fileFormatFreeNode;
        int                          cnt = 0;
        TAILQ_FOREACH(fileFormatFreeNode, &g_QueueFormatFreeHead[DB][camId], tailq_entry)
        {
            cnt++;
        }
        return cnt;
    }
}

unsigned long long DCF_GetDBFormatFreeSize(int DB, int camId)
{
    return g_sDB[DB].fileFormatFreeSize[camId];
}

void DCF_DumpInfo(void)
{
    struct statfs      diskInfo;
    unsigned long long blockSize       = 0;
    unsigned long long totalSizeKB     = 0;
    unsigned long long availableSizeKB = 0;
    struct file_node * item;
    int                ret = 0;

    ret = statfs(mnt_name, &diskInfo);
    if (ret != 0)
    {
        perror("statfs3 fail");
        return;
    }

    blockSize       = diskInfo.f_bsize;
    totalSizeKB     = blockSize * diskInfo.f_blocks >> 10;
    availableSizeKB = blockSize * diskInfo.f_bavail >> 10;

    printf("Total Size %lluMB\nAvailable Size %lluMB\n", totalSizeKB >> 10, availableSizeKB >> 10);

    for (int DB = 0; DB < DB_NUM; DB++)
    {
        printf("========= DB[%s] =========\n", g_sDB[DB].pDBFolderName);
        for (int camId = 0; camId < camNum; camId++)
        {
            if ((chnNum != 0) && (chn[camId] == 0)) {
                continue;
            }
            printf("CAM[%d]: %s\n", camId, g_sDB[DB].bCamSubFileEnable[camId] ? "Enable Sub File." : "");
            printf("file cnt[%d]\n", g_sDB[DB].fileCnt[camId]);
            printf("file total size[%llu]\n", g_sDB[DB].fileTotalSize[camId]);
            TAILQ_FOREACH(item, &g_QueueHead[DB][camId], tailq_entry)
            {
                printf("\t%40s\t\t%lu\n", item->fileName, item->size);
                if (g_sDB[DB].bCamSubFileEnable[camId])
                {
                    if (item->subFileName[0])
                        printf("\t\t%40s\t%lu\n", item->subFileName, item->subFileSize);
                }
            }
        }
        printf("\n\n");
    }
}

void  DCF_setMountName(char* mntName) {
    LOGB("DCF set mount name:%s, %s", mnt_name, mntName);
    strcpy(mnt_name, mntName);
}

void  DCF_setCamNum(int _camNum) {
    LOGB("DCF set cam num:%d", _camNum);
    camNum = _camNum;
}

void  DCF_setChnStatus(int _chnNum) {
    LOGB("DCF set chn status:%d", _chnNum);
    chnNum = _chnNum;
    for (int n = 0; n < CAM_NUM; n++) {
        if (_chnNum & 1<<n) {
            chn[n] = 1;
        }
    }
}

void  DCF_setFileSize(int s_size, int l_size) {
    LOGB("DCF set file size :%d, %d", s_size, l_size);
//    if(s_size < 10 || s_size > 100 || l_size < 10 || l_size > 100) {
//        LOGC("DCF set formatfree size is out of place %d %d", s_size, l_size);
//        return ;
//    }
    for (int n = 0; n < camNum; n++) {
        g_sDB[0].fileFormatFreeSize[n] = s_size * MBYTES;
    }
    g_sDB[0].fileFormatFreeSize[1] = l_size * MBYTES;
}
