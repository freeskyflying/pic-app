//
// Created by fu on 3/2/18.
//

#include <curl/curl.h>
#include <memory.h>
#include <pthread.h>
#include "common.h"
#include "ttsmain.h"
#include "token.h"
#include "ttscurl.h"
#include "pnt_log.h"

#define LOG_TTS_DBG		printf
#define LOG_TTS_ERR		printf//PNT_LOGE

const char TTS_SCOPE[] = "audio_tts_post";
const char API_TTS_URL[] = "http://tsn.baidu.com/text2audio"; // 可改为https

const char* utf8_to_url(const char* input)
{
    // 使用curl_easy_escape进行百分比编码
    char* output = curl_easy_escape(NULL, input, 0);
    if (output == NULL)
	{
        LOG_TTS_ERR("Failed to convert to URL encoding\n");
		return input;
    }
    return output;
}


RETURN_CODE fill_config(char* argv,struct tts_config *config)
{
    // 填写网页上申请的appkey 如 g_api_key="g8eBUMSokVB1BHGmgxxxxxx"
    char api_key[] = "P1NDP3FXFFx7ZPxitjKrOP2I";
    // 填写网页上申请的APP SECRET 如 $secretKey="94dc99566550d87f8fa8ece112xxxxx"
    char secret_key[] = "9bC9ACmUKxDuhLU0XXp3wjPpjMud0nnh";

    // text 的内容为"欢迎使用百度语音合成"的urlencode,utf-8 编码
    // 可以百度搜索"urlencode"
    //char text[] = "10秒后系统升级开始，如不升级，请拔出存储器";
//，SD2故障，硬盘故障，U盘故障
    // 发音人选择, 0为普通女声，1为普通男生，3为情感合成-度逍遥，4为情感合成-度丫丫，默认为普通女声
    int per = 0;
    // 语速，取值0-9，默认为5中语速
    int spd = 5;
    // #音调，取值0-9，默认为5中语调
    int pit = 5;


    // #音量，取值0-9，默认为5中音量
    int vol = 5;
    // 下载的文件格式, 3：mp3(default) 4： pcm-16k 5： pcm-8k 6. wav
	int aue = 5;
	
    // 将上述参数填入config中
    snprintf(config->api_key, sizeof(config->api_key), "%s", api_key);
    snprintf(config->secret_key, sizeof(config->secret_key), "%s", secret_key);
    snprintf(config->text, strlen(argv)+1, "%s", argv);
    config->text_len = strlen(argv);
    snprintf(config->cuid, sizeof(config->cuid), "1234567C");
    config->per = per;
    config->spd = spd;
    config->pit = pit;
    config->vol = vol;
	config->aue = aue;
	sprintf(config->downloadPath, "%s", "/cache/result.pcm");
	// aue对应的格式，format
	const char formats[4][4] = {"mp3", "pcm", "pcm", "wav"};
	snprintf(config->format, sizeof(config->format), formats[aue - 3]);
	
    return RETURN_OK;
}

