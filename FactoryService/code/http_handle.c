#include "controller.h"
#include "mongoose.h"
#include "cJSON.h"
#include "http_handle.h"
#include "common_test.h"
#include "system_param.h"
#include "rild.h"
#include "encrypt.h"

#define ERR_PRINT           //printf

TestItemResult_t gTestResults[ITEM_MAX] = { 0 };

static cJSON * getTestItemResult(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    if (cjsonObjReturn != NULL)
    {
		int item = 0;
		char strInfo[128] = { 0 };
		
    	if (strstr(hm->uri.p, "name=tf"))
    	{
    		item = ITEM_TF;
    	}
		else if (strstr(hm->uri.p, "name=gps"))
		{
    		item = ITEM_GPS;
		}
		else if (strstr(hm->uri.p, "name=gsensor"))
		{
    		item = ITEM_GSENSOR;
		}
		else if (strstr(hm->uri.p, "name=rtc"))
		{
    		item = ITEM_RTC;
		}
		else if (strstr(hm->uri.p, "name=net4g"))
		{
    		item = ITEM_NET4G;
		}
		else if (strstr(hm->uri.p, "name=acc"))
		{
    		item = ITEM_ACC;
		}
		else if (strstr(hm->uri.p, "name=wifikey"))
		{
    		item = ITEM_WIFIKEY;
		}
		else if (strstr(hm->uri.p, "name=sos"))
		{
    		item = ITEM_SOS;
		}
		else if (strstr(hm->uri.p, "name=ver"))
		{
    		item = ITEM_VERSION;
		}
		else if (strstr(hm->uri.p, "name=imei"))
		{
    		item = ITEM_IMEI;
		}
		else if (strstr(hm->uri.p, "name=encrypt"))
		{
			item = ITEM_ENCRYPT;
		}
		else if (strstr(hm->uri.p, "name=pwradc"))
		{
			item = ITEM_PWRADC;
		}
		
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, gTestResults[item].result);

        cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, gTestResults[item].strInfo);
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * commandTestItem(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();
	
    if (cjsonObjReturn != NULL)
    {
		int result = 0;
		char strInfo[128] = { 0 };
		
    	if (strstr(hm->uri.p, "name=speaker"))
    	{
			system("amixer cset numid=7 84");
    		playAudio("starting.pcm");
			strcpy(strInfo, "已播放，请注意喇叭是否有声音");
    	}
		else if (strstr(hm->uri.p, "name=mic"))
		{
			MICTest_Start();
			strcpy(strInfo, "录音3秒，结束后自动播放");
		}
		else if (strstr(hm->uri.p, "name=getsn"))
		{
			result = 1;
			strcpy(strInfo, gGlobalSysParam->serialNum);
		}
		else if (strstr(hm->uri.p, "name=snapch1"))
		{
			result = CameraTest_Snap(0, strInfo);
		}
		else if (strstr(hm->uri.p, "name=snapch2"))
		{
			result = CameraTest_Snap(1, strInfo);
		}
		else if (strstr(hm->uri.p, "name=snapch3"))
		{
			result = CameraTest_Snap(2, strInfo);
		}
		else if (strstr(hm->uri.p, "name=shutdown"))
		{
			result = 1;
			extern int gTestShutdown;
			gTestShutdown = 1;
			strcpy(strInfo, "请断开ACC，设备自动关机");
		}
		
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, result);

        cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, strInfo);
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * writeSn(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    if (cjsonObjReturn != NULL)
    {
		int result = 0;
		char strInfo[128] = { 0 };

		char* sn = strchr(hm->uri.p, '=');

		if (NULL != sn)
		{
			char* end = strchr(hm->uri.p, ' ');
			end[0] = 0;
			result = 1;
			
			FILE* pfile=fopen("/data/sn", "wb");
			if (NULL != pfile)
			{
				strncpy(gGlobalSysParam->serialNum, sn+1, sizeof(gGlobalSysParam->serialNum));
				strncpy(gGlobalSysParam->wifiSSID, sn+1, sizeof(gGlobalSysParam->wifiSSID));
				fwrite(gGlobalSysParam->serialNum, 1, sizeof(gGlobalSysParam->serialNum), pfile);
				fclose(pfile);

				if (0 == gTestResults[ITEM_ENCRYPT].result)
				{
					if (0 == EncryptWrite())
					{
						gTestResults[ITEM_ENCRYPT].result = 1;
						gTestResults[ITEM_ENCRYPT].strInfo[0] = '\0';
					}
				}
				result = 1;
			}
			else
			{
				result = 0;
				sprintf(strInfo, "保存失败");
			}
		}
		else
		{
			sprintf(strInfo, "参数错误");
		}
		
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, result);

        cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, strInfo);
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * unsupportRequestAck(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 98);
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

void HttpGetRequestProcess(struct mg_connection *nc, struct http_message *hm)
{
    cJSON *cjsonObjReturn = NULL;

    int i=0, j = 0;
    char cmd[512] = { 0 };
    while (hm->uri.p[i] != ' ')
    {
        if (hm->uri.p[i] == '?')
        {
            j = i+1;
        }
        i ++;
    }
    memcpy(cmd, hm->uri.p+j, i-j);
    cmd[i-j+1] = 0;
    ERR_PRINT("RECV: %s\n", hm->uri.p);

    if (mg_vcmp(&hm->uri, REQUEST_GET_RESULT) == 0)
    {
        cjsonObjReturn = getTestItemResult(hm);
    }
	else if (mg_vcmp(&hm->uri, REQUEST_COMMAND) == 0)
	{
        cjsonObjReturn = commandTestItem(hm);
	}
	else if (mg_vcmp(&hm->uri, REQUEST_SET_SN) == 0)
	{
		cjsonObjReturn = writeSn(hm);
	}
	else
	{
		char* filename = cmd;
		if (0 == access(filename, R_OK))
		{
			mg_http_serve_file(nc, hm, filename, mg_mk_str("image/jpeg"), mg_mk_str(""));
			return;
		}
	}
	
    if (NULL == cjsonObjReturn)
    {
        cjsonObjReturn = unsupportRequestAck(hm);
    }

    mg_printf(nc, "%s", RETURN_HTTP_HEAD);
    mg_printf_http_chunk(nc, cJSON_Print(cjsonObjReturn));
    mg_send_http_chunk(nc, "", 0);

	ERR_PRINT("%s\n", cJSON_PrintUnformatted(cjsonObjReturn));

    cJSON_Delete(cjsonObjReturn);
}