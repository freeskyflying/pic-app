/*
 *------------------------------------------------------------------------------
 * @File      :    mdp_adec.c
 * @Date      :    2021-10-12
 * @Author    :    lomboswer <lomboswer@lombotech.com>
 * @Brief     :    Source file for MDP(Media Development Platform).
 *
 * Copyright (C) 2020-2021, LomboTech Co.Ltd. All rights reserved.
 *------------------------------------------------------------------------------
 */


#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "ei_posix.h"

#include "sample_comm.h"

#define AUDIO_SAMPLES_PER_FRAME 1024

#define SAMPLE_DBG(s32Ret)\
    do{\
        printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__);\
    }while (0)

#define SAMPLE_RES_CHECK_NULL_PTR(ptr)\
    do{\
        if (NULL == (EI_U8*)(ptr) )\
        {\
            printf("ptr is NULL,fuc:%s,line:%d\n", __FUNCTION__, __LINE__);\
            return NULL;\
        }\
    }while (0)

static PAYLOAD_TYPE_E gs_enPayloadType = PT_LPCM;

static EI_BOOL gs_bAioReSample  = EI_FALSE;
static EI_BOOL gs_bUserGetMode  = EI_FALSE;

static AUDIO_SAMPLE_RATE_E enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
static AUDIO_SAMPLE_RATE_E enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
static VBUF_CONFIG_S stAbConfig;

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

struct riff_wave_header {
    unsigned int riff_id;
    unsigned int riff_sz;
    unsigned int wave_id;
};

struct chunk_header {
    unsigned int id;
    unsigned int sz;
};

