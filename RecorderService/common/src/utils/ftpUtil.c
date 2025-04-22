#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "pnt_log.h"
#include "ftpUtil.h"
#include "utils.h"

static int ftpWriteFile(void *buffer, size_t size, size_t nmemb, void *stream)
{
    if (stream == NULL)
	{
        return 0;
    }

    FtpUtil_t *out = (FtpUtil_t *) stream;
    if (!out->stream)
	{
        out->stream = fopen(out->mFileName, "wb");
        if (!out->stream)
		{
            PNT_LOGE("fatal state error!!! fail to open file of %s, caused by %s", out->mFileName, strerror(errno));
            return -1;
        }
    }
    return fwrite(buffer, size, nmemb, out->stream);
}

static size_t ftpReadFile(void *ptr, size_t size, size_t nmemb, void *stream)
{
    FtpUtil_t *out = (FtpUtil_t *) stream;
	
    if (ferror(out->stream))
	{
        return CURL_READFUNC_ABORT;
    }

    if (stream == NULL)
	{
        return 0;
    }

    size_t length = 0;
    length = fread(ptr, size, nmemb, out->stream) * size;
    return length;
}

static size_t ftpGetContentLen(void *ptr, size_t size, size_t nmemb, void *stream)
{
    int readContentLen = 0;
    long len = 0;
    readContentLen = sscanf((const char *) ptr, "Content-Length: %ld\n", &len);
    if (stream == NULL)
	{
        return 0;
    }

    if (readContentLen)
	{
        *((long *) stream) = len;
    }
    return size * nmemb;
}

 static size_t progressCallback(void *ptr, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
 {
    FtpUtil_t *pUtil = (FtpUtil_t*)ptr;
    if (pUtil == NULL)
	{
        return 0;
    }

    CURL *curl = pUtil->curl;
    curl_off_t currentTime = 0;
    // 获取远程url连接的相关信息
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &currentTime);
    if (currentTime - pUtil->lastRuntime > 3000000) {
        pUtil->lastRuntime = currentTime;
        //PNT_LOGI("TOTAL TIME: %ld", currentTime);
    }

    if (ultotal) {
        //PNT_LOGI("UP PROGRESS :%g / %g (%g %%)", ulnow, ultotal, ulnow * 100.0 / ultotal);
    }

    if (dltotal) {
        //PNT_LOGI("DOWNLOAD PROGRESS: %g / %g (%g %%)", dlnow, dltotal, dlnow * 100.0 / dltotal);
    }
	
    return 0;
}

CURLcode handleDownloadFiles(FtpUtil_t* pUtil, long connectTimeout, long responseTimeout)
{
    CURL *downCurl = NULL;
    CURLcode res = CURLE_GOT_NOTHING;
    char fullPath[1024] = {0};
    if (strlen(pUtil->mFileName) > 0)
    {
        sprintf(fullPath, "%s/%s", pUtil->mLocalPath, pUtil->mFileName);
    }
    else
    {
        sprintf(fullPath, "%s", pUtil->mLocalPath);
    }
    size_t fullPathLen = strlen(fullPath);
    fullPath[fullPathLen] = '\0';

    int localLen = getFileSize(fullPath);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    downCurl = curl_easy_init();
    if (!downCurl)
	{
        PNT_LOGE("FATAL ERROR!!! fail to init the curl module");
        return res;
    }

    PNT_LOGE("Download target file from %s, save file as %s", pUtil->mRemotePath, pUtil->mLocalPath);
    curl_easy_setopt(downCurl, CURLOPT_URL, pUtil->mRemotePath);


    if (pUtil->mUserName != NULL && pUtil->mPassWord != NULL)
	{
        char userAccountInfo[200] = {0};
        sprintf(userAccountInfo, "%s:%s", pUtil->mUserName, pUtil->mPassWord);
        size_t userAccountInfoLen = strlen(userAccountInfo);
        userAccountInfo[userAccountInfoLen] = '\0';
        curl_easy_setopt(downCurl, CURLOPT_USERPWD, userAccountInfo);
    }

    if (connectTimeout > 0)
	{
        curl_easy_setopt(downCurl, CURLOPT_CONNECTTIMEOUT, connectTimeout);
    }

    // 设置响应的超时时间 单位 秒
    if (responseTimeout > 0)
	{
        curl_easy_setopt(downCurl, CURLOPT_FTP_RESPONSE_TIMEOUT, responseTimeout);
    }

    if (localLen > 0)
    {
        PNT_LOGE("Download target file from %s, save file as %s, start from %d", pUtil->mRemotePath, pUtil->mLocalPath, localLen);
        pUtil->stream = fopen(fullPath, "a+b");
        if (pUtil->stream)
        {
            fseek(pUtil->stream, 0, SEEK_END);
        }
    }
	else
	{
        pUtil->stream = fopen(fullPath, "wb");
	}

    curl_easy_setopt(downCurl, CURLOPT_WRITEFUNCTION, ftpWriteFile);
    curl_easy_setopt(downCurl, CURLOPT_WRITEDATA, pUtil);
    curl_easy_setopt(downCurl, CURLOPT_VERBOSE, 1L);

    // 设置下载文件进度，不设置会出现libcurl卡死，不执行curl_easy_perform方法
    curl_easy_setopt(downCurl, CURLOPT_XFERINFOFUNCTION, progressCallback);
	pUtil->curl = downCurl;
    curl_easy_setopt(downCurl, CURLOPT_XFERINFODATA, pUtil);
    curl_easy_setopt(downCurl, CURLOPT_NOPROGRESS, 0);

    if (localLen > 0)
    {
        curl_easy_setopt(downCurl, CURLOPT_FOLLOWLOCATION,1L);
        curl_easy_setopt(downCurl, CURLOPT_RESUME_FROM, localLen);
        curl_easy_setopt(downCurl, CURLOPT_PROGRESSDATA, &localLen);
    }

    res = curl_easy_perform(downCurl);

    PNT_LOGE("Handle down file completed,handle result is %d", res);
    curl_easy_cleanup(downCurl);
	
    if (pUtil->stream)
	{
        fclose(pUtil->stream);
    }
	
    return res;
}

