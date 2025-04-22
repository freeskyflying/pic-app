/*
 *------------------------------------------------------------------------------
 * @File      :    mdp_aenc.c
 * @Date      :    2021-10-12
 * @Author    :    lomboswer <lomboswer@lombotech.com>
 * @Brief     :    Source file for MDP(Media Development Platform).
 *
 * Copyright (C) 2020-2021, LomboTech Co.Ltd. All rights reserved.
 *------------------------------------------------------------------------------
 */

#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "ei_posix.h"

#include "sample_comm.h"

struct speech_wav_header {
    char riff_id[4];            /* "RIFF" */
    uint32_t size0;             /* file len - 8 */
    char wave_fmt[8];           /* "WAVEfmt " */
    unsigned int size1;             /* 0x10 */
    unsigned short fmttag;            /* 0x01 */
    unsigned short channel;           /* 1 */
    unsigned int samplespersec;     /* 8000 */
    unsigned int bytepersec;        /* 8000 * 2 */
    unsigned short blockalign;        /* 1 * 16 / 8 */
    unsigned short bitpersamples;     /* 16 */
    char data_id[4];            /* "data" */
    unsigned int size2;             /* file len - 44 */
};

typedef struct eiAudio_Suffix_S {
    char *pSuffix;
    PAYLOAD_TYPE_E enType;
} Audio_Suffix_S;

Audio_Suffix_S g_AudioSuffix[] = {
    {"wav", PT_UNINIT},
    {"g711a", PT_G711A},
    {"g711u", PT_G711U},
    {"adpcm", PT_ADPCMA},
    {"g726", PT_G726},
    {"g729", PT_G729},
    {"pcm", PT_LPCM},
    {"aac", PT_AAC},
    {"mp3", PT_MP3},
    {NULL, PT_BUTT},
};

static char *SAMPLE_AUDIO_Pt2Str(PAYLOAD_TYPE_E enType)
{
    int i = 0;

    while (g_AudioSuffix[i].pSuffix != NULL) {
        if (g_AudioSuffix[i].enType == enType) {
            return g_AudioSuffix[i].pSuffix;
        }
        i++;
    }

    return "data";
}

static void speech_wav_init_header(struct speech_wav_header *header,
    unsigned int channel, int samplespersec, int bitpersamples)
{
    memcpy(header->riff_id, "RIFF", 4);
    header->size0           = 0;       /* Final file size not known yet, write 0 */
    memcpy(header->wave_fmt, "WAVEfmt ", 8);
    header->size1           = 16;               /* Sub-chunk size, 16 for PCM */
    header->fmttag          = 1;                /* AudioFormat, 1 for PCM */
    /* Number of channels, 1 for mono, 2 for stereo */
    header->channel         = channel;
    header->samplespersec   = samplespersec;    /* Sample rate */
    /*Byte rate */
    header->bytepersec      = samplespersec * bitpersamples * channel / 8;
    /* Block align, NumberOfChannels*BitsPerSample/8 */
    header->blockalign      = channel * bitpersamples / 8;
    header->bitpersamples   = bitpersamples;
    memcpy(header->data_id, "data", 4);
    header->size2           = 0;
}

static void speech_wav_upgrade_size(struct speech_wav_header *header,
    unsigned int paylodsize)
{
    header->size0           = paylodsize + 36;
    header->size2           = paylodsize;
}

static EI_S64 _get_time()
{
    struct timeval time;

    gettimeofday(&time, NULL);
    return time.tv_sec*1e6 + time.tv_usec;
}

