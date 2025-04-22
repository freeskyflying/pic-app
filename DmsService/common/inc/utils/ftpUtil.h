#ifndef _FTPUTIL_H_
#define _FTPUTIL_H_

#include <curl/curl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*ftpComplete)(void* arg);
typedef void(*ftpFailed)(void* arg);

typedef struct
{

	char		mRemotePath[255];
	char		mLocalPath[128];
	char		mFileName[64];
	char		mUserName[64];
	char		mPassWord[64];

	int			mDirection;			/* 0-download 1-upload */

	void*		mOwner;

	ftpComplete mComplete;
	ftpFailed	mFailed;

	FILE *stream;
	curl_off_t lastRuntime;
	CURL  *curl;

} FtpUtil_t;

/* pUtil必须使用静态内存变量，否则会出问题 */
int FtpUtil_DownloadCreate(FtpUtil_t* pUtil);

/* pUtil必须使用静态内存变量，否则会出问题 */
int FtpUtil_UploadCreate(FtpUtil_t* pUtil);

CURLcode handleUploadFiles(FtpUtil_t* pUtil, long connectTimeout, long responseTimeout);

#ifdef __cplusplus
}
#endif

#endif

