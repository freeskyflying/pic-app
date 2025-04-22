#ifndef _MEDIA_AUDIO_H_
#define _MEDIA_AUDIO_H_

#include "ei_posix.h"
#include "sample_comm.h"
#include "pnt_ipc_stream.h"

#define MEDIA_AUDIO_RATE        8000
#define MEDIA_AUDIO_ENCTYPE     PT_AAC

typedef void(*audioFrameCallback)(int bindVenc, uint8_t* data, int len);

typedef struct
{

    int aenChn;
    int bindVenc;
    int bThreadStart;
    pthread_t aencPid;
    PNTIPC_StreamServer_t ipcStreamServer;

    pthread_mutex_t mutex;

    int recStatus;

    audioFrameCallback callback;

} MediaAudioStream_t;

void MediaAudio_Start(int bindVenc, audioFrameCallback callback);

int MediaAudio_DecAoStart(PAYLOAD_TYPE_E type, int chnNum, int rate);

int MediaAudio_DecAoStop(int chnNum);

void MediaAudio_DecAoSendFrame(char* frame, int frameLen);

#endif