CURLcode handleUploadFiles(FtpUtil_t* pUtil, long connectTimeout, long responseTimeout)
{
    CURL *uploadCurl = NULL;
    long uploadedLen = 0;
    CURLcode uploadResult = CURLE_GOT_NOTHING;
    int fileTotalsize = 0;
    FILE *localFile = fopen(pUtil->mLocalPath, "rb");
    if (localFile == NULL)
	{
        PNT_LOGE("FATAL ERROR !!! Handle upload file failed,error msg is %d,%s", errno, strerror(errno));
        return uploadResult;
    }

	pUtil->stream = localFile;

    fseek(localFile, 0L, SEEK_END);
    fileTotalsize = ftell(localFile);
    fseek(localFile, 0L, SEEK_SET);

    PNT_LOGE("Upload file total size  is %d", fileTotalsize);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    uploadCurl = curl_easy_init();
    if (uploadCurl == NULL)
	{
        PNT_LOGE("FATAL ERROR !!! uploadCurl is empty,so init curl failed!");
        curl_global_cleanup();
        fclose(localFile);
        return uploadResult;
    }
	
    char remotePathForm[500] = {0};
#if 0
    sprintf(remotePathForm, "%s/%s", pUtil->mRemotePath, fileName);
    size_t remotePathFormLen = strlen(remotePathForm);
    remotePathForm[remotePathFormLen] = '\0';
#else
	sprintf(remotePathForm, "%s", pUtil->mRemotePath);
#endif
    PNT_LOGE("remotePathForm is %s",remotePathForm);

    // 设置上传的url
    curl_easy_setopt(uploadCurl, CURLOPT_URL, remotePathForm);
    // 设置支持上传文件，非零值代表的是支持上传文件
    curl_easy_setopt(uploadCurl, CURLOPT_UPLOAD, 1L);

    if (pUtil->mUserName != NULL && pUtil->mPassWord != NULL)
	{
        char accountInfo[200] = {0};
        sprintf(accountInfo, "%s:%s", pUtil->mUserName, pUtil->mPassWord);
        size_t accountInfoLen = strlen(accountInfo);
        accountInfo[accountInfoLen] = '\0';
        curl_easy_setopt(uploadCurl, CURLOPT_USERPWD, accountInfo);
    }

    // 设置连接超时时间 单位 秒
    if (connectTimeout > 0)
	{
        curl_easy_setopt(uploadCurl, CURLOPT_CONNECTTIMEOUT, connectTimeout);
    }

    // 设置响应的超时时间 单位 秒
    if (responseTimeout > 0)
	{
        curl_easy_setopt(uploadCurl, CURLOPT_FTP_RESPONSE_TIMEOUT, responseTimeout);
    }

    curl_easy_setopt(uploadCurl, CURLOPT_HEADERFUNCTION, ftpGetContentLen);
    curl_easy_setopt(uploadCurl, CURLOPT_HEADERDATA, &uploadedLen);
    curl_easy_setopt(uploadCurl, CURLOPT_INFILESIZE, fileTotalsize);
    curl_easy_setopt(uploadCurl, CURLOPT_WRITEFUNCTION, ftpWriteFile);
    curl_easy_setopt(uploadCurl, CURLOPT_READFUNCTION, ftpReadFile);
    curl_easy_setopt(uploadCurl, CURLOPT_READDATA, pUtil);
    curl_easy_setopt(uploadCurl, CURLOPT_VERBOSE, 1L);
    // 设置自动创建目录的属性
    curl_easy_setopt(uploadCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);


    // 设置上传进度的属性
    curl_easy_setopt(uploadCurl, CURLOPT_XFERINFOFUNCTION, progressCallback);
    pUtil->curl = uploadCurl;
    curl_easy_setopt(uploadCurl, CURLOPT_XFERINFODATA, pUtil);
    curl_easy_setopt(uploadCurl, CURLOPT_NOPROGRESS, 0);

    curl_easy_setopt(uploadCurl, CURLOPT_APPEND, 0L);
    uploadResult = curl_easy_perform(uploadCurl);
    PNT_LOGE("Upload file completed, uploadResult is %d", uploadResult);
    curl_easy_cleanup(uploadCurl);
    curl_global_cleanup();
    fclose(localFile);

    return uploadResult;
}

