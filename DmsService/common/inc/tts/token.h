//
// Created by fu on 3/2/18.
//

#ifndef SPEECH_C_DEMO_TOKEN_H
#define SPEECH_C_DEMO_TOKEN_H

#include "common.h"

#ifdef __cplusplus
extern "C"{
#endif

extern const char API_TOKEN_URL[];

extern const int MAX_TOKEN_SIZE;

/**
 * @brief ��ȡtoken ,���� parse_token��������� ע�������expires_in�� ��ʾ��sampleδ������
 * @param api_key in �û���ҳ�������Ӧ�û�ȡappKey
 * @param secret_key in �û���ҳ�������Ӧ�û�ȡappi_key
 * @param scope in Ӧ�ÿ�ͨ��Ȩ��
 * @param token out ��ȡ��token
 * @return RETURN_OK �ɹ� ����ʧ��
 */
RETURN_CODE speech_get_token(const char *api_key,
                             const char *secret_key, const char *scope, char *token);

/**
 * @brief ����response json �� ��ȡtoken����֤scope��Ϣ
 * @param response in �ӿڷ��ص�json
 * @param scope in scope ����ʶ���������������ϳ�����
 * @param token out ��������token
 * @return RETURN_OK �ɹ� ����ʧ��
 */
RETURN_CODE parse_token(const char *response, const char *scope, char *token);

/**
 * @brief ����json����ȡĳ��key��Ӧ��value
 * @param json in ԭʼjson�ַ���
 * @param key in json key
 * @param value out ����Ľ��
 * @param value_size in value�����size
 * @return
 */
RETURN_CODE obtain_json_str(const char *json, const char *key, char *value, int value_size);



#ifdef __cplusplus
};
#endif



#endif //SPEECH_C_DEMO_TOKEN_H