/******************************************************************************
* function : Start Ai
******************************************************************************/
static EI_S32 AUDIO_StartAi(AUDIO_DEV AiDevId, EI_S32 s32AiChnCnt,
    AIO_ATTR_S *pstAioAttr, AUDIO_SAMPLE_RATE_E enOutSampleRate, EI_BOOL bResampleEn,
    EI_VOID *pstAiVqeAttr)
{
    EI_S32 i;
    EI_S32 s32Ret;
    /* Left 5dB Right 0dB, but real value is 5 * 1.5 = 7.5dB.*/
    EI_S32 as32VolumeDb[2] = {5, 0};
    EI_S32 s32Length = (sizeof(as32VolumeDb) / sizeof(as32VolumeDb[0]));

    s32Ret = EI_MI_AUDIOIN_Init(AiDevId);
    if (EI_SUCCESS != s32Ret) {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return s32Ret;
    }

    s32Ret = EI_MI_AI_SetPubAttr(AiDevId, pstAioAttr);
    if (s32Ret) {
        printf("%s: EI_MI_AI_SetPubAttr(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }

    s32Ret = EI_MI_AI_Enable(AiDevId);
    if (s32Ret) {
        printf("%s: EI_MI_AI_Enable(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }

    s32Ret = EI_MI_AI_SetVolume(AiDevId, s32Length, as32VolumeDb);
    if (s32Ret) {
        printf("%s: EI_MI_AI_SetVolume(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }

    for (i = 0; i < s32AiChnCnt >> pstAioAttr->enSoundmode; i++) {
        s32Ret = EI_MI_AI_EnableChn(AiDevId, i);
        if (s32Ret) {
            printf("%s: EI_MI_AI_EnableChn(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
            return s32Ret;
        }

        if (EI_TRUE == bResampleEn) {
            s32Ret = EI_MI_AI_EnableReSmp(AiDevId, i, enOutSampleRate);
            if (s32Ret) {
                printf("%s: EI_MI_AI_EnableReSmp(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
                return s32Ret;
            }
        }

        if (NULL != pstAiVqeAttr) {
            s32Ret = EI_MI_AI_SetVqeAttr(AiDevId, i, 0, 0, pstAiVqeAttr);
            if (s32Ret) {
                printf("%s: SetAiVqe(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
                return s32Ret;
            }

            s32Ret = EI_MI_AI_EnableVqe(AiDevId, i);
            if (s32Ret) {
                printf("%s: EI_MI_AI_EnableVqe(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
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

    for (i = 0; i < s32AiChnCnt; i++) {
        if (EI_TRUE == bResampleEn) {
            s32Ret = EI_MI_AI_DisableReSmp(AiDevId, i);
            if (EI_SUCCESS != s32Ret) {
                printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
                return s32Ret;
            }
        }

        if (EI_TRUE == bVqeEn) {
            s32Ret = EI_MI_AI_DisableVqe(AiDevId, i);
            if (EI_SUCCESS != s32Ret) {
                printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
                return s32Ret;
            }
        }

        s32Ret = EI_MI_AI_DisableChn(AiDevId, i);
        if (EI_SUCCESS != s32Ret) {
            printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
            return s32Ret;
        }
    }

    s32Ret = EI_MI_AI_Disable(AiDevId);
    if (EI_SUCCESS != s32Ret) {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return s32Ret;
    }

    s32Ret = EI_MI_AUDIOIN_Exit(AiDevId);
    if (EI_SUCCESS != s32Ret) {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return s32Ret;
    }
    printf("EI_MI_AUDIOIN_Exit %d ok\n", __LINE__);

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

    if (PT_ADPCMA == stAencAttr.enType) {
        stAencAttr.pValue       = &stAdpcmAenc;
        stAdpcmAenc.enADPCMType = AUDIO_ADPCM_TYPE;
    } else if (PT_G711A == stAencAttr.enType || PT_G711U == stAencAttr.enType) {
        stAencAttr.pValue       = &stAencG711;
    } else if (PT_G726 == stAencAttr.enType) {
        stAencAttr.pValue       = &stAencG726;
        stAencG726.enG726bps    = G726_BPS;
    } else if (PT_G729 == stAencAttr.enType) {
    } else if (PT_LPCM == stAencAttr.enType) {
        stAencAttr.pValue = &stAencLpcm;
    } else if (PT_AAC == stAencAttr.enType) {

        stAencAttr.pValue = &stAencAac;
        stAencAac.enBitRate = 128000;
        stAencAac.enBitWidth = AUDIO_BIT_WIDTH_16;
        stAencAac.enSmpRate = pstAioAttr->enSamplerate;
        stAencAac.enSoundMode = pstAioAttr->enSoundmode;
        stAencAac.s16BandWidth = 0;
    } else {
        printf("%s: invalid aenc payload type:%d\n", __FUNCTION__, stAencAttr.enType);
        return EI_FAILURE;
    }

    for (i = 0; i < s32AencChnCnt; i++) {
        AeChn = i;

        s32Ret = EI_MI_AENC_CreateChn(AeChn, &stAencAttr);
        if (EI_SUCCESS != s32Ret) {
            printf("%s: EI_MI_AENC_CreateChn(%d) failed with %#x!\n", __FUNCTION__,
                AeChn, s32Ret);
            return s32Ret;
        }
    }

    return EI_SUCCESS;
}

static EI_S32 AUDIO_StopAenc(EI_S32 s32AencChnCnt)
{
    EI_S32 i;
    EI_S32 s32Ret;

    for (i = 0; i < s32AencChnCnt; i++) {
        s32Ret = EI_MI_AENC_DestroyChn(i);
        if (EI_SUCCESS != s32Ret) {
            printf("%s: EI_MI_AENC_DestroyChn(%d) failed with %#x!\n", __FUNCTION__,
                i, s32Ret);
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

EI_S32 Sample_AUDIO_RecordWave(EI_CHAR *wavfile, EI_U32 chn, EI_U32 rate, EI_U32 bitwidth, EI_U32 duration)
{
    struct speech_wav_header wav_header;
    AIO_ATTR_S stAioAttr;
    AUDIO_FRAME_S stFrame;
    AI_CHN_PARAM_S stAiChnPara;
    EI_S32 s32AiChnCnt;
    /* we only test the one chn mono mode or 2 chn stereo mode here */
    AI_CHN      AiChn = 0;
    AUDIO_DEV   AiDev = 0;
    FILE *fp = EI_NULL;
    EI_S32 s32Ret = EI_SUCCESS;
    EI_S32 total_len;
    EI_S32 record_len;

    fp = fopen(wavfile, "wb+");
    if (fp == EI_NULL) {
        printf("open file %s err!!\n", wavfile);
        return -1;
    }
    /* init & write the wav file header */
    speech_wav_init_header(&wav_header, chn, rate, 16);
    fwrite(&wav_header, sizeof(struct speech_wav_header), 1, fp);

    total_len = duration * rate * chn * 16 >> 3;
    record_len = 0;

    stAioAttr.enSamplerate   = rate;
    stAioAttr.enBitwidth     = bitwidth; /* only support 16bit now */
    /* we only test the one chn mono mode or 2 chn stereo mode here */
    if (chn == 2) {
        stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_STEREO;
    } else {
        stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    }
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = 1024;
    stAioAttr.u32ChnCnt      = chn;
    s32AiChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;

    s32Ret = AUDIO_StartAi(AiDev, stAioAttr.u32ChnCnt, &stAioAttr, AUDIO_SAMPLE_RATE_BUTT, EI_FALSE, NULL);
    if (s32Ret) {
        printf("ai start err %d!!\n", s32Ret);
        return -1;
    }

    /* for user mode, you need to set the frame depth */
    stAiChnPara.u32UsrFrmDepth = 30;
    s32Ret = EI_MI_AI_SetChnParam(AiDev, AiChn, &stAiChnPara);
    if (EI_SUCCESS != s32Ret) {
        printf("%s: set ai chn param failed\n", __FUNCTION__);
        return -1;
    }

    while (record_len < total_len) {
        s32Ret = EI_MI_AI_GetFrame(AiDev, AiChn, &stFrame, NULL, 100);
        if (EI_SUCCESS != s32Ret) {
            usleep(5000);
            printf("FUN: %s line:%d len %d,RET:%d!\n", __FUNCTION__, __LINE__, stFrame.u32Len, s32Ret);
            continue;
        }
        s32Ret = fwrite(stFrame.u64VirAddr, 1, stFrame.u32Len, fp);
        if (s32Ret != stFrame.u32Len) {
            printf("write file err ret %d\n", s32Ret);
            EI_MI_AI_ReleaseFrame(AiDev, AiChn, &stFrame, NULL);
            break;
        }
        record_len += stFrame.u32Len;
        /* finally you must release the stream */
        s32Ret = EI_MI_AI_ReleaseFrame(AiDev, AiChn, &stFrame, NULL);
        if (EI_SUCCESS != s32Ret) {
            printf("%s: EI_MI_AI_ReleaseFrame(%d, %d), failed with %#x!\n",
                __FUNCTION__, AiDev, AiChn, s32Ret);
            break;
        }
    }

    /* upgrage wav header */
    fseek(fp, 0, SEEK_SET);
    speech_wav_upgrade_size(&wav_header, record_len);
    fwrite(&wav_header, sizeof(struct speech_wav_header), 1, fp);
    fclose(fp);

    /* for user mode we need to set the frame depth to 0 before we stop the ai */
    stAiChnPara.u32UsrFrmDepth = 0;
    s32Ret = EI_MI_AI_SetChnParam(AiDev, AiChn, &stAiChnPara);
    if (EI_SUCCESS != s32Ret) {
        printf("%s: set ai chn param failed\n", __FUNCTION__);
        return EI_FAILURE;
    }
    s32Ret = AUDIO_StopAi(AiDev, s32AiChnCnt, EI_FALSE, EI_FALSE);
    if (s32Ret) {
        printf("stop ai err %d\n", s32Ret);
    }

    return s32Ret;
}

/*  record pcm data with aec processed, and encode it into g711a format. */
/*  we use linked mode here, so you don't need to deal with comunication */
/*  between ai and aenc. */
/*  you can use Sample_AUDIO_PlayWave to playa wavfile in the background, */
/*  then use this func to record a wavfile. The recorded */
/*  voice will cancel the the voice from playback */
EI_S32 Sample_AI_AENC_AEC(EI_CHAR *encfile, EI_U32 rate, EI_U32 duration, PAYLOAD_TYPE_E enType)
{
    EI_S32 s32Ret;
    EI_S32      s32AiChnCnt;
    EI_S32      s32AencChnCnt;
    /* for aec, we only support one chn mono mode, 16 bit, 8k or 16k */
    AI_CHN      AiChn = 0;
    AUDIO_DEV   AiDev = 0;
    AENC_CHN    AeChn = 0;
    FILE        *fp = EI_NULL;
    AIO_ATTR_S stAioAttr;
    AI_VQE_CONFIG_S stAiVqeAttr;
    EI_S64 start_time;
    AUDIO_STREAM_S stStream;
    fd_set read_fds;
    EI_S32 AencFd;
    struct timeval TimeoutVal;

    fp = fopen(encfile, "wb+");
    if (fp == EI_NULL) {
        printf("open file %s err!!\n", encfile);
        return -1;
    }

    /*  1. start ai with vqe atrrs */
    stAioAttr.enSamplerate   = rate;
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
    stAiVqeAttr.s32WorkSampleRate    = rate;
    stAiVqeAttr.s32FrameSample       = stAioAttr.u32PtNumPerFrm; /* SAMPLE_AUDIO_PTNUMPERFRM; */
    stAiVqeAttr.enWorkstate          = VQE_WORKSTATE_COMMON;
    stAiVqeAttr.u32OpenMask = AI_VQE_MASK_AEC;/*  if you want to use anr, you can | AI_VQE_MASK_ANR here */
    s32Ret = AUDIO_StartAi(AiDev, stAioAttr.u32ChnCnt, &stAioAttr,
        AUDIO_SAMPLE_RATE_BUTT, EI_FALSE, &stAiVqeAttr);
    if (s32Ret) {
        printf("ai start err %d!!\n", s32Ret);
        return -1;
    }

    /*  2. start aenc with encode type ** as an example */
    s32AencChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32Ret = AUDIO_StartAenc(s32AencChnCnt, &stAioAttr, enType);
    if (s32Ret != EI_SUCCESS) {
        printf("AENC start err %d!!\n", s32Ret);
        goto EXIT1;
    }

    /*  3. link ai to aenc */
    s32Ret = AUDIO_AencLinkAi(AiDev, AiChn, AeChn);
    if (s32Ret != EI_SUCCESS) {
        printf("Aenc link to ai err %d!!\n", s32Ret);
        goto EXIT2;
    }

    /*  4. get enc stream and write to file */
    start_time = _get_time();
    FD_ZERO(&read_fds);
    AencFd = EI_MI_AENC_GetFd(AeChn);
    FD_SET(AencFd, &read_fds);
    while (1) {
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(AencFd, &read_fds);

        s32Ret = select(AencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        /* printf("####func:%s,line:%d,ret:%d\n",__FUNCTION__,__LINE__,s32Ret); */
        if (s32Ret < 0) {
            break;
        } else if (0 == s32Ret) {
            printf("%s: get aenc stream select time out\n", __FUNCTION__);
            break;
        }

        if (FD_ISSET(AencFd, &read_fds)) {
            s32Ret = EI_MI_AENC_GetStream(AeChn, &stStream, -1);
            if (EI_SUCCESS != s32Ret) {
                printf("%s: EI_MI_AENC_GetStream(%d), failed with %#x!\n", \
                    __FUNCTION__, AeChn, s32Ret);
                goto EXIT3;
            }

            if (fp) {
                s32Ret = fwrite(stStream.pStream, 1, stStream.u32Len, fp);
            }

            /* fflush(pstAencCtl->pfd); */
            /* EI_TRACE_ADEC(EI_DBG_ERR, "##########fwrite:%d,\n",stStream.u32Len); */

            s32Ret = EI_MI_AENC_ReleaseStream(AeChn, &stStream);
            if (EI_SUCCESS != s32Ret) {
                printf("%s: EI_MI_AENC_ReleaseStream(%d), failed with %#x!\n", \
                    __FUNCTION__, AeChn, s32Ret);
                goto EXIT3;
            }
        }

        if ((_get_time() - start_time)/1000000 > duration) {
            printf("reach record time %d\n", duration);
            break;
        }
    }

    if (fp)
        fclose(fp);

EXIT3:
    s32Ret = AUDIO_AencUnbindAi(AiDev, AiChn, AeChn);
    if (s32Ret != EI_SUCCESS) {
        printf("AUDIO_AencUnbindAi err %d\n", s32Ret);
    }
EXIT2:
    s32Ret = AUDIO_StopAenc(s32AencChnCnt);
    if (s32Ret != EI_SUCCESS) {
        printf("stop AENC err %d\n", s32Ret);
    }

EXIT1:
    s32Ret = AUDIO_StopAi(AiDev, s32AiChnCnt, EI_FALSE, EI_TRUE);
    if (s32Ret) {
        printf("stop ai err %d\n", s32Ret);
    }
    printf("Sample_AI_AENC_AEC end!!\n");

    return 0;
}

static EI_VOID aenc_help(EI_VOID)
{
    printf("Version:1.0.0  \n");
    printf("such as: mdp_aenc -c 1 -r 48000 -t 20 -e 3\n");
    printf("such as: mdp_aenc -c 1 -r 48000 -t 20 -e 3 -f /data/audio-48000.g711a\n");
    printf("arguments:\n");
    printf("-c    select chn, default:1, support:1~2, 1:MONOã€?:STEREO\n");
    printf("-r    select sampling rate, default:48000,support:8000/12000/11025/16000/22050/24000/32000/44100/48000/64000/96000\n");
    printf("-t    select record time, default:10, support:1~\n");
    printf("-e    select audio type, default:0, support:0~3, 0:waveã€?:pcmã€?:aacã€?:g711a\n");
    printf("-f    select file name, no default\n");
}

int main(int argc, char *argv[])
{
    EI_S32 s32Ret = EI_SUCCESS;
    char aszFileName[128];
    EI_CHAR *audiofile = NULL;
    EI_U32 chn = 1;     /* 1 chn mono normal, 2 chn stereo audio exception */
    EI_U32 rate = 48000;
    EI_U32 bitwidth = AUDIO_BIT_WIDTH_16;   /* only support 16bit now */
    EI_U32 duration = 10;
    PAYLOAD_TYPE_E enType = PT_UNINIT;     /* PT_UNINIT:wave */
    int c;

    if (argc < 3) {
        aenc_help();
        goto EXIT;
    }

    while ((c = getopt(argc, argv, "f:c:r:t:e:")) != -1) {
        switch (c) {
        case 'f':
            audiofile = (EI_CHAR *)argv[optind - 1];
            break;
        case 'c':
            chn = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'r':
            rate = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 't':
            duration = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'e':
            enType = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        default:
            EI_PRINT("Arguments error!\n");
            aenc_help();
            goto EXIT;
        }
    }

    /* signal(SIGINT, SAMPLE_AUDIO_HandleSig); */
    /* signal(SIGTERM, SAMPLE_AUDIO_HandleSig); */

    if (enType == 0) {
        enType = PT_UNINIT;
    } else if (enType == 1) {
        enType = PT_LPCM;       /* support */
    } else if (enType == 2) {
        enType = PT_AAC;        /* support */
    } else if (enType == 3) {
        enType = PT_G711A;      /* support */
    } else if (enType == 4) {
        enType = PT_G726;       /* support */
    } else if (enType == 5) {
        enType = PT_G729;       /* support */
    } else if (enType == 6) {
        enType = PT_ADPCMA;     /* encode support,decode not support */
    } else if (enType == 7) {
        enType = PT_MP3;        /* encode not support */
    } else if (enType == 8) {
        enType = PT_G711U;      /* support */
    } else {
        EI_PRINT("invalid aenc payload type!\n");
        goto EXIT;
    }

    if (audiofile == NULL) {
        snprintf(aszFileName, 128, "/data/audio-%d.%s", rate, SAMPLE_AUDIO_Pt2Str(enType));
        audiofile = aszFileName;
    }

    EI_MI_MBASE_Init();
    EI_MI_VBUF_Init();
    EI_MI_MLINK_Init();

    if (PT_UNINIT == enType) {
        s32Ret = Sample_AUDIO_RecordWave(audiofile, chn, rate, bitwidth, duration);
    } else {
        s32Ret = Sample_AI_AENC_AEC(audiofile, rate, duration, enType);
    }

    if (EI_SUCCESS == s32Ret) {
        EI_PRINT("Generate %s \n", audiofile);
    }

    EI_MI_MLINK_Exit();
    EI_MI_VBUF_Exit();
    EI_MI_MBASE_Exit();

EXIT:
    return s32Ret;
}


