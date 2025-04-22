//
// Created by fu on 3/5/18.
//

#ifndef SPEECH_C_DEMO_TTSHTTP_H
#define SPEECH_C_DEMO_TTSHTTP_H

#ifdef __cplusplus
extern "C"{
#endif

#include "common.h"
#include "stdlib.h"

struct http_result {
    int has_error;
    char *format; // ��Ƶ�ĸ�ʽ��aue��������
    FILE *fp; // ���������ļ�
    char* path;
};



size_t writefunc_data(void *ptr, size_t size, size_t nmemb, struct http_result *result);

size_t header_callback(char *buffer, size_t size, size_t nitems, struct http_result *result);



#ifdef __cplusplus
};
#endif



#endif //SPEECH_C_DEMO_TTSHTTP_H