struct chunk_fmt {
    unsigned short audio_format;
    unsigned short num_channels;
    unsigned int sample_rate;
    unsigned int byte_rate;
    unsigned short block_align;
    unsigned short bits_per_sample;
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

static EI_S32 SAMPLE_AUDIO_Name2Id(char *name, PAYLOAD_TYPE_E *enType)
{
    char *suffix = NULL;
    int i = 0;
    int found = 0;

    suffix = strrchr(name, '.');
    printf("name:%s,suffix:%s\n", name, suffix);
    if (suffix != NULL) {
        int len;
        suffix++;
        len = strlen(suffix);
        i = 0;
        while (g_AudioSuffix[i].pSuffix != NULL) {
            char *suf = g_AudioSuffix[i].pSuffix;
            while (suf != NULL) {
                if (!strncasecmp(suf, suffix, len) &&
                    (suf[len] == ',' || suf[len] == '\0')) {
                    found = 1;
                    break;
                }
                suf = strchr(suf, ',');
                if (suf != NULL) {
                    suf++;
                }
            }
            if (found) {
                break;
            }
            i++;
        }
    }

    if (found) {
        *enType = g_AudioSuffix[i].enType;
        return EI_SUCCESS;
    } else {
        printf("suffix %s not support,self is:%d \n", suffix, *enType);
        return EI_FAILURE;
    }
}


static FILE *SAMPLE_AUDIO_OpenPlayFile(EI_CHAR *paszFileName)
{
    FILE *pfd;
    EI_CHAR aszFileName[FILE_NAME_LEN] = {0};


#ifdef __EOS__
    snprintf(aszFileName, FILE_NAME_LEN, "/mnt/userdata/%s", paszFileName);
#else
    snprintf(aszFileName, FILE_NAME_LEN, "%s", paszFileName);
#endif

    if (aszFileName == NULL) {
        return NULL;
    }

    pfd = fopen(aszFileName, "rb");
    if (NULL == pfd) {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for play ok\n", aszFileName);
    return pfd;
}

/******************************************************************************
* function : Start Ao
******************************************************************************/
static EI_S32 AUDIO_StartAo(AUDIO_DEV AoDevId, EI_S32 s32AoChnCnt,
    AIO_ATTR_S *pstAioAttr, AUDIO_SAMPLE_RATE_E enInSampleRate, EI_BOOL bResampleEn)
{
    EI_S32 i;
    EI_S32 s32Ret;

    s32Ret = EI_MI_AUDIOOUT_Init(AoDevId);
    if (EI_SUCCESS != s32Ret) {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return s32Ret;
    }

    s32Ret = EI_MI_AO_SetPubAttr(AoDevId, pstAioAttr);
    if (EI_SUCCESS != s32Ret) {
        printf("%s: EI_MI_AO_SetPubAttr(%d) failed with %#x!\n", __FUNCTION__, \
            AoDevId, s32Ret);
        return EI_FAILURE;
    }
    printf("%s: EI_MI_AO_SetPubAttr ok \n", __FUNCTION__);

    s32Ret = EI_MI_AO_Enable(AoDevId);
    if (EI_SUCCESS != s32Ret) {
        printf("%s: EI_MI_AO_Enable(%d) failed with %#x!\n", __FUNCTION__, AoDevId, s32Ret);
        return EI_FAILURE;
    }
    printf("%s: EI_MI_AO_Enable ok \n", __FUNCTION__);

    for (i = 0; i < s32AoChnCnt >> pstAioAttr->enSoundmode; i++) {
        s32Ret = EI_MI_AO_EnableChn(AoDevId, i);
        if (EI_SUCCESS != s32Ret) {
            printf("%s: EI_MI_AO_EnableChn(%d) failed with %#x!\n", __FUNCTION__, i, s32Ret);
            return EI_FAILURE;
        }

        if (EI_TRUE == bResampleEn) {
            s32Ret = EI_MI_AO_DisableReSmp(AoDevId, i);
            s32Ret |= EI_MI_AO_EnableReSmp(AoDevId, i, enInSampleRate);
            if (EI_SUCCESS != s32Ret) {
                printf("%s: EI_MI_AO_EnableReSmp(%d,%d) failed with %#x!\n", __FUNCTION__, AoDevId, i, s32Ret);
                return EI_FAILURE;
            }
        }

    }

    return EI_SUCCESS;
}

/******************************************************************************
* function : Stop Ao
******************************************************************************/
static EI_S32 AUDIO_StopAo(AUDIO_DEV AoDevId, EI_S32 s32AoChnCnt, EI_BOOL bResampleEn)
{
    EI_S32 i;
    EI_S32 s32Ret;

    for (i = 0; i < s32AoChnCnt; i++) {
        if (EI_TRUE == bResampleEn) {
            s32Ret = EI_MI_AO_DisableReSmp(AoDevId, i);
            if (EI_SUCCESS != s32Ret) {
                printf("%s: EI_MI_AO_DisableReSmp failed with %#x!\n", __FUNCTION__, s32Ret);
                return s32Ret;
            }
        }

        s32Ret = EI_MI_AO_DisableChn(AoDevId, i);
        if (EI_SUCCESS != s32Ret) {
            printf("%s: EI_MI_AO_DisableChn failed with %#x!\n", __FUNCTION__, s32Ret);
            return s32Ret;
        }
    }

    s32Ret = EI_MI_AO_Disable(AoDevId);
    if (EI_SUCCESS != s32Ret) {
        printf("%s: EI_MI_AO_Disable failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    s32Ret = EI_MI_AUDIOOUT_Exit(AoDevId);
    if (EI_SUCCESS != s32Ret) {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return s32Ret;
    }
    printf("EI_MI_AUDIOOUT_Exit %d ok\n", __LINE__);

    return EI_SUCCESS;
}


EI_S32 Sample_AUDIO_PlayWave(EI_CHAR *wavfile)
{
    FILE *file = EI_NULL;
    struct riff_wave_header wave_header;
    struct chunk_header chunk_header;
    struct chunk_fmt chunk_fmt;
    unsigned int more_chunks = 1;
    AUDIO_DEV   AoDev = 0;
    AO_CHN      AoChn = 0;
    AIO_ATTR_S stAioAttr;
    EI_S32 s32AoChnCnt;
    VBUF_POOL_CONFIG_S vbufConfig;
    VIDEO_FRAME_INFO_S frameInfo;
    VBUF_POOL pool;
    VBUF_BUFFER bufid;
    EI_U32 u32ReadLen;
    EI_U8 *bufvirt = NULL;
    AUDIO_FRAME_S audioFrame;
    EI_S32 s32Ret = EI_SUCCESS;

    if (wavfile == EI_NULL) {
        printf("wavfile null\n");
        return -1;
    }
    file = fopen(wavfile, "rb");
    if (file == EI_NULL) {
        printf("open wavfile %s err!!\n", wavfile);
        return -1;
    }

    if (fread(&wave_header, sizeof(wave_header), 1, file) != 1) {
        printf("read wav header err!!\n");
        return -1;
    }
    if ((wave_header.riff_id != ID_RIFF) || (wave_header.wave_id != ID_WAVE)) {
        printf("wav header err!!\n");
        return -1;
    }

    do {
        if (fread(&chunk_header, sizeof(chunk_header), 1, file) != 1) {
            printf("chunk_header header err!!\n");
            return -1;
        }
        switch (chunk_header.id) {
        case ID_FMT:
            if (fread(&chunk_fmt, sizeof(chunk_fmt), 1, file) != 1) {
                printf("chunk_fmt header err!!\n");
                return -1;
            }
            /* If the format header is larger, skip the rest */
            if (chunk_header.sz > sizeof(chunk_fmt))
                fseek(file, chunk_header.sz -
                    sizeof(chunk_fmt), SEEK_CUR);
            break;
        case ID_DATA:
            /* Stop looking for chunks */
            more_chunks = 0;
            break;
        default:
            fseek(file, chunk_header.sz, SEEK_CUR);
            break;
        }
    } while (more_chunks);

    printf("chn %u, rate %u, fmt %u, datasize %u\n", chunk_fmt.num_channels,
        chunk_fmt.sample_rate, chunk_fmt.audio_format, chunk_header.sz);

    stAioAttr.enSamplerate   = chunk_fmt.sample_rate;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16; /* default set to 16 */
    if (chunk_fmt.num_channels == 2) {
        stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_STEREO;
    } else {
        stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    }
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = 1024;
    stAioAttr.u32ChnCnt      = chunk_fmt.num_channels;

    /* enable AO */
    s32AoChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, 0, EI_FALSE);
    if (s32Ret) {
        printf("start ao err!!\n");
        return -1;
    }

    /* create buf pool */
    vbufConfig.u32BufSize = 1024 * 16 * stAioAttr.u32ChnCnt / 8;
    vbufConfig.u32BufCnt = ABUF_POOL_NUM;
    vbufConfig.enVbufUid = VBUF_UID_COMMON;
    vbufConfig.enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    pool = EI_MI_VBUF_CreatePool(&vbufConfig);
    if (pool == VBUF_INVALID_POOL) {
        printf("create pool %ld err!\n", pool);
        return EI_ERR_AUDIOIN_NOMEM;
    }
    printf("create pool %ld\n", pool);


    /* set the buf frame info */
    frameInfo.enFrameType = MDP_FRAME_TYPE_AUDIO;
    frameInfo.stAudioFrameInfo.s32Channels =  stAioAttr.u32ChnCnt;
    frameInfo.stAudioFrameInfo.s32SampleRate = stAioAttr.enSamplerate;
    frameInfo.stAudioFrameInfo.s32BitsPeSample = 16;
    frameInfo.stAudioFrameInfo.u32TotalSize = vbufConfig.u32BufSize;
    s32Ret = EI_MI_VBUF_SetFrameInfo(pool, &frameInfo);
    if (s32Ret) {
        printf("Set frame info error %d failed!\n", vbufConfig.u32BufSize);
        return s32Ret;
    }

    while (1) {
        bufvirt = NULL;
        bufid = 0;
        /* get one buffer */
        bufid = EI_MI_VBUF_GetBuffer(pool, 100);
        if (bufid == VBUF_INVALID_BUFFER) {
            /*  printf("get buffer err %d\n", bufid); */
            continue;
        }
        /*  printf("get buf id %u\n", bufid); */
        s32Ret = EI_MI_VBUF_GetFrameInfo(bufid, &frameInfo);
        if (s32Ret != EI_SUCCESS) {
            printf("get frame info error.\n");
            EI_MI_VBUF_PutBuffer(bufid);
            continue;
        }
        bufvirt = (EI_U8 *) EI_MI_MBASE_Mmap(frameInfo.stAudioFrameInfo.u64PlanePhyAddr, vbufConfig.u32BufSize);
        if (bufvirt == NULL) {
            printf("MMAP error.\n");
            EI_MI_VBUF_PutBuffer(bufid);
            continue;
        }
        /* read pcm data to buf */
        u32ReadLen = fread(bufvirt, 1, vbufConfig.u32BufSize, file);
        if (u32ReadLen <= 0) {
            printf("read end or error.\n");
            EI_MI_MBASE_Munmap(bufvirt, vbufConfig.u32BufSize);
            EI_MI_VBUF_PutBuffer(bufid);
            break;
        }
        /*  printf("read file len %d\n", u32ReadLen); */
        /* send to ao dev */
        audioFrame.enBitwidth = AUDIO_BIT_WIDTH_16;
        audioFrame.enSoundmode = stAioAttr.enSoundmode;
        audioFrame.u32BufferId = bufid;
        audioFrame.u32Len = vbufConfig.u32BufSize;
        audioFrame.u32PoolId = pool;
        audioFrame.u64PhyAddr = frameInfo.stAudioFrameInfo.u64PlanePhyAddr;
        audioFrame.u64VirAddr = bufvirt;
        s32Ret = EI_MI_AO_SendFrame(AoDev, AoChn, &audioFrame, 1000);
        if (s32Ret != EI_SUCCESS) {
            printf("send frame err!\n");
            break;
        }

        EI_MI_MBASE_Munmap(bufvirt, vbufConfig.u32BufSize);
        EI_MI_VBUF_PutBuffer(bufid);
    }

    if (file) {
        fclose(file);
        file = EI_NULL;
    }

    s32AoChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    AUDIO_StopAo(AoDev, s32AoChnCnt, EI_FALSE);

    s32Ret = EI_MI_VBUF_DestroyPool(pool);
    if (s32Ret) {
        printf("destroy pool err!\n");
    }

    return s32Ret;
}

EI_S32 SAMPLE_AUDIO_AdecAo(EI_CHAR *aszFileName, EI_U32 chn, EI_U32 rate)
{

    EI_S32      s32Ret;
    AO_CHN      AoChn = 0;
    ADEC_CHN    AdChn = 0;
    EI_S32      s32AoChnCnt;
    FILE        *pfd = NULL;
    FILE        *pfo = NULL;
    AIO_ATTR_S stAioAttr;

    AUDIO_DEV   AoDev = SAMPLE_AUDIO_INNER_AO_DEV;
    stAioAttr.enSamplerate   = rate;    /* AUDIO_SAMPLE_RATE_48000; */
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = AUDIO_SAMPLES_PER_FRAME;
    stAioAttr.u32ChnCnt      = chn; /* 2; */
    if (chn == 2) {
        stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_STEREO;
    } else {
        stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    }

    gs_bAioReSample = EI_FALSE;
    enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;

    s32Ret = SAMPLE_AUDIO_Name2Id(aszFileName, &gs_enPayloadType);

    s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdChn, &stAioAttr, gs_enPayloadType);
    if (s32Ret != EI_SUCCESS) {
        SAMPLE_DBG(s32Ret);
        goto ADECAO_ERR3;
    }

    s32AoChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, enInSampleRate, gs_bAioReSample);
    if (s32Ret != EI_SUCCESS) {
        SAMPLE_DBG(s32Ret);
        goto ADECAO_ERR2;
    }

    if (EI_TRUE == gs_bUserGetMode) {
        s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAdeAo(AoDev, AoChn, AdChn, pfo);
        if (s32Ret != EI_SUCCESS) {
            SAMPLE_DBG(s32Ret);
            goto ADECAO_ERR1;
        }
    } else {
        s32Ret = SAMPLE_COMM_AUDIO_AoLinkAdec(AoDev, AoChn, AdChn);
        if (s32Ret != EI_SUCCESS) {
            SAMPLE_DBG(s32Ret);
            goto ADECAO_ERR1;
        }
    }


    pfd = SAMPLE_AUDIO_OpenPlayFile(aszFileName);
    if (!pfd) {
        SAMPLE_DBG(EI_FAILURE);
        goto ADECAO_ERR0;
    }

    s32Ret = SAMPLE_COMM_AUDIO_CreatTrdFileAdec(AdChn, pfd);
    if (s32Ret != EI_SUCCESS) {
        SAMPLE_DBG(s32Ret);
        goto ADECAO_ERR10;
    }

    printf("bind adec:%d to ao(%d,%d) ok \n", AdChn, AoDev, AoChn);

    printf("\nplease press twice ENTER to exit this sample\n");
#if defined(__EOS__)
    sleep(10);
#else
    getchar();
    getchar();
#endif
    printf("func:%s,line:%d\n", __FUNCTION__, __LINE__);
    s32Ret = SAMPLE_COMM_AUDIO_DestroyTrdFileAdec(AdChn);
    if (s32Ret != EI_SUCCESS) {
        SAMPLE_DBG(s32Ret);
        return EI_FAILURE;
    }
ADECAO_ERR10:
    if (pfd) {
        fclose(pfd);
        pfd = NULL;
    }
ADECAO_ERR0:
    if (EI_TRUE == gs_bUserGetMode) {
        s32Ret = SAMPLE_COMM_AUDIO_DestroyTrdAdec(AdChn);
        if (s32Ret != EI_SUCCESS) {
            SAMPLE_DBG(s32Ret);
        }
    } else {
        s32Ret = SAMPLE_COMM_AUDIO_AoUnbindAdec(AoDev, AoChn, AdChn);
        if (s32Ret != EI_SUCCESS) {
            SAMPLE_DBG(s32Ret);
        }
    }
ADECAO_ERR1:
    s32AoChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32Ret |= SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, gs_bAioReSample);
    if (s32Ret != EI_SUCCESS) {
        SAMPLE_DBG(s32Ret);
    }
ADECAO_ERR2:

    s32Ret |= SAMPLE_COMM_AUDIO_StopAdec(AdChn);
    if (s32Ret != EI_SUCCESS) {
        SAMPLE_DBG(s32Ret);
    }
ADECAO_ERR3:
    return s32Ret;
}

void SAMPLE_AUDIO_HandleSig(EI_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTERM == signo) {
        /* EI_S32 s32Ret = EI_SUCCESS; */

        SAMPLE_COMM_AUDIO_DestroyAllTrd();
        SAMPLE_COMM_SYS_Exit(&stAbConfig);
    }

    exit(0);
}

EI_VOID adec_help(EI_VOID)
{
    printf("Version:1.0.0  \n");
    printf("such as: mdp_adec -c 1 -r 48000 -f /data/audio-48000.g711a\n");
    printf("arguments:\n");
    printf("-c    select chn, default:1,support:1~2, 1:MONO、2:STEREO\n");
    printf("-r    select sampling rate,default:48000,support:8000/12000/11025/16000/22050/24000/32000/44100/48000/64000/96000\n");
    printf("-f    select file name, no default\n");
}

int main(int argc, char *argv[])
{
    EI_S32 s32Ret = EI_SUCCESS;
    EI_CHAR *audiofilepath = NULL;
    EI_U32 chn = 1;     /* 1 chn mono normal, 2 chn stereo audio exception */
    EI_U32 rate = 48000;
    PAYLOAD_TYPE_E enType = PT_BUTT;     /* PT_UNINIT:wave */
    int c;

    if (argc < 3) {
        adec_help();
        goto EXIT;
    }

    while ((c = getopt(argc, argv, "f:c:r:")) != -1) {
        switch (c) {
        case 'f':
            audiofilepath = (EI_CHAR *)argv[optind - 1];
            break;
        case 'c':
            chn = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        case 'r':
            rate = (unsigned int)strtol(argv[optind - 1], NULL, 10);
            break;
        default:
            EI_PRINT("Arguments error!\n");
            adec_help();
            goto EXIT;
        }
    }

    if (audiofilepath == NULL) {
        EI_PRINT("The audio file path cannot be empty!\n");
        goto EXIT;
    }

    s32Ret = SAMPLE_AUDIO_Name2Id(audiofilepath, &enType);
    if (s32Ret != EI_SUCCESS || enType == PT_BUTT) {
        EI_PRINT("suffix not support!\n");
        goto EXIT;
    }

    /* signal(SIGINT, SAMPLE_AUDIO_HandleSig); */
    /* signal(SIGTERM, SAMPLE_AUDIO_HandleSig); */

    EI_MI_MBASE_Init();
    EI_MI_VBUF_Init();
    EI_MI_MLINK_Init();

    if (enType == PT_UNINIT) {
        Sample_AUDIO_PlayWave(audiofilepath);
    }else {
        SAMPLE_AUDIO_AdecAo(audiofilepath, chn, rate);
    }

    EI_MI_MLINK_Exit();
    EI_MI_VBUF_Exit();
    EI_MI_MBASE_Exit();
EXIT:
    return s32Ret;
}


