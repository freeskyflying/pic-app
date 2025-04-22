#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <errno.h>
#include <sys/prctl.h>
#include "dirent.h"
#include <pthread.h>

#include "osal.h"
#include "sample_comm.h"

#include "typedef.h"
#include "pnt_log.h"
#include "queue_list.h"
#include "utils.h"
#include "media_utils.h"

#include "media_audio.h"

#define AUDIOENC_DBG        printf
#define AUDIOENC_ERR        printf

MediaAudioStream_t gMediaAudioStreamer = { 0 };

/******************************************************************************
* function : Start Ai
******************************************************************************/
static EI_S32 AUDIO_StartAi(AUDIO_DEV AiDevId, EI_S32 s32AiChnCnt, AIO_ATTR_S *pstAioAttr, AUDIO_SAMPLE_RATE_E enOutSampleRate, EI_BOOL bResampleEn, EI_VOID *pstAiVqeAttr)
{
    EI_S32 i;
    EI_S32 s32Ret;
    /* Left 5dB Right 0dB, but real value is 5 * 1.5 = 7.5dB.*/
    EI_S32 as32VolumeDb[2] = {5, 0};
    EI_S32 s32Length = (sizeof(as32VolumeDb) / sizeof(as32VolumeDb[0]));

    s32Ret = EI_MI_AUDIOIN_Init(AiDevId);
    if (EI_SUCCESS != s32Ret)
    {
        AUDIOENC_ERR("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return s32Ret;
    }

    s32Ret = EI_MI_AI_SetPubAttr(AiDevId, pstAioAttr);
    if (s32Ret)
    {
        AUDIOENC_ERR("%s: EI_MI_AI_SetPubAttr(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }

    s32Ret = EI_MI_AI_Enable(AiDevId);
    if (s32Ret)
    {
        AUDIOENC_ERR("%s: EI_MI_AI_Enable(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }

    s32Ret = EI_MI_AI_SetVolume(AiDevId, s32Length, as32VolumeDb);
    if (s32Ret)
    {
        AUDIOENC_ERR("%s: EI_MI_AI_SetVolume(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }

    for (i = 0; i < s32AiChnCnt >> pstAioAttr->enSoundmode; i++) {
        s32Ret = EI_MI_AI_EnableChn(AiDevId, i);
        if (s32Ret)
        {
            AUDIOENC_ERR("%s: EI_MI_AI_EnableChn(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
            return s32Ret;
        }

        if (EI_TRUE == bResampleEn)
        {
            s32Ret = EI_MI_AI_EnableReSmp(AiDevId, i, enOutSampleRate);
            if (s32Ret)
            {
                AUDIOENC_ERR("%s: EI_MI_AI_EnableReSmp(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
                return s32Ret;
            }
        }

        if (NULL != pstAiVqeAttr)
        {
            s32Ret = EI_MI_AI_SetVqeAttr(AiDevId, i, 0, 0, pstAiVqeAttr);
            if (s32Ret)
            {
                AUDIOENC_ERR("%s: SetAiVqe(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
                return s32Ret;
            }

            s32Ret = EI_MI_AI_EnableVqe(AiDevId, i);
            if (s32Ret)
            {
                AUDIOENC_ERR("%s: EI_MI_AI_EnableVqe(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
                return s32Ret;
            }
        }
    }

    return EI_SUCCESS;
}

/******************************************************************************
* function : Stop Ai
******************************************************************************/
static EI_S32 AUDIO_StopAi(AUDIO_DEV AiDevId, EI_S32 s32AiChnCnt,
    EI_BOOL bResampleEn, EI_BOOL bVqeEn)
{
    EI_S32 i;
    EI_S32 s32Ret;

    for (i = 0; i < s32AiChnCnt; i++)
    {
        if (EI_TRUE == bResampleEn)
        {
            s32Ret = EI_MI_AI_DisableReSmp(AiDevId, i);
            if (EI_SUCCESS != s32Ret)
            {
                AUDIOENC_ERR("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
                return s32Ret;
            }
        }

        if (EI_TRUE == bVqeEn)
        {
            s32Ret = EI_MI_AI_DisableVqe(AiDevId, i);
            if (EI_SUCCESS != s32Ret)
            {
                AUDIOENC_ERR("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
                return s32Ret;
            }
        }

        s32Ret = EI_MI_AI_DisableChn(AiDevId, i);
        if (EI_SUCCESS != s32Ret)
        {
            AUDIOENC_ERR("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
            return s32Ret;
        }
    }

    s32Ret = EI_MI_AI_Disable(AiDevId);
    if (EI_SUCCESS != s32Ret)
    {
        AUDIOENC_ERR("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return s32Ret;
    }

    s32Ret = EI_MI_AUDIOIN_Exit(AiDevId);
    if (EI_SUCCESS != s32Ret)
    {
        AUDIOENC_ERR("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return s32Ret;
    }
    AUDIOENC_DBG("EI_MI_AUDIOIN_Exit %d ok\n", __LINE__);

    return EI_SUCCESS;
}

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4
#define G726_BPS G726_40K//MEDIA_G726_40K
static EI_S32 AUDIO_StartAenc(EI_S32 s32AencChnCnt, AIO_ATTR_S *pstAioAttr, PAYLOAD_TYPE_E enType)
{
    AENC_CHN AeChn;
    EI_S32 s32Ret, i;
    AENC_CHN_ATTR_S stAencAttr;
    AENC_ATTR_ADPCM_S stAdpcmAenc;
    AENC_ATTR_G711_S stAencG711;
    AENC_ATTR_G726_S stAencG726;
    AENC_ATTR_LPCM_S stAencLpcm;
    AENC_ATTR_AAC_S  stAencAac;

    stAencAttr.enType = enType;
    stAencAttr.u32BufSize = 30;
    stAencAttr.u32PtNumPerFrm = pstAioAttr->u32PtNumPerFrm;
    stAencAttr.enSoundmode = pstAioAttr->enSoundmode;
    stAencAttr.enSamplerate = pstAioAttr->enSamplerate;

    if (PT_ADPCMA == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAdpcmAenc;
        stAdpcmAenc.enADPCMType = AUDIO_ADPCM_TYPE;
    }
    else if (PT_G711A == stAencAttr.enType || PT_G711U == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAencG711;
    }
    else if (PT_G726 == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAencG726;
        stAencG726.enG726bps    = G726_BPS;
    }
    else if (PT_G729 == stAencAttr.enType)
    {
    }
    else if (PT_LPCM == stAencAttr.enType)
    {
        stAencAttr.pValue = &stAencLpcm;
    }
    else if (PT_AAC == stAencAttr.enType)
    {

        stAencAttr.pValue = &stAencAac;
        stAencAac.enBitRate = 128000;
        stAencAac.enBitWidth = AUDIO_BIT_WIDTH_16;
        stAencAac.enSmpRate = pstAioAttr->enSamplerate;
        stAencAac.enSoundMode = pstAioAttr->enSoundmode;
        stAencAac.s16BandWidth = 0;
    }
    else
    {
        AUDIOENC_ERR("%s: invalid aenc payload type:%d\n", __FUNCTION__, stAencAttr.enType);
        return EI_FAILURE;
    }

    for (i = 0; i < s32AencChnCnt; i++) {
        AeChn = i;
        s32Ret = EI_MI_AENC_CreateChn(AeChn, &stAencAttr);
        if (EI_SUCCESS != s32Ret)
        {
            AUDIOENC_ERR("%s: EI_MI_AENC_CreateChn(%d) failed with %#x!\n", __FUNCTION__, AeChn, s32Ret);
            return s32Ret;
        }
    }

    return EI_SUCCESS;
}

static EI_S32 AUDIO_StopAenc(EI_S32 s32AencChnCnt)
{
    EI_S32 i;
    EI_S32 s32Ret;

    for (i = 0; i < s32AencChnCnt; i++)
    {
        s32Ret = EI_MI_AENC_DestroyChn(i);
        if (EI_SUCCESS != s32Ret)
        {
            AUDIOENC_ERR("%s: EI_MI_AENC_DestroyChn(%d) failed with %#x!\n", __FUNCTION__, i, s32Ret);
            return s32Ret;
        }

    }

    return EI_SUCCESS;
}

static EI_S32 AUDIO_AencLinkAi(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    MDP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId = EI_ID_AUDIOIN;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = EI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;

    return EI_MI_MLINK_Link(&stSrcChn, &stDestChn);
}

static EI_S32 AUDIO_AencUnbindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    MDP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId = EI_ID_AUDIOIN;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = EI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;

    return EI_MI_MLINK_UnLink(&stSrcChn, &stDestChn);
}

static EI_VOID *_get_aenc_stream_proc(EI_VOID *p)
{
    EI_S32 s32Ret;
    AUDIO_STREAM_S stStream;
    fd_set read_fds;
    EI_S32 AencFd;
    struct timeval TimeoutVal;

    MediaAudioStream_t* handle = (MediaAudioStream_t*)p;

    FD_ZERO(&read_fds);
    AencFd = EI_MI_AENC_GetFd(handle->aenChn);
    FD_SET(AencFd, &read_fds);

    gMediaAudioStreamer.bThreadStart = TRUE;

    while (gMediaAudioStreamer.bThreadStart)
    {
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(AencFd, &read_fds);

        s32Ret = select(AencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        
        if (s32Ret <= 0)
        {
            continue;
        }

        if (FD_ISSET(AencFd, &read_fds))
        {
            s32Ret = EI_MI_AENC_GetStream(handle->aenChn, &stStream, -1);

            if (EI_SUCCESS != s32Ret)
            {
                continue;
            }

            if (handle->callback)
            {
                handle->callback(handle->bindVenc, (uint8_t*)&stStream, stStream.u32Len);
            }
            
            EI_MI_AENC_ReleaseStream(handle->aenChn, &stStream);
        }
    }

    return NULL;
}

/* for aec, we only support one chn mono mode, 16 bit, 8k or 16k */
EI_S32 MediaAudio_EncorderInit(AI_CHN AiChn, AUDIO_DEV AiDev, AENC_CHN AeChn)
{
    EI_S32 s32Ret;
    EI_S32      s32AiChnCnt;
    EI_S32      s32AencChnCnt;
    AIO_ATTR_S stAioAttr;
    AI_VQE_CONFIG_S stAiVqeAttr;

    /*  1. start ai with vqe atrrs */
    stAioAttr.enSamplerate   = MEDIA_AUDIO_RATE;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = 1024;
    stAioAttr.u32ChnCnt      = 1;
    s32AiChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;

    /*  if you want to enable vqe funcs, such as aec,anr, just past the */
    /*  AiVqeAttr to ai, and enable the vqe. */
    memset(&stAiVqeAttr, 0, sizeof(AI_VQE_CONFIG_S));
    stAiVqeAttr.s32WorkSampleRate    = MEDIA_AUDIO_RATE;
    stAiVqeAttr.s32FrameSample       = stAioAttr.u32PtNumPerFrm; /* SAMPLE_AUDIO_PTNUMPERFRM; */
    stAiVqeAttr.enWorkstate          = VQE_WORKSTATE_COMMON;
    stAiVqeAttr.u32OpenMask = AI_VQE_MASK_AEC;/*  if you want to use anr, you can | AI_VQE_MASK_ANR here */
    s32Ret = AUDIO_StartAi(AiDev, stAioAttr.u32ChnCnt, &stAioAttr, AUDIO_SAMPLE_RATE_BUTT, EI_FALSE, &stAiVqeAttr);
    if (s32Ret)
    {
        AUDIOENC_ERR("ai start err %d!!\n", s32Ret);
        return -1;
    }

    /*  2. start aenc with encode type ** as an example */
    s32AencChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32Ret = AUDIO_StartAenc(s32AencChnCnt, &stAioAttr, MEDIA_AUDIO_ENCTYPE);
    if (s32Ret != EI_SUCCESS)
    {
        AUDIOENC_ERR("AENC start err %d!!\n", s32Ret);
        goto EXIT1;
    }

    /*  3. link ai to aenc */
    s32Ret = AUDIO_AencLinkAi(AiDev, AiChn, AeChn);
    if (s32Ret != EI_SUCCESS)
    {
        AUDIOENC_ERR("Aenc link to ai err %d!!\n", s32Ret);
        goto EXIT2;
    }

    gMediaAudioStreamer.aenChn = AeChn;

    s32Ret = pthread_create(&gMediaAudioStreamer.aencPid, 0, _get_aenc_stream_proc, (EI_VOID *)&gMediaAudioStreamer);
    if (s32Ret)
    {
        AUDIOENC_ERR("_get_aenc_stream_proc start err %d\n", s32Ret);
        goto EXIT3;
    }

    return 0;

EXIT3:
    s32Ret = AUDIO_AencUnbindAi(AiDev, AiChn, AeChn);
    if (s32Ret != EI_SUCCESS)
    {
        AUDIOENC_ERR("AUDIO_AencUnbindAi err %d\n", s32Ret);
    }
EXIT2:
    s32Ret = AUDIO_StopAenc(s32AencChnCnt);
    if (s32Ret != EI_SUCCESS)
    {
        AUDIOENC_ERR("stop AENC err %d\n", s32Ret);
    }

EXIT1:
    s32Ret = AUDIO_StopAi(AiDev, s32AiChnCnt, EI_FALSE, EI_TRUE);
    if (s32Ret)
    {
        AUDIOENC_ERR("stop ai err %d\n", s32Ret);
    }

    return 0;
}

void MediaAudio_Start(int bindVenc, audioFrameCallback callback)
{
    memset(&gMediaAudioStreamer, 0, sizeof(gMediaAudioStreamer));

    gMediaAudioStreamer.bindVenc = bindVenc;
    gMediaAudioStreamer.callback = callback;

    MediaAudio_EncorderInit(0, 0, 0);
}

unsigned char decAoStreamBuffer[2048] = { 0 };
int MediaAudio_DecAoStart(PAYLOAD_TYPE_E type, int chnNum, int rate)
{
    EI_S32      s32Ret;
    AO_CHN      AoChn = 0;
    ADEC_CHN    AdChn = 0;
    EI_S32      s32AoChnCnt;
    AIO_ATTR_S stAioAttr;

    AUDIO_DEV   AoDev = SAMPLE_AUDIO_INNER_AO_DEV;
    stAioAttr.enSamplerate   = rate;    /* AUDIO_SAMPLE_RATE_48000; */
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = 1024;
    stAioAttr.u32ChnCnt      = chnNum; /* 2; */
    if (chnNum == 2)
	{
        stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_STEREO;
    }
	else
	{
        stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    }

    s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdChn, &stAioAttr, PT_AAC);
    if (s32Ret != EI_SUCCESS)
	{
        goto ADECAO_ERR3;
    }

    s32AoChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, rate, EI_FALSE);
    if (s32Ret != EI_SUCCESS)
	{
        goto ADECAO_ERR2;
    }
	
    s32Ret = SAMPLE_COMM_AUDIO_AoLinkAdec(AoDev, AoChn, AdChn);
    if (s32Ret != EI_SUCCESS)
	{
        goto ADECAO_ERR1;
    }
	
	EI_MI_AO_SetMute(AoDev, EI_FALSE, NULL);
	EI_MI_AO_SetVolume(AoDev, 8);
	EI_MI_AO_EnableVqe(AoDev, AoChn);
	
	return EI_SUCCESS;

    s32Ret = SAMPLE_COMM_AUDIO_AoUnbindAdec(AoDev, AoChn, AdChn);
    if (s32Ret != EI_SUCCESS)
	{
    }
	
ADECAO_ERR1:
    s32AoChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32Ret |= SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, EI_FALSE);
    if (s32Ret != EI_SUCCESS) 
	{
    }
ADECAO_ERR2:

    s32Ret |= SAMPLE_COMM_AUDIO_StopAdec(AdChn);
    if (s32Ret != EI_SUCCESS)
	{
    }
ADECAO_ERR3:
    return EI_FAILURE;
}

int MediaAudio_DecAoStop(int chnNum)
{
    EI_S32      s32Ret;
    AO_CHN      AoChn = 0;
    ADEC_CHN    AdChn = 0;
    EI_S32      s32AoChnCnt;
    EI_S32 s32AdecChn = 0;
	
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_INNER_AO_DEV;
	
    if (chnNum == 2)
	{
        s32AoChnCnt = chnNum >> AUDIO_SOUND_MODE_STEREO;
    }
	else
	{
        s32AoChnCnt = chnNum >> AUDIO_SOUND_MODE_MONO;
    }

	EI_MI_AO_DisableVqe(AoDev, AoChn);

	EI_MI_ADEC_SendEndOfStream(s32AdecChn, EI_FALSE);

	SAMPLE_COMM_AUDIO_AoUnbindAdec(AoDev, AoChn, AdChn);

	SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, EI_FALSE);

	SAMPLE_COMM_AUDIO_StopAdec(AdChn);
	
	return EI_SUCCESS;
}

void MediaAudio_DecAoSendFrame(char* frame, int frameLen)
{
    AUDIO_STREAM_S stAudioStream = {0};
    EI_S32 s32AdecChn = 0;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_INNER_AO_DEV;
	
	stAudioStream.u32Len = frameLen;

	memcpy(decAoStreamBuffer, frame, frameLen);
	
    stAudioStream.pStream = decAoStreamBuffer;

	if (EI_SUCCESS != EI_MI_ADEC_SendStream(s32AdecChn, &stAudioStream, EI_TRUE))
	{
        printf("%s: EI_MI_ADEC_SendStream(%d) failed\n", __FUNCTION__, s32AdecChn);
	}
}

