//
// Created by fu on 3/2/18.
//

#ifndef SPEECH_C_DEMO_COMMON_H
#define SPEECH_C_DEMO_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"{
#endif


extern const int ENABLE_CURL_VERBOSE;

/**
 * @brief ��������ֵ����
 */
typedef enum {
    RETURN_OK = 0, // ��������
    RETURN_ERROR = 1, // ���ش���
    RETURN_FAIL = 2, // ����ʧ��

    ERROR_TOKEN_CURL = 13, // TOKEN CURL ���ô���
    ERROR_TOKEN_PARSE_ACCESS_TOKEN = 15,  // access_token�ֶ��ڷ��ؽ���в�����
    ERROR_TOKEN_PARSE_SCOPE = 16, // ����scope�ֶΣ�����scope������

    ERROR_ASR_FILE_NOT_EXIST = 101, // �����ļ�������
    ERROR_ASR_CURL = 102, // ʶ�� curl ����

    ERROR_TTS_CURL = 23
} RETURN_CODE;

/**
 * @brief ȫ�ֵı�����Ϣ��������������Ӧ��ֹͣ
 */
extern const int BUFFER_ERROR_SIZE;

extern char g_demo_error_msg[];

/**
 * @see libcurl CURLOPT_WRITEFUNCTION
 *
 * @brief curl�ص���http����Ľ����result�У�ע����Ҫ�ͷ�free(*result);
 * @param ptr
 * @param size
 * @param nmemb
 * @param result ����ʱ������NULL�� ʹ�ú������ͷ�
 * @return
 */
size_t writefunc(void *ptr, size_t size, size_t nmemb, char **result);


#ifdef __cplusplus
};
#endif




#endif //SPEECH_C_DEMO_COMMON_H