void* ftpThread(void* arg)
{
    pthread_detach(pthread_self());

	FtpUtil_t* pUtil = (FtpUtil_t*)arg;

	PNT_LOGE("ftpThread %s %s start.", pUtil->mDirection==0?"download":"upload", pUtil->mRemotePath);

	CURLcode result = CURLE_OK;

	if (0 == pUtil->mDirection)
	{
		result = handleDownloadFiles(pUtil, 0, 0);

		if (result != CURLE_OK)
		{
	        PNT_LOGE("fatal exception!!! fail to download %s/%s, result are %d", pUtil->mLocalPath, pUtil->mFileName, result);
	    }
		else
		{
	        PNT_LOGE("download ota packet:%s/%s success!!!", pUtil->mLocalPath, pUtil->mFileName);
	    }
	}
	else if (1 == pUtil->mDirection)
	{
		result = handleUploadFiles(pUtil, 0, 0);

		if (result != CURLE_OK)
		{
	        PNT_LOGE("fatal exception!!! fail to upload %s/%s, result are %d", pUtil->mLocalPath, pUtil->mFileName, result);
	    }
		else
		{
	        PNT_LOGE("upload ota packet:%s/%s success!!!", pUtil->mLocalPath, pUtil->mFileName);
	    }
	}
	
	if (result != CURLE_OK)
	{
		if (pUtil->mFailed)
		{
			pUtil->result = result;
			pUtil->mFailed(pUtil);
		}
    }
	else
	{
		if (pUtil->mComplete)
		{
			pUtil->mComplete(pUtil);
		}
    }

	if (NULL != pUtil)
	{
		free(pUtil);
	}
	
    pthread_exit(NULL);
    return NULL;
}

int FtpUtil_DownloadCreate(FtpUtil_t* pUtil)
{
	if (NULL == pUtil)
	{
		return -1;
	}
	
	FtpUtil_t* pFtp = (FtpUtil_t*)malloc(sizeof(FtpUtil_t));

	memset(pFtp, 0, sizeof(FtpUtil_t));

	memcpy(pFtp, pUtil, sizeof(FtpUtil_t));

	pFtp->mDirection = 0;

	pthread_t pid;
	int threadRet = pthread_create(&pid, NULL, ftpThread, pFtp);
	if (threadRet != 0)
	{
		PNT_LOGE("FTP download %s create failed.", pFtp->mRemotePath);
		free(pFtp);
		return -1;
	}

	return 0;
}

int FtpUtil_UploadCreate(FtpUtil_t* pUtil)
{
	if (NULL == pUtil)
	{
		return -1;
	}
	
	FtpUtil_t* pFtp = (FtpUtil_t*)malloc(sizeof(FtpUtil_t));

	memset(pFtp, 0, sizeof(FtpUtil_t));

	memcpy(pFtp, pUtil, sizeof(FtpUtil_t));

	pFtp->mDirection = 1;

	pthread_t pid;
	int threadRet = pthread_create(&pid, NULL, ftpThread, pFtp);
	if (threadRet != 0)
	{
		PNT_LOGE("FTP upload %s create failed.", pFtp->mRemotePath);
		free(pFtp);
		return -1;
	}

	return 0;
}