// 调用识别接口
RETURN_CODE run_tts(TTSMain_t* tts, struct tts_config *config, const char *token)
{
    char params[2048];
    CURL *curl = curl_easy_init(); // 需要释放
    char *cuid = curl_easy_escape(curl, config->cuid, strlen(config->cuid)); // 需要释放
    char *textemp = curl_easy_escape(curl, config->text, config->text_len); // 需要释放
	char *tex = curl_easy_escape(curl, textemp, strlen(textemp)); // 需要释放
	curl_free(textemp);
	char params_pattern[] = "ctp=1&lan=zh&cuid=%s&tok=%s&tex=%s&per=%d&spd=%d&pit=%d&vol=%d&aue=%d";
    snprintf(params, sizeof(params), params_pattern , cuid, token, tex,
             config->per, config->spd, config->pit, config->vol, config->aue);
			 
	char url[1024];
	snprintf(url, sizeof(url), "%s?%s", API_TTS_URL, params);
    LOG_TTS_ERR("download the recognized pcm file from --> :%s\n", url);
    curl_free(cuid);
  	curl_free(tex);
	
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params);
    curl_easy_setopt(curl, CURLOPT_URL, API_TTS_URL);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30); // 连接5s超时
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60); // 整体请求60s超时
    struct http_result result = {1, config->format ,NULL, config->downloadPath};
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback); // 检查头部
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &result);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);  // 需要释放
    curl_easy_setopt(curl, CURLOPT_VERBOSE, ENABLE_CURL_VERBOSE);
    CURLcode res_curl = curl_easy_perform(curl);

    RETURN_CODE res = RETURN_OK;
    if (res_curl != CURLE_OK)
    {
        LOG_TTS_ERR("unexpected exception happened!!! fail to download %s from %s, with ret:%d\n", config->downloadPath, url,
                res);
        // curl 失败
        snprintf(g_demo_error_msg, BUFFER_ERROR_SIZE, "perform curl error:%d, %s.\n", res,
                 curl_easy_strerror(res_curl));
        res = ERROR_TTS_CURL;
    }
    else
    {
        LOG_TTS_ERR(" --> download %s from %s success!!!\n", config->downloadPath, url);
    }
	if (result.fp != NULL)
    {
		fclose(result.fp);
	}
    curl_easy_cleanup(curl);
	if(tts->mCb != NULL)
    {
        LOG_TTS_ERR("notity download:%s success\n", config->downloadPath);
        tts->mCb(config->downloadPath);
	}
    return res;
}

void* ttsThreadLoop(void* arg)
{
    pthread_detach(pthread_self());

    TTSMain_t* tts = (TTSMain_t*)arg;

    pthread_mutex_lock(&tts->mMutex);

    LOG_TTS_ERR("--> the tts content are --> %s\n", tts->mText);

    curl_global_init(CURL_GLOBAL_ALL);
    struct tts_config config = {0};
    char token[MAX_TOKEN_SIZE];

    RETURN_CODE res = fill_config(tts->mText, &config);
    LOG_TTS_ERR("the tts remote server config result are --> %d\n", res);
    if (res == RETURN_OK) {
        // 获取token
        LOG_TTS_ERR("start to request the token --> \n");
        res = speech_get_token(config.api_key, config.secret_key, TTS_SCOPE, token);
        LOG_TTS_ERR("the result of get speech token are --> %d\n", res);
        if (res == RETURN_OK) {
            // 调用识别接口
            LOG_TTS_ERR("start to perform recognize with token --> %s\n", token);
            run_tts(tts, &config, token);
        } else {
            LOG_TTS_ERR("fatal exception!!! fail to get the speech recognize token with ret:%d!!!\n", res);
        }
    } else {
        LOG_TTS_ERR("fatal state exception!!! fail to perform the config with ret:%d\n", res);
    }
    curl_global_cleanup();
    free(tts->mText);
    tts->mText = NULL;
    tts->mRunning = 0;
	
    pthread_mutex_unlock(&tts->mMutex);
	LOG_TTS_ERR("%s", g_demo_error_msg);

    pthread_exit(NULL);
    return NULL;
}

int TTSMain_Start(TTSMain_t* tts, char* text, ttsCompleteCallback cb)
{
    if (tts->mRunning || NULL == text)
    {
        return RETURN_OK;
    }
    if (1 != tts->mInitFlag)
    {
        pthread_mutex_init(&tts->mMutex, NULL);
        tts->mInitFlag = 1;
    }
    
    pthread_mutex_lock(&tts->mMutex);

    tts->mCb = cb;
    tts->mText = (char*)malloc(4096);
    if(tts->mText != NULL){
        memset(tts->mText, 0, 4096);
        memcpy(tts->mText, text, strlen(text));		
        pthread_t tid;
        pthread_create(&tid, NULL, ttsThreadLoop, tts);
    }
    tts->mRunning = 1;

    pthread_mutex_unlock(&tts->mMutex);
    
    return RETURN_OK;      
}

