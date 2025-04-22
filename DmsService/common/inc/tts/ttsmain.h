//
// Created by fu on 3/2/18.
//

#ifndef SPEECH_C_DEMO_TTSMAIN_H
#define SPEECH_C_DEMO_TTSMAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include <pthread.h>

struct tts_config {
    char api_key[40]; // 填写网页上申请的appkey 如 $apiKey="g8eBUMSokVB1BHGmgxxxxxx"
    char secret_key[40]; // 填写网页上申请的APP SECRET 如 $secretKey="94dc99566550d87f8fa8ece112xxxxx"
    char text[512 * 3 + 1];  // 需要合成的文本  最多512个汉字
    int text_len; // 文本的长度
    char cuid[20];
    int spd;
    int pit;
    int vol;
    int per;
    int aue;
    char format[4];
    char downloadPath[128];
};

typedef int(*ttsCompleteCallback)(char* path);

typedef struct
{
    
    volatile char   mRunning;
    char            mInitFlag;
    char*           mText;
    pthread_mutex_t mMutex;
    ttsCompleteCallback mCb;

} TTSMain_t;

int TTSMain_Start(TTSMain_t* tts, char* text, ttsCompleteCallback cb);

#ifdef __cplusplus
};
#endif


#endif //SPEECH_C_DEMO_TTSMAIN_H





