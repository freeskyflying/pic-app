#include "mongoose.h"
#include "cJSON.h"
#include "6zhentan.h"
#include "utils.h"
#include "queue_list.h"
#include "media_utils.h"
#include "media_storage.h"
#include "media_video.h"
#include "system_param.h"

#define DBG_PRINT           printf
#define ERR_PRINT           printf

typedef struct
{
    
    char name[32];
    char value[32];

    void* next;

} paramItem;

extern BoardSysInfo_t gBoardInfo;

int gSimulateSpeed = 0;

int setParamItems(const char* str, paramItem** result)
{
    int i = 0, j = 0, start = 0;
    paramItem* head = NULL, *lastNode = NULL;
    char* tmp = NULL;

    char cmd[512] = { 0 };
    while (str[i] != ' ')
    {
        if (str[i] == '?')
        {
            j = i+1;
        }
        i ++;
    }
    memcpy(cmd, str+j, i-j);
    cmd[i-j+1] = 0;

    i = 0;
    j = 0;
    while (cmd[i])
    {
        if (cmd[i] == '&' || cmd[i+1] == 0)
        {
            if (cmd[i+1] == 0)
            {
                j+=1;
            }
            tmp = strchr(cmd+start, '=');
            if (NULL == tmp)
            {
                break;
            }
            paramItem* item = (paramItem*)malloc(sizeof(paramItem));
            memset(item, 0, sizeof(paramItem));

            memcpy(item->name, cmd+start, tmp - (cmd + start));
            memcpy(item->value, tmp+1, j-strlen(item->name)-1);

            if (head == NULL)
            {
                head = item;
            }
            else
            {
                lastNode->next = item;
            }
            lastNode = item;

            j = 0;
            start = i+1;
        }
        else
        {
            j++;
        }
        i++;
    }

    *result = head;

    return 0;
}

static cJSON * getProductInfoRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON *cjsonObjResultInReturn = cJSON_CreateObject();

        if (cjsonObjResultInReturn != NULL)
        {
            cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, cjsonObjResultInReturn);

            cJSON_AddStringToObject(cjsonObjResultInReturn, "model", gBoardInfo.mProductModel);
            cJSON_AddStringToObject(cjsonObjResultInReturn, "company", "PAINUOTE");
            cJSON_AddStringToObject(cjsonObjResultInReturn, "sp", "PAINUOTE");
            cJSON_AddStringToObject(cjsonObjResultInReturn, "soc", "eeasytech");

        }
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * getDeviceAttrRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON *cjsonObjResultInReturn = cJSON_CreateObject();

        if (cjsonObjResultInReturn != NULL)
        {
            cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, cjsonObjResultInReturn);

            cJSON_AddStringToObject(cjsonObjResultInReturn, "uuid", "PDC12310070001");
			
            cJSON_AddStringToObject(cjsonObjResultInReturn, "softver", gBoardInfo.mSwVersion);
            cJSON_AddStringToObject(cjsonObjResultInReturn, "otaver", "v1.20231007.1");
            cJSON_AddStringToObject(cjsonObjResultInReturn, "hwver", gBoardInfo.mHwVersion);
            cJSON_AddStringToObject(cjsonObjResultInReturn, "ssid", gGlobalSysParam->wifiSSID);
			char mac[32] = { 0 };
			getWifiMac("wlan0", mac);
            cJSON_AddStringToObject(cjsonObjResultInReturn, "bssid", mac);
            cJSON_AddNumberToObject(cjsonObjResultInReturn, "camnum", 1);
            cJSON_AddNumberToObject(cjsonObjResultInReturn, "curcamid", 0);
            cJSON_AddNumberToObject(cjsonObjResultInReturn, "wifireboot", 0);
        }
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * getMediaInfoRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON *cjsonObjResultInReturn = cJSON_CreateObject();

        if (cjsonObjResultInReturn != NULL)
        {
            cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, cjsonObjResultInReturn);

            cJSON_AddStringToObject(cjsonObjResultInReturn, "rtsp", "rtsp://192.168.169.1:554/live");
            cJSON_AddStringToObject(cjsonObjResultInReturn, "transport", "tcp");
            cJSON_AddNumberToObject(cjsonObjResultInReturn, "port", 554);
        }
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * getTFCardInfoRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON *cjsonObjResultInReturn = cJSON_CreateObject();

        if (cjsonObjResultInReturn != NULL)
        {
            cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, cjsonObjResultInReturn);

			int status = getFlagStatus("tfcard");
            SDCardInfo_t sdInfo = { 0 };
            storage_info_get("/mnt/card", &sdInfo);

            cJSON_AddNumberToObject(cjsonObjResultInReturn, "status", status);
            cJSON_AddNumberToObject(cjsonObjResultInReturn, "free", sdInfo.free/1024);
            cJSON_AddNumberToObject(cjsonObjResultInReturn, "total", sdInfo.total/1024);
        }
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * getCapabilityRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    paramItem* result = NULL, *tmp = NULL;
    setParamItems(hm->uri.p, &result);
    while (NULL != result)
    {
        DBG_PRINT("set param %s value %s\r\n", result->name, result->value);
        tmp = result->next;
        free(result);
        result = tmp;
    }

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON *cjsonObjResultInReturn = cJSON_CreateObject();

        if (cjsonObjResultInReturn != NULL)
        {
            cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, cjsonObjResultInReturn);
			
            cJSON_AddStringToObject(cjsonObjResultInReturn, "value", "100000000");
        }
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * getFileListRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

	char folder_type[10] = { 0 };
	int start = 0, end = 0;
    paramItem* result = NULL, *tmp = NULL;
    setParamItems(hm->uri.p, &result);
    while (NULL != result)
    {
        DBG_PRINT("set param %s value %s\r\n", result->name, result->value);
		if (strstr(result->name, "folder"))
		{
			strcpy(folder_type, result->value);
		}
		else if (strstr(result->name, "start"))
		{
			start = atoi(result->value);
		}
		else if (strstr(result->name, "end"))
		{
			end = atoi(result->value);
		}
        tmp = result->next;
        free(result);
        result = tmp;
    }

	char tempFileName[128] = { 0 };
	
    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON *array = cJSON_CreateArray();

        if (array != NULL)
        {		
            cJSON *paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "folder", folder_type);

			int total_count = 0;

			if (!strcmp(folder_type, "loop") || strlen(folder_type) == 0)
			{
				total_count = queue_list_get_count(&gVideoListQueue[0]) + queue_list_get_count(&gVideoListQueue[1]);

				if ((end-start+1) < total_count)
				{
					total_count = (0 == end)?total_count:(end-start+1);
				}

	            cJSON_AddNumberToObject(paramItem, "count", total_count);

				if (total_count > 0)
				{					
		            cJSON * arrayFiles= cJSON_CreateArray();
					
					int j = 0, count = 0;
					
					queue_t* head_ch1 = gVideoListQueue[0].m_head->prev;
					queue_t* head_ch2 = gVideoListQueue[1].m_head->prev;

					int flag = 0;
					while(count < total_count)
					{
						if (head_ch1 != NULL && !(flag&0x01))
						{
							time_t fileTime = (time_t)head_ch1->m_content;

							get_videoname_by_time(1, fileTime, tempFileName);
							
							struct stat buf = { 0 };
							memset(&buf, 0, sizeof(struct stat));
		    				stat(tempFileName, &buf);
							
							if (j >= start || (0 == end))
							{
								int duration = get_videofile_duration(tempFileName);
								
		            			cJSON * fileItem = cJSON_CreateObject();
			           	 		cJSON_AddStringToObject(fileItem, "name", tempFileName);
			           	 		cJSON_AddNumberToObject(fileItem, "duration", duration);
			           	 		cJSON_AddNumberToObject(fileItem, "size", buf.st_size/1024);
			           	 		cJSON_AddNumberToObject(fileItem, "createtime", fileTime+(gGlobalSysParam->timezone*3600));
			           	 		cJSON_AddNumberToObject(fileItem, "type", 2);
								
			            		cJSON_AddItemToArray(arrayFiles, fileItem);
							
								count ++;
							}
							j += 1;
							
							head_ch1 = head_ch1->prev;
						}
						
						if (head_ch2 != NULL && !(flag&0x02))
						{
							time_t fileTime = (time_t)head_ch2->m_content;
							
							get_videoname_by_time(2, fileTime, tempFileName);
							
							struct stat buf = { 0 };
							memset(&buf, 0, sizeof(struct stat));
		    				stat(tempFileName, &buf);
							
							if (j >= start || (0 == end))
							{
								int duration = get_videofile_duration(tempFileName);
								
		            			cJSON * fileItem = cJSON_CreateObject();
			           	 		cJSON_AddStringToObject(fileItem, "name", tempFileName);
			           	 		cJSON_AddNumberToObject(fileItem, "duration", duration);
			           	 		cJSON_AddNumberToObject(fileItem, "size", buf.st_size/1024);
			           	 		cJSON_AddNumberToObject(fileItem, "createtime", fileTime+(gGlobalSysParam->timezone*3600));
			           	 		cJSON_AddNumberToObject(fileItem, "type", 2);
								
			            		cJSON_AddItemToArray(arrayFiles, fileItem);
							
								count ++;
							}
							j += 1;
							
							head_ch2 = head_ch2->prev;
						}
						if (3 == flag)
						{
							break;
						}
						if (head_ch1 == gVideoListQueue[0].m_head)
						{
							flag |= 1;
						}
						if (gVideoListQueue[1].m_head == head_ch2)
						{
							flag |= 2;
						}
					}
					
		            cJSON_AddItemToObject(paramItem, "files", arrayFiles);
					
		        	cJSON_AddItemToArray(array, paramItem);
				}
			}
			else
			{
	            cJSON_AddNumberToObject(paramItem, "count", 0);
			}
            /*--------------------------------------------------------*/
        }
		
        cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, array);
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * getAdasParamItemsRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    paramItem* result = NULL, *tmp = NULL;
    setParamItems(hm->uri.p, &result);
    while (NULL != result)
    {
        DBG_PRINT("set param %s value %s\r\n", result->name, result->value);
        tmp = result->next;
        free(result);
        result = tmp;
    }

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON *array = cJSON_CreateArray();

        if (array != NULL)
        {
            cJSON *paramItem = cJSON_CreateObject();
			
            cJSON_AddStringToObject(paramItem, "name", "osw_speed");
            cJSON_AddStringToObject(paramItem, "range", "0,300");
            cJSON_AddNumberToObject(paramItem, "step", 10);
            cJSON_AddStringToObject(paramItem, "unit", "km/h");
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "car_width");
            cJSON_AddStringToObject(paramItem, "range", "0,300");
            cJSON_AddNumberToObject(paramItem, "step", 1);
            cJSON_AddStringToObject(paramItem, "unit", "cm");
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "camera_to_ground");
            cJSON_AddStringToObject(paramItem, "range", "0,300");
            cJSON_AddNumberToObject(paramItem, "step", 1);
            cJSON_AddStringToObject(paramItem, "unit", "cm");
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "camera_to_car_head");
            cJSON_AddStringToObject(paramItem, "range", "0,300");
            cJSON_AddNumberToObject(paramItem, "step", 1);
            cJSON_AddStringToObject(paramItem, "unit", "cm");
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "camera_to_middle");
            cJSON_AddStringToObject(paramItem, "range", "0,300");
            cJSON_AddNumberToObject(paramItem, "step", 1);
            cJSON_AddStringToObject(paramItem, "unit", "cm");
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "ldw_en");
            cJSON *arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("low"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("mid"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("high"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            cJSON *arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(2));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(3));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "hmw_en");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("low"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("mid"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("high"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(2));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(3));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "fcw_en");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("low"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("mid"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("high"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(2));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(3));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "pcw_en");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("low"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("mid"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("high"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(2));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(3));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "fmw_en");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("low"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("mid"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("high"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(2));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(3));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "tdw_sensitivity");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("low"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("mid"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("high"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(2));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(3));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "dw_sensitivity");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("low"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("mid"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("high"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(2));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(3));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "sw_sensitivity");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("low"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("mid"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("high"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(2));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(3));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "pw_sensitivity");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("low"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("mid"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("high"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(2));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(3));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "fcw_speed");
            cJSON_AddStringToObject(paramItem, "range", "0,100");
            cJSON_AddNumberToObject(paramItem, "step", 1);
            cJSON_AddStringToObject(paramItem, "unit", "KM/H");
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "pcw_speed");
            cJSON_AddStringToObject(paramItem, "range", "0,100");
            cJSON_AddNumberToObject(paramItem, "step", 1);
            cJSON_AddStringToObject(paramItem, "unit", "KM/H");
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "hmw_speed");
            cJSON_AddStringToObject(paramItem, "range", "0,100");
            cJSON_AddNumberToObject(paramItem, "step", 1);
            cJSON_AddStringToObject(paramItem, "unit", "KM/H");
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "ldw_speed");
            cJSON_AddStringToObject(paramItem, "range", "0,300");
            cJSON_AddNumberToObject(paramItem, "step", 10);
            cJSON_AddStringToObject(paramItem, "unit", "km/h");
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "tdw_speed");
            cJSON_AddStringToObject(paramItem, "range", "0,300");
            cJSON_AddNumberToObject(paramItem, "step", 10);
            cJSON_AddStringToObject(paramItem, "unit", "km/h");
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "dw_speed");
            cJSON_AddStringToObject(paramItem, "range", "0,300");
            cJSON_AddNumberToObject(paramItem, "step", 10);
            cJSON_AddStringToObject(paramItem, "unit", "km/h");
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "sw_speed");
            cJSON_AddStringToObject(paramItem, "range", "0,300");
            cJSON_AddNumberToObject(paramItem, "step", 10);
            cJSON_AddStringToObject(paramItem, "unit", "km/h");
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "pw_speed");
            cJSON_AddStringToObject(paramItem, "range", "0,300");
            cJSON_AddNumberToObject(paramItem, "step", 10);
            cJSON_AddStringToObject(paramItem, "unit", "km/h");
            cJSON_AddItemToArray(array, paramItem);
        }

        cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, array);
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * getAdasParamValuesRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    paramItem* result = NULL, *tmp = NULL;
    setParamItems(hm->uri.p, &result);
    while (NULL != result)
    {
        DBG_PRINT("set param %s value %s\r\n", result->name, result->value);
        tmp = result->next;
        free(result);
        result = tmp;
    }

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON *array = cJSON_CreateArray();

        if (array != NULL)
        {
            cJSON *paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "osw_speed");
            cJSON_AddNumberToObject(paramItem, "value", 110);
            cJSON_AddItemToArray(array, paramItem);
				
				paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "car_width");
            cJSON_AddNumberToObject(paramItem, "value", 185);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "camera_to_ground");
            cJSON_AddNumberToObject(paramItem, "value", 80);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "camera_to_car_head");
            cJSON_AddNumberToObject(paramItem, "value", 80);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "camera_to_middle");
            cJSON_AddNumberToObject(paramItem, "value", 50);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "ldw_en");
            cJSON_AddNumberToObject(paramItem, "value", 1);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "hmw_en");
            cJSON_AddNumberToObject(paramItem, "value", 1);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "fcw_en");
            cJSON_AddNumberToObject(paramItem, "value", 1);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "pcw_en");
            cJSON_AddNumberToObject(paramItem, "value", 1);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "fmw_en");
            cJSON_AddNumberToObject(paramItem, "value", 1);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "tdw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", 1);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "dw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", 1);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "sw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", 1);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "pw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", 1);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "fcw_speed");
            cJSON_AddNumberToObject(paramItem, "value", 30);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "pcw_speed");
            cJSON_AddNumberToObject(paramItem, "value", 30);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "pmw_speed");
            cJSON_AddNumberToObject(paramItem, "value", 30);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "ldw_speed");
            cJSON_AddNumberToObject(paramItem, "value", 30);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "tdw_speed");
            cJSON_AddNumberToObject(paramItem, "value", 30);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "dw_speed");
            cJSON_AddNumberToObject(paramItem, "value", 30);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "sw_speed");
            cJSON_AddNumberToObject(paramItem, "value", 30);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "pw_speed");
            cJSON_AddNumberToObject(paramItem, "value", 30);
            cJSON_AddItemToArray(array, paramItem);
        }
		
        cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, array);
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * setAdasParamValuesRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    paramItem* result = NULL, *tmp = NULL;
    setParamItems(hm->uri.p, &result);
	
	if (NULL != result)
	{
		tmp = result->next;
        DBG_PRINT("set param %s value %s\r\n", tmp->name, tmp->value);
	}
	
    while (NULL != result)
    {
        tmp = result->next;
        free(result);
        result = tmp;
    }

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * setSysTimeRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    paramItem* result = NULL, *tmp = NULL;
    setParamItems(hm->uri.p, &result);
    while (NULL != result)
    {
        DBG_PRINT("set param %s value %s\r\n", result->name, result->value);
		if (!strcmp(result->name, "date"))
		{
			int year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0;
			sscanf(result->value, "%04d%02d%02d%02d%02d%02d", &year, &mon, &day, &hour, &min, &sec);
			char cmd[128] = { 0 };
			sprintf(cmd, "date -s \"%04d-%02d-%02d %02d:%02d:%02d\"", year, mon, day, hour, min, sec);
			system(cmd);
			system("hwclock -w");
		}
        tmp = result->next;
        free(result);
        result = tmp;
    }

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, "set success");
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * setTimeZoneRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    paramItem* result = NULL, *tmp = NULL;
    setParamItems(hm->uri.p, &result);
    while (NULL != result)
    {
        DBG_PRINT("set param %s value %s\r\n", result->name, result->value);
        tmp = result->next;
        free(result);
        result = tmp;
    }

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, "set success");
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * getParamValueRequest(struct http_message *hm)
{
    int type = 0;

    paramItem* result = NULL, *tmp = NULL;
    setParamItems(hm->uri.p, &result);
    if (NULL != result)
    {
        if (!strcmp(result->value, "all"))
        {
            type = 0;
        }
        else
        {
            if (!strcmp(result->value, "mic"))
            {
                type = 1;
            }
            else if (!strcmp(result->value, "osd"))
            {
                type = 2;
            }
            else if (!strcmp(result->value, "speaker"))
            {
                type = 3;
            }
            else if (!strcmp(result->value, "rec"))
            {
                type = 4;
            }
            else
            {
                type = -1;
            }
        }
        while (result)
        {
            tmp = result->next;
            free(result);
            result = tmp;
        }
    }

    if (type == -1)
    {
        return NULL;
    }
    
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

		if (0 == type)
		{
	        cJSON *array = cJSON_CreateArray();

	        if (array != NULL)
	        {			
	            cJSON *paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "mic");
		        cJSON_AddNumberToObject(paramItem, "value", gGlobalSysParam->micEnable);
		        cJSON_AddItemToArray(array, paramItem);
			
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "osd");
		        cJSON_AddNumberToObject(paramItem, "value", gGlobalSysParam->osdEnable);
		        cJSON_AddItemToArray(array, paramItem);
			
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "speaker");
		        cJSON_AddNumberToObject(paramItem, "value", gGlobalSysParam->mVolume);
		        cJSON_AddItemToArray(array, paramItem);
			
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "rec");
		        cJSON_AddNumberToObject(paramItem, "value", 1);
		        cJSON_AddItemToArray(array, paramItem);

				AppPlatformParams_t param = { 0 };
				getAppPlatformParams(&param, 0);
				
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "ip_address");
		        cJSON_AddStringToObject(paramItem, "value", param.ip);
		        cJSON_AddItemToArray(array, paramItem);
			
	            paramItem = cJSON_CreateObject();
				char portStr[10] = { 0 };
				sprintf(portStr, "%d", param.port);
	            cJSON_AddStringToObject(paramItem, "name", "ip_port");
		        cJSON_AddStringToObject(paramItem, "value", portStr);
		        cJSON_AddItemToArray(array, paramItem);
			
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "phone");
		        cJSON_AddStringToObject(paramItem, "value", param.phoneNum);
		        cJSON_AddItemToArray(array, paramItem);
			
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "plate_color");
		        cJSON_AddNumberToObject(paramItem, "value", param.plateColor);
		        cJSON_AddItemToArray(array, paramItem);
			
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "plate_number");
		        cJSON_AddStringToObject(paramItem, "value", param.plateNum);
		        cJSON_AddItemToArray(array, paramItem);
				
				paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "platform_protocol");
	            cJSON_AddNumberToObject(paramItem, "value", 0);
	            cJSON_AddItemToArray(array, paramItem);
		
	            paramItem = cJSON_CreateObject();
				char intStr[10] = { 0 };
				sprintf(intStr, "%d", gSimulateSpeed);
	            cJSON_AddStringToObject(paramItem, "name", "sim_speed");
		        cJSON_AddStringToObject(paramItem, "value", intStr);
		        cJSON_AddItemToArray(array, paramItem);
	        }
			
	        cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, array);
		}
		else
		{
	        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

	        cJSON *cjsonObjResultInReturn = cJSON_CreateObject();

	        if (cjsonObjResultInReturn != NULL)
	        {
	            cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, cjsonObjResultInReturn);
				
				if (1 == type)
				{
		            cJSON_AddStringToObject(cjsonObjResultInReturn, "name", "mic");
			        cJSON_AddNumberToObject(cjsonObjResultInReturn, "value", gGlobalSysParam->adasEnable);
				}
				else if (2 == type)
				{
		            cJSON_AddStringToObject(cjsonObjResultInReturn, "name", "osd");
			        cJSON_AddNumberToObject(cjsonObjResultInReturn, "value", gGlobalSysParam->dsmEnable);
				}
				else if (3 == type)
				{
		            cJSON_AddStringToObject(cjsonObjResultInReturn, "name", "speaker");
			        cJSON_AddNumberToObject(cjsonObjResultInReturn, "value", gGlobalSysParam->mVolume);
				}
				else if (4 == type)
				{
		            cJSON_AddStringToObject(cjsonObjResultInReturn, "name", "rec");
			        cJSON_AddNumberToObject(cjsonObjResultInReturn, "value", 1);
				}
	        }
		}
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * getParamItemsRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    paramItem* result = NULL, *tmp = NULL;
    setParamItems(hm->uri.p, &result);
    while (NULL != result)
    {
        DBG_PRINT("set param %s value %s\r\n", result->name, result->value);
        tmp = result->next;
        free(result);
        result = tmp;
    }

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON *array = cJSON_CreateArray();

        if (array != NULL)
        {
            cJSON *paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "mic");
            cJSON * arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("on"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            cJSON * arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "osd");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("on"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "speaker");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("low"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("middle"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("high"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(2));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(3));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "rec");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("on"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "platform_protocol");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("808-2013"));
            //cJSON_AddItemToArray(arrayItems, cJSON_CreateString("905"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            //cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "ip_address");
            cJSON_AddItemToArray(array, paramItem);
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "ip_port");
            cJSON_AddItemToArray(array, paramItem);
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "phone");
            cJSON_AddItemToArray(array, paramItem);
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "plate_color");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("None"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("Blue"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("Yellow"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("Black"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("White"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("Green"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("Other"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(2));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(3));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(4));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(5));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(6));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "plate_number");
            cJSON_AddItemToArray(array, paramItem);
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "sim_speed");
            cJSON_AddStringToObject(paramItem, "range", "0,300");
            cJSON_AddNumberToObject(paramItem, "step", 10);
            cJSON_AddStringToObject(paramItem, "unit", "km/h");
            cJSON_AddItemToArray(array, paramItem);
        }
		
        cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, array);
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * setParamValueRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    paramItem* result = NULL, *tmp = NULL;
    setParamItems(hm->uri.p, &result);
    if (NULL != result)
    {
        tmp = result->next;
        DBG_PRINT("set %s=%s %s=%s\r\n", result->name, result->value, tmp->name, tmp->value);
        if (!strcmp(result->value, "switchcam"))
        {
            extern int gCurrentRtspChannel;
            DBG_PRINT("set %s value %s\r\n", tmp->name, tmp->value);
            gCurrentRtspChannel = atoi(tmp->value);
        }
		else if (!strcmp(result->value, "speaker"))
		{
			gGlobalSysParam->mVolume = atoi(tmp->value);
			setSpeakerVolume(gGlobalSysParam->mVolume);
		}
		else if (!strcmp(result->value, "mic"))
		{
			gGlobalSysParam->micEnable = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "osd"))
		{
			gGlobalSysParam->osdEnable = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "ip_address"))
        {
			setAppPlatformParamItems(0, tmp->value, 0);
        }
		else if (!strcmp(result->value, "ip_port"))
		{
			setAppPlatformParamItems(1, tmp->value, 0);
		}
		else if (!strcmp(result->value, "plate_number"))
		{
			setAppPlatformParamItems(2, tmp->value, 0);
		}
		else if (!strcmp(result->value, "phone"))
		{
			setAppPlatformParamItems(3, tmp->value, 0);
		}
		else if (!strcmp(result->value, "plate_color"))
		{
			setAppPlatformParamItems(4, tmp->value, 0);
		}
		else if (!strcmp(result->value, "platform_protocol"))
		{
			setAppPlatformParamItems(5, tmp->value, 0);
		}
		else if (!strcmp(result->value, "sim_speed"))
		{
			gSimulateSpeed = atoi(tmp->value);
		}
		
		free(tmp);	tmp = NULL;
        free(result);	result = NULL;
    }

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, "set success");
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * setWifiParamRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    paramItem* result = NULL, *tmp = NULL;
    setParamItems(hm->uri.p, &result);
    if (NULL != result)
    {
        DBG_PRINT("set %s %s\r\n", result->name, result->value);
        if (!strcmp(result->name, "wifissid"))
        {
        	strncpy(gGlobalSysParam->wifiSSID, result->value, sizeof(gGlobalSysParam->wifiSSID)-1);
        }
        else if (!strcmp(result->name, "wifipwd"))
        {
        	strncpy(gGlobalSysParam->wifiPswd, result->value, sizeof(gGlobalSysParam->wifiPswd)-1);
        }
		free(result);
		result = NULL;
    }

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, "set success");
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * setWifiRebootRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

	extern void WifiReboot(void);
	WifiReboot();

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, "set success");
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * setSDFormatRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

		if (formatAndMountTFCard("/mnt/card", "/dev/mmcblk0p1"))
		{
        	cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, "format failed");
		}
		else
		{
        	cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, "set success");
		}
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
    DBG_PRINT("RECV: %s\n", cmd);

    if (mg_vcmp(&hm->uri, REQUEST_GET_PRODUCTINFO) == 0)
    {
        DBG_PRINT("REQUEST_GET_PRODUCTINFO\r\n");
        cjsonObjReturn = getProductInfoRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_GET_DEVICEATTR) == 0)
    {
        DBG_PRINT("REQUEST_GET_DEVICEATTR\r\n");
        cjsonObjReturn = getDeviceAttrRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_GET_MEDIAINFO) == 0)
    {
        DBG_PRINT("REQUEST_GET_MEDIAINFO\r\n");
        cjsonObjReturn = getMediaInfoRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_GET_TFCARDINFO) == 0)
    {
        DBG_PRINT("REQUEST_GET_TFCARDINFO\r\n");
        cjsonObjReturn = getTFCardInfoRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_GET_BATTERYINFO) == 0)
    {
        DBG_PRINT("REQUEST_GET_BATTERYINFO\r\n");
    }
    else if (mg_vcmp(&hm->uri, REQUEST_SET_SYSTIME) == 0)
    {
        DBG_PRINT("REQUEST_SET_SYSTIME\r\n");
        cjsonObjReturn = setSysTimeRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_SET_TIMEZONE) == 0)
    {
        DBG_PRINT("REQUEST_SET_TIMEZONE\r\n");
        cjsonObjReturn = setTimeZoneRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_GET_RECDURATION) == 0)
    {
        DBG_PRINT("REQUEST_GET_RECDURATION\r\n");
    }
    else if (mg_vcmp(&hm->uri, REQUEST_SET_SETTING) == 0)
    {
        DBG_PRINT("REQUEST_SET_SETTING\r\n");
    }
    else if (mg_vcmp(&hm->uri, REQUEST_SET_PLAYBACK) == 0)
    {
        DBG_PRINT("REQUEST_SET_PLAYBACK\r\n");
    }
    else if (mg_vcmp(&hm->uri, REQUEST_SET_WIFI) == 0)
    {
        DBG_PRINT("REQUEST_SET_WIFI\r\n");
		cjsonObjReturn = setWifiParamRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_SD_FORMAT) == 0)
    {
        DBG_PRINT("REQUEST_SD_FORMAT\r\n");
		cjsonObjReturn = setSDFormatRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_PARAMS_RESET) == 0)
    {
        DBG_PRINT("REQUEST_PARAMS_RESET\r\n");
    }
    else if (mg_vcmp(&hm->uri, REQUEST_GET_CAMERAINFO) == 0)
    {
        DBG_PRINT("REQUEST_GET_CAMERAINFO\r\n");
    }
    else if (mg_vcmp(&hm->uri, REQUEST_SET_PARAMVALUE) == 0)
    {
        DBG_PRINT("REQUEST_SET_PARAMVALUE\r\n");
        cjsonObjReturn = setParamValueRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_SNAPSHOT) == 0)
    {
        DBG_PRINT("REQUEST_SNAPSHOT\r\n");
    }
    else if (mg_vcmp(&hm->uri, REQUEST_LOCK_VIDEO) == 0)
    {
        DBG_PRINT("REQUEST_LOCK_VIDEO\r\n");
    }
    else if (mg_vcmp(&hm->uri, REQUEST_WIFI_REBOOT) == 0)
    {
        DBG_PRINT("REQUEST_WIFI_REBOOT\r\n");
        cjsonObjReturn = setWifiRebootRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_GET_CAPABILITY) == 0)
    {
        DBG_PRINT("REQUEST_GET_CAPABILITY\r\n");
        cjsonObjReturn = getCapabilityRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_GET_FILELIST) == 0)
    {
        DBG_PRINT("REQUEST_GET_FILELIST\r\n");
        cjsonObjReturn = getFileListRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_GET_THUMBNAIL) == 0)
    {
        DBG_PRINT("REQUEST_GET_THUMBNAIL\r\n");
		char filename[128] = { 0 };
		get_thumbname_by_videoname(filename, &cmd[5]);
		if (0 == access(filename, R_OK))
		{
			mg_http_serve_file(nc, hm, filename, mg_mk_str("image/jpeg"), mg_mk_str(""));
			return;
		}
    }
    else if (mg_vcmp(&hm->uri, REQUEST_DELETE_FILE) == 0)
    {
        DBG_PRINT("REQUEST_DELETE_FILE\r\n");
    }
    else if (mg_vcmp(&hm->uri, REQUEST_QUERY_PARAMITEMS) == 0)
    {
        DBG_PRINT("REQUEST_QUERY_PARAMITEMS\r\n");
        cjsonObjReturn = getParamItemsRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_GET_PARAMVALUES) == 0)
    {
        DBG_PRINT("REQUEST_GET_PARAMVALUES\r\n");
		cjsonObjReturn = getParamValueRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_GET_ADAS_PARAMITEMS) == 0)
    {
        DBG_PRINT("REQUEST_GET_ADAS_PARAMITEMS\r\n");
        cjsonObjReturn = getAdasParamItemsRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_GET_ADAS_PARAMVALUES) == 0)
    {
        DBG_PRINT("REQUEST_GET_ADAS_PARAMVALUES\r\n");
        cjsonObjReturn = getAdasParamValuesRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, REQUEST_SET_ADAS_PARAMVALUES) == 0)
    {
        DBG_PRINT("REQUEST_SET_ADAS_PARAMVALUES\r\n");
        cjsonObjReturn = setAdasParamValuesRequest(hm);
    }
    else if (strncmp(hm->uri.p, SDCARD_PATH, strlen(SDCARD_PATH)) == 0 || strncmp(hm->uri.p, "/data", 5) == 0)
    {
        DBG_PRINT("request download file\r\n");
		if (strstr(hm->uri.p, VIDEO_PATH"/CH"))
		{
			mg_http_serve_file(nc, hm, cmd, mg_mk_str("video/mp4"), mg_mk_str(""));
		}
		else
		{
			mg_http_serve_file(nc, hm, cmd, mg_mk_str("application/octet-stream"), mg_mk_str(""));
		}
		return;
    }

    if (NULL == cjsonObjReturn)
    {
        cjsonObjReturn = unsupportRequestAck(hm);
    }

    mg_printf(nc, "%s", RETURN_HTTP_HEAD);
    mg_printf_http_chunk(nc, cJSON_Print(cjsonObjReturn));
    mg_send_http_chunk(nc, "", 0);

	DBG_PRINT("%s\n", cJSON_PrintUnformatted(cjsonObjReturn));

    cJSON_Delete(cjsonObjReturn);
}