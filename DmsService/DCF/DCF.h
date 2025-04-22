/*
* DCF.h - Sigmastar
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

#ifndef _DCF_H_
#define _DCF_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MBYTES            (1024 * 1024)

// #define SD_ROOT           "/mnt/mmc"
// #define SD_ROOT           mnt_name;
#define DB_FILE_NAME      "%2s%1d-%4d%02d%02d-%02d%02d%02d-%d.%s"
#define FORMAT_FLAG       ".sstar.format"
// #define FORMAT_FLAG       ".sdCardState"
#define FORMAT_FF_FLAG    ".sstar.formatfree"
#define FORMATFREE_PREFIX ".FREE"
#define SUB_CAM_PREFIX    ".s_"

#define DB0_FOLDER        "Normal"
#define DB1_FOLDER        "Event"
#define DB2_FOLDER        "Photo"
#define DB3_FOLDER        "Parking"
// #define DB3_FOLDER        "Share"
// #define DB4_FOLDER        "Parking"

#define DB0_PREFIX        "CH" // sync DB_FILE_NAME %5s
#define DB1_PREFIX        "SOS" // sync DB_FILE_NAME %3s
#define DB2_PREFIX        "IMG" // sync DB_FILE_NAME %3s
#define DB3_PREFIX        "PAR" // sync DB_FILE_NAME %3s
// #define DB3_PREFIX        "SUB" // sync DB_FILE_NAME %3s
// #define DB4_PREFIX        "PAR" // sync DB_FILE_NAME %3s

#define DB0_FORMATFREE    (1)
#define DB1_FORMATFREE    (1)
#define DB2_FORMATFREE    (1)
#define DB3_FORMATFREE    (1)
#define DB4_FORMATFREE    (0)

#define DB0_FORMATFREE_FILESIZE    {30 * MBYTES, 60 * MBYTES, 30 * MBYTES, 30 * MBYTES, 30 * MBYTES, 30 * MBYTES, 30 * MBYTES, 30 * MBYTES, 30 * MBYTES}
#define DB1_FORMATFREE_FILESIZE    {100 * MBYTES, 30 * MBYTES}
#define DB2_FORMATFREE_FILESIZE    {  4 * MBYTES,  4 * MBYTES}
#define DB3_FORMATFREE_FILESIZE    {230 * MBYTES, 70 * MBYTES}
#define DB4_FORMATFREE_FILESIZE    {100 * MBYTES, 80 * MBYTES}

#define DB0_FORMATFREE_USAGE       (98) // %
#define DB1_FORMATFREE_USAGE       (10) // %
#define DB2_FORMATFREE_USAGE       (5)  // %
#define DB3_FORMATFREE_USAGE       (15) // %
#define DB4_FORMATFREE_USAGE       (10) // %

#define DB0_SUBFILE_ENABLE         {0, 0, 0, 0, 0, 0, 0, 0, 0} // TBD : If enable formatfree, disable it now.
#define DB1_SUBFILE_ENABLE         {0, 0}
#define DB2_SUBFILE_ENABLE         {0, 0}
#define DB3_SUBFILE_ENABLE         {0, 0}
#define DB4_SUBFILE_ENABLE         {0, 0, 0, 0}

#define CAM_NUM           (9)
#define CAM0_FOLDER       "CH1"
#define CAM1_FOLDER       "CH2"
#define CAM2_FOLDER       "CH3"
#define CAM3_FOLDER       "CH4"
#define CAM4_FOLDER       "CH5"
#define CAM5_FOLDER       "CH6"
#define CAM6_FOLDER       "CH7"
#define CAM7_FOLDER       "CH8"
#define CAM8_FOLDER       "CH9"

#define CAM_INITIAL_INFO  {0, 0} // sync CAM_NUM

#define DB_INFO(x)                                                  \
    {                                                               \
        DB##x##_FOLDER,                /* pDBFolderName */          \
        DB##x##_PREFIX,                /* pDBFilePrefixName */      \
        DB##x##_FORMATFREE,            /* bDBFormatFree */          \
        {                              /* pCamFolderName */         \
            CAM0_FOLDER,                                            \
            CAM1_FOLDER,                                            \
            CAM2_FOLDER,                                            \
            CAM3_FOLDER,                                            \
            CAM4_FOLDER,                                            \
            CAM5_FOLDER,                                            \
            CAM6_FOLDER,                                            \
            CAM7_FOLDER,                                            \
            CAM8_FOLDER,                                            \
        },                                                          \
        DB##x##_SUBFILE_ENABLE,        /* bCamSubFileEnable */      \
        CAM_INITIAL_INFO,              /* fileNameIdx */            \
        CAM_INITIAL_INFO,              /* fileCnt */                \
        CAM_INITIAL_INFO,              /* fileTotalSize */          \
        DB##x##_FORMATFREE_FILESIZE,   /* fileFormatFreeSize */     \
        DB##x##_FORMATFREE_USAGE,      /* fileFormatFreeUsage */    \
        CAM_INITIAL_INFO,              /* fileFormatFreeCnt */      \
    }                                                               \

#define DCF_INFO \
    DB_INFO(0),  \
    DB_INFO(1),  \
    DB_INFO(2),  \
    DB_INFO(3),  \

enum DB_TYPE {
    DB_NORMAL = 0,
    DB_EVENT,
    DB_PHOTO,
    // DB_SHARE,
    DB_PARKING,
    DB_NUM = 1,
};

int   DCF_Lock(void);
int   DCF_UnLock(void);
int   DCF_Format(long long totalSize);
int   DCF_CheckDBFormat(int DB, int camId);
int   DCF_Mount(int* chnStatus);
void  DCF_UnMount(void);
bool  DCF_IsSubFileEnable(int DB, int camId);
int   DCF_CreateFileName(int DB, int camId, char* pszFilePathName, time_t fileTime, const char* suffix_name);
int   DCF_CreateSubFileName(int DB, int camId, char* pszFilePathName, time_t fileTime);
void  DCF_AddFile(int DB, int camId, char* pszFilePathName);
void  DCF_UpdateFileSizeByName(int DB, int camId, char* pszFilePathName);
void  DCF_UpdateSubFileSizeByName(int DB, int camId, char* pszFilePathName);
char* DCF_GetFileNameFromTailByIdx(int DB, int camId, int idx);
char* DCF_GetFileNameFromHeadByIdx(int DB, int camId, int idx);
char* DCF_GetSubFileNameFromTailByIdx(int DB, int camId, int idx);
char* DCF_GetSubFileNameFromHeadByIdx(int DB, int camId, int idx);
char* DCF_GetPairFileName(char* pszFilePathName, int camId);
char* DCF_GetPairSubFileName(char* pszFilePathName, int camId);
void  DCF_MoveLastFile(int srcDB, int dstDB);
int   DCF_DeleteFileByFileName(char* pszFilePathName);
int   DCF_DeleteFileFromTailByIdx(int DB, int camId, int idx);
int   DCF_DeleteFileFromHeadByIdx(int DB, int camId, int idx);
int   DCF_ProtectFileFromTailByIdx(int DB, int camId, int idx);
int   DCF_ProtectFileFromHeadByIdx(int DB, int camId, int idx);
int   DCF_UnProtectFileFromTailByIdx(int DB, int camId, int idx);
int   DCF_UnProtectFileFromHeadByIdx(int DB, int camId, int idx);
bool  DCF_IsProtectFileFromTailByIdx(int DB, int camId, int idx);
bool  DCF_IsProtectFileFromHeadByIdx(int DB, int camId, int idx);
unsigned long long DCF_GetUsedSizeByDB(int DB);
unsigned long long DCF_GetUsedSize(void);
const char* DCF_GetDBFolderName(int DB);
int   DCF_GetDBFileCnt(int DB, int camId);
int   DCF_GetDBFileCntByDB(int DB);
bool  DCF_IsDBFormatFree(int DB);
bool  DCF_IsDBFormatFreeEmpty(int DB, int camId);
int   DCF_GetDBFormatFreeReservedCnt(int DB, int camId);
unsigned long long DCF_GetDBFormatFreeSize(int DB, int camId);
void  DCF_DumpInfo(void);
void  DCF_setMountName(char* mntName);
void  DCF_setCamNum(int _camNum);
void  DCF_setChnStatus(int _chnNum);
void  DCF_setFileSize(int s_size, int l_size);

#ifdef __cplusplus
}
#endif

#endif // _DCF_H_