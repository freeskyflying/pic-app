#include "mongoose.h"
#include "cJSON.h"
#include "6zhentan.h"
#include "utils.h"
#include "queue_list.h"
#include "media_utils.h"
#include "media_storage.h"
#include "media_video.h"
#include "system_param.h"
#include "wifi.h"
#include "pnt_log.h"
#include "adas_config.h"
#include "dms_config.h"
#include "algo_common.h"
#include "gps_recorder.h"

#define DBG_PRINT           //printf
#define ERR_PRINT           printf

typedef struct
{
    
    char name[32];
    char value[32];

    void* next;

} paramItem;

extern BoardSysInfo_t gBoardInfo;
extern int gCurrentRtspChannel;

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

            cJSON_AddStringToObject(cjsonObjResultInReturn, "uuid", gGlobalSysParam->serialNum);
			
            cJSON_AddStringToObject(cjsonObjResultInReturn, "softver", gBoardInfo.mSwVersion);
            cJSON_AddStringToObject(cjsonObjResultInReturn, "otaver", "v1.20231007.1");
            cJSON_AddStringToObject(cjsonObjResultInReturn, "hwver", gBoardInfo.mHwVersion);
            cJSON_AddStringToObject(cjsonObjResultInReturn, "ssid", gGlobalSysParam->wifiSSID);
			char mac[32] = { 0 };
			getWifiMac("wlan0", mac);
            cJSON_AddStringToObject(cjsonObjResultInReturn, "bssid", mac);
            cJSON_AddNumberToObject(cjsonObjResultInReturn, "camnum", MAX_VIDEO_NUM);
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
			
            cJSON_AddStringToObject(cjsonObjResultInReturn, "value", "200000000");
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
				for (int i=0; i<MAX_VIDEO_NUM; i++)
				{
					total_count += queue_list_get_count(&gVideoListQueue[i]);
				}

				if ((end-start+1) < total_count)
				{
					total_count = (0 == end)?total_count:(end-start+1);
				}

				total_count = (total_count/MAX_VIDEO_NUM)*MAX_VIDEO_NUM;

	            cJSON_AddNumberToObject(paramItem, "count", total_count);

				if (total_count > 0)
				{					
		            cJSON * arrayFiles= cJSON_CreateArray();
					
					int j = 0, count = 0, exitCondition = 0;

					queue_t* head_chn[MAX_VIDEO_NUM] = { 0 };
					for (int i=0; i<MAX_VIDEO_NUM; i++)
					{
						head_chn[i] = gVideoListQueue[i].m_head->prev;
						exitCondition |= (1<<i);
					}
					queue_t* head_gps = (NULL==gGpsFileListQueue.m_head)?NULL:gGpsFileListQueue.m_head->prev;

					int extflag = 0;
					while (count < total_count)
					{
						for (int i=0; i<MAX_VIDEO_NUM; i++)
						{
							if (head_chn[i] != NULL && !(extflag&(1<<i)))
							{
								if (head_chn[i] == gVideoListQueue[i].m_head)
								{
									extflag |= (1<<i);
								}
								
								time_t fileTime = (time_t)head_chn[i]->m_content;

								get_videoname_by_time(i+1, fileTime, tempFileName);
								
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
				           	 		cJSON_AddNumberToObject(fileItem, "createtime", fileTime+gTimezoneOffset);
				           	 		cJSON_AddNumberToObject(fileItem, "type", 2);

									if (NULL != head_gps)
									{
										char gpsFile[128] = { 0 };
										if (abs((time_t)head_gps->m_content - fileTime) < 10)
										{
											gps_file_get_name((time_t)head_gps->m_content, gpsFile);
										}
										else if (head_gps->prev != NULL && abs((time_t)head_gps->prev->m_content - fileTime) < 10)
										{
											gps_file_get_name((time_t)head_gps->prev->m_content, gpsFile);
										}
										else if (head_gps->next != NULL && abs((time_t)head_gps->next->m_content - fileTime) < 10)
										{
											gps_file_get_name((time_t)head_gps->next->m_content, gpsFile);
										}
										cJSON_AddStringToObject(fileItem, "GPSPATH", gpsFile);
									}
									
				            		cJSON_AddItemToArray(arrayFiles, fileItem);
								
									count ++;
								}
								j += 1;
								
								head_chn[i] = head_chn[i]->prev;
							}
						}

						if (head_gps == gGpsFileListQueue.m_head)
						{
							head_gps = NULL;
						}
						else
						{
							if (NULL != head_gps) head_gps = head_gps->prev;
						}
							
						if (exitCondition == extflag)
						{
							break;
						}
					}

		            cJSON_AddItemToObject(paramItem, "files", arrayFiles);
					
		        	cJSON_AddItemToArray(array, paramItem);
				}
			}
			else if (!strcmp(folder_type, "park"))
			{
				for (int i=0; i<MAX_VIDEO_NUM; i++)
				{
					total_count += queue_list_get_count(&gParkPhotoListQueue[i]);
				}

				if ((end-start+1) < total_count)
				{
					total_count = (0 == end)?total_count:(end-start+1);
				}

				total_count = (total_count/MAX_VIDEO_NUM)*MAX_VIDEO_NUM;

	            cJSON_AddNumberToObject(paramItem, "count", total_count);

				if (total_count > 0)
				{					
		            cJSON * arrayFiles= cJSON_CreateArray();
					
					int j = 0, count = 0, exitCondition = 0;

					queue_t* head_chn[MAX_VIDEO_NUM] = { 0 };
					for (int i=0; i<MAX_VIDEO_NUM; i++)
					{
						head_chn[i] = gParkPhotoListQueue[i].m_head->prev;
						exitCondition |= (1<<i);
					}

					int extflag = 0;
					while (count < total_count)
					{
						for (int i=0; i<MAX_VIDEO_NUM; i++)
						{
							if (head_chn[i] != NULL && !(extflag&(1<<i)))
							{
								if (head_chn[i] == gParkPhotoListQueue[i].m_head)
								{
									extflag |= (1<<i);
								}
								
								time_t fileTime = (time_t)head_chn[i]->m_content;

								get_photoname_by_time(i+1, fileTime, tempFileName);
								
								struct stat buf = { 0 };
								memset(&buf, 0, sizeof(struct stat));
			    				stat(tempFileName, &buf);
								
								if (j >= start || (0 == end))
								{									
			            			cJSON * fileItem = cJSON_CreateObject();
				           	 		cJSON_AddStringToObject(fileItem, "name", tempFileName);
				           	 		cJSON_AddNumberToObject(fileItem, "duration", 0);
				           	 		cJSON_AddNumberToObject(fileItem, "size", buf.st_size/1024);
				           	 		cJSON_AddNumberToObject(fileItem, "createtime", fileTime+gTimezoneOffset);
				           	 		cJSON_AddNumberToObject(fileItem, "type", 1);

				            		cJSON_AddItemToArray(arrayFiles, fileItem);
								
									count ++;
								}
								j += 1;
								
								head_chn[i] = head_chn[i]->prev;
							}
						}

						if (exitCondition == extflag)
						{
							break;
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
            cJSON_AddStringToObject(paramItem, "range", "-150,150");
            cJSON_AddNumberToObject(paramItem, "step", 1);
            cJSON_AddStringToObject(paramItem, "unit", "cm");
            cJSON_AddItemToArray(array, paramItem);

			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "adas_draw");
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
            cJSON_AddStringToObject(paramItem, "name", "dms_draw");
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
            cJSON_AddStringToObject(paramItem, "name", "rdms_draw");
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
            cJSON_AddStringToObject(paramItem, "name", "ldw_sensitivity");
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
            cJSON_AddStringToObject(paramItem, "name", "hmw_sensitivity");
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
            cJSON_AddStringToObject(paramItem, "name", "fcw_sensitivity");
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
            cJSON_AddStringToObject(paramItem, "name", "pcw_sensitivity");
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
            cJSON_AddStringToObject(paramItem, "name", "fmw_sensitivity");
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
            cJSON_AddStringToObject(paramItem, "name", "srw_sensitivity");
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
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "srw_speed");
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
            cJSON_AddNumberToObject(paramItem, "value", gGlobalSysParam->limitSpeed);
            cJSON_AddItemToArray(array, paramItem);
				
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "car_width");
            cJSON_AddNumberToObject(paramItem, "value", gAdasAlgoParams->car_width);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "camera_to_ground");
            cJSON_AddNumberToObject(paramItem, "value", gAdasAlgoParams->camera_to_ground);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "camera_to_car_head");
            cJSON_AddNumberToObject(paramItem, "value", gAdasAlgoParams->car_head_to_camera);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "camera_to_middle");
            cJSON_AddNumberToObject(paramItem, "value", gAdasAlgoParams->camera_to_middle);
            cJSON_AddItemToArray(array, paramItem);
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "adas_draw");
	        cJSON_AddNumberToObject(paramItem, "value", gAlComParam.mAdasRender);
	        cJSON_AddItemToArray(array, paramItem);
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "dms_draw");
	        cJSON_AddNumberToObject(paramItem, "value", gAlComParam.mDmsRender);
	        cJSON_AddItemToArray(array, paramItem);
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "rdms_draw");
	        cJSON_AddNumberToObject(paramItem, "value", gAlComParam.mBsdRender);
	        cJSON_AddItemToArray(array, paramItem);
		
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "ldw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", gAdasAlgoParams->ldw_sensitivity);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "hmw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", gAdasAlgoParams->hmw_sensitivity);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "fcw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", gAdasAlgoParams->fcw_sensitivity);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "pcw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", gAdasAlgoParams->pcw_sensitivity);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "fmw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", gAdasAlgoParams->fmw_sensitivity);
            cJSON_AddItemToArray(array, paramItem);

			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "tdw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", gDmsAlgoParams->fati_sensitivity);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "dw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", gDmsAlgoParams->dist_sensitivity);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "sw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", gDmsAlgoParams->smok_sensitivity);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "pw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", gDmsAlgoParams->call_sensitivity);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "srw_sensitivity");
            cJSON_AddNumberToObject(paramItem, "value", gDmsAlgoParams->belt_sensitivity);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "fcw_speed");
            cJSON_AddNumberToObject(paramItem, "value", gAdasAlgoParams->fcw_speed);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "pcw_speed");
            cJSON_AddNumberToObject(paramItem, "value", gAdasAlgoParams->pcw_speed);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "hmw_speed");
            cJSON_AddNumberToObject(paramItem, "value", gAdasAlgoParams->hmw_speed);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "ldw_speed");
            cJSON_AddNumberToObject(paramItem, "value", gAdasAlgoParams->ldw_speed);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "tdw_speed");
            cJSON_AddNumberToObject(paramItem, "value", gDmsAlgoParams->fati_speed);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "dw_speed");
            cJSON_AddNumberToObject(paramItem, "value", gDmsAlgoParams->dist_speed);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "sw_speed");
            cJSON_AddNumberToObject(paramItem, "value", gDmsAlgoParams->smok_speed);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "pw_speed");
            cJSON_AddNumberToObject(paramItem, "value", gDmsAlgoParams->call_speed);
            cJSON_AddItemToArray(array, paramItem);
			
			paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "srw_speed");
            cJSON_AddNumberToObject(paramItem, "value", gDmsAlgoParams->belt_speed);
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
        DBG_PRINT("set %s=%s %s=%s\r\n", result->name, result->value, tmp->name, tmp->value);
        if (!strcmp(result->value, "osw_speed"))
        {
        	gGlobalSysParam->limitSpeed = atoi(tmp->value);
        }
		else if (!strcmp(result->value, "car_width"))
		{
			gAdasAlgoParams->car_width = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "camera_to_ground"))
		{
			gAdasAlgoParams->camera_to_ground = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "camera_to_car_head"))
		{
			gAdasAlgoParams->car_head_to_camera = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "camera_to_middle"))
		{
			gAdasAlgoParams->camera_to_middle = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "adas_draw"))
		{
			gAlComParam.mAdasRender = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "dms_draw"))
		{
			gAlComParam.mDmsRender = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "rdms_draw"))
		{
			gAlComParam.mBsdRender = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "ldw_sensitivity"))
        {
			gAdasAlgoParams->ldw_sensitivity = atoi(tmp->value);
        }
		else if (!strcmp(result->value, "hmw_sensitivity"))
		{
			gAdasAlgoParams->hmw_sensitivity = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "fcw_sensitivity"))
		{
			gAdasAlgoParams->fcw_sensitivity = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "pcw_sensitivity"))
		{
			gAdasAlgoParams->pcw_sensitivity = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "fmw_sensitivity"))
		{
			gAdasAlgoParams->fmw_sensitivity = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "tdw_sensitivity"))
		{
			gDmsAlgoParams->fati_sensitivity = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "dw_sensitivity"))
		{
			gDmsAlgoParams->dist_sensitivity = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "sw_sensitivity"))
		{
			gDmsAlgoParams->smok_sensitivity = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "pw_sensitivity"))
		{
			gDmsAlgoParams->call_sensitivity = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "srw_sensitivity"))
		{
			gDmsAlgoParams->belt_sensitivity = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "fcw_speed"))
		{
			gAdasAlgoParams->fcw_speed = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "pcw_speed"))
		{
			gAdasAlgoParams->pcw_speed = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "hmw_speed"))
		{
			gAdasAlgoParams->hmw_speed = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "ldw_speed"))
		{
			gAdasAlgoParams->ldw_speed = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "tdw_speed"))
		{
			gDmsAlgoParams->fati_speed = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "dw_speed"))
		{
			gDmsAlgoParams->dist_speed = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "sw_speed"))
		{
			gDmsAlgoParams->smok_speed = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "pw_speed"))
		{
			gDmsAlgoParams->call_speed = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "srw_speed"))
		{
			gDmsAlgoParams->belt_speed = atoi(tmp->value);
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

static cJSON* setDriverAreaParamsRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

	char folder_type[10] = { 0 };
	int start = 0, end = 0;
    paramItem* result = NULL, *tmp = NULL;
    setParamItems(hm->uri.p, &result);
    while (NULL != result)
    {
        DBG_PRINT("set param %s value %s\r\n", result->name, result->value);
		if (gDmsAlgoParams)
		{
			if (strstr(result->name, "lefttop_x"))
			{
				gDmsAlgoParams->active_rect_xs = atoi(result->value);
			}
			else if (strstr(result->name, "lefttop_y"))
			{
				gDmsAlgoParams->active_rect_ys = atoi(result->value);
			}
			else if (strstr(result->name, "rightbottom_x"))
			{
				gDmsAlgoParams->active_rect_xe = atoi(result->value);
			}
			else if (strstr(result->name, "rightbottom_y"))
			{
				gDmsAlgoParams->active_rect_ye = atoi(result->value);
			}
			gDmsAlgoParams->otherarea_cover |= 0x80; // update
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
		#if 0
		if (!strcmp(result->name, "timezone"))
		{
			if (result->value[0] == '-')
			{
				gGlobalSysParam->timezone = 0 - atoi(result->value+1);
			}
			else
			{
				gGlobalSysParam->timezone = atoi(result->value);
			}
		}
		#endif
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
            else if (!strcmp(result->value, "language"))
            {
                type = 5;
            }
			else if (!strcmp(result->value, "parking_monitor"))
			{
				type = 6;
			}
			else if (!strcmp(result->value, "timelapse_rate"))
			{
				type = 7;
			}
			else if (!strcmp(result->value, "driverfaces"))
			{
				type = 8;
			}
			else if (!strcmp(result->value, "driverfaces"))
			{
				type = 8;
			}
			else if (!strcmp(result->value, "face_enable"))
			{
				type = 9;
			}
			else if (!strcmp(result->value, "face_score_th"))
			{
				type = 10;
			}
	        else if (!strcmp(result->value, "wakealarm_enable"))
	        {
	        	type = 11;
	        }
	        else if (!strcmp(result->value, "wakealarm_interval"))
	        {
	        	type = 12;
	        }
	        else if (!strcmp(result->value, "passenger_enable"))
	        {
	        	type = 13;
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
		        cJSON_AddNumberToObject(paramItem, "value", MediaVideo_GetRecordStatus(gCurrentRtspChannel));
		        cJSON_AddItemToArray(array, paramItem);
			
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "language");
		        cJSON_AddNumberToObject(paramItem, "value", gGlobalSysParam->language);
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
	            cJSON_AddNumberToObject(paramItem, "value", param.protocalType);
	            cJSON_AddItemToArray(array, paramItem);
		
	            paramItem = cJSON_CreateObject();
				char intStr[10] = { 0 };
				sprintf(intStr, "%d", gSimulateSpeed);
	            cJSON_AddStringToObject(paramItem, "name", "sim_speed");
		        cJSON_AddStringToObject(paramItem, "value", intStr);
		        cJSON_AddItemToArray(array, paramItem);
				
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "apn_name");
		        cJSON_AddStringToObject(paramItem, "value", (const char*)gGlobalSysParam->APN);
		        cJSON_AddItemToArray(array, paramItem);
				
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "apn_account");
		        cJSON_AddStringToObject(paramItem, "value", (const char*)gGlobalSysParam->apnUser);
		        cJSON_AddItemToArray(array, paramItem);
				
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "apn_password");
		        cJSON_AddStringToObject(paramItem, "value", (const char*)gGlobalSysParam->apnPswd);
		        cJSON_AddItemToArray(array, paramItem);
			
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "parking_monitor");
		        cJSON_AddNumberToObject(paramItem, "value", gGlobalSysParam->parking_monitor);
		        cJSON_AddItemToArray(array, paramItem);
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "timelapse_rate");
		        cJSON_AddNumberToObject(paramItem, "value", gGlobalSysParam->timelapse_rate);
		        cJSON_AddItemToArray(array, paramItem);
	        }
			
	        cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, array);
		}
		else if ( 8 == type)
		{
			char name[128] = { 0 };
			int idx = 0;
			
			cJSON *array = cJSON_CreateArray();

			while (1)
			{
				if (facerecg_nn_get_drvier(idx, name))
				{
					break;
				}
	            cJSON *paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", name);
				cJSON_AddNumberToObject(paramItem, "index", idx+1);
	            cJSON_AddItemToArray(array, paramItem);

				idx ++;
			}
			
			cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, array);
		}
		else
		{
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
			        cJSON_AddNumberToObject(cjsonObjResultInReturn, "value", MediaVideo_GetRecordStatus(gCurrentRtspChannel));
				}
				else if (5 == type)
				{
		            cJSON_AddStringToObject(cjsonObjResultInReturn, "name", "language");
			        cJSON_AddNumberToObject(cjsonObjResultInReturn, "value", gGlobalSysParam->language);
				}
				else if (6 == type)
				{
		            cJSON_AddStringToObject(cjsonObjResultInReturn, "name", "parking_monitor");
			        cJSON_AddNumberToObject(cjsonObjResultInReturn, "value", gGlobalSysParam->parking_monitor);
				}
				else if (7 == type)
				{
		            cJSON_AddStringToObject(cjsonObjResultInReturn, "name", "timelapse_rate");
			        cJSON_AddNumberToObject(cjsonObjResultInReturn, "value", gGlobalSysParam->timelapse_rate);
				}
				else if (9 == type)
				{
		            cJSON_AddStringToObject(cjsonObjResultInReturn, "name", "face_enable");
			        cJSON_AddNumberToObject(cjsonObjResultInReturn, "value", gDmsAlgoParams->driver_recgn);
				}
				else if (10 == type)
				{
		            cJSON_AddStringToObject(cjsonObjResultInReturn, "name", "face_score_th");
			        cJSON_AddNumberToObject(cjsonObjResultInReturn, "value", gDmsAlgoParams->facerecg_match_score);
				}
		        else if (11 == type)
		        {
		            cJSON_AddStringToObject(cjsonObjResultInReturn, "name", "wakealarm_enable");
			        cJSON_AddNumberToObject(cjsonObjResultInReturn, "value", gGlobalSysParam->wakealarm_enable);
		        }
		        else if (12 == type)
		        {
		            cJSON_AddStringToObject(cjsonObjResultInReturn, "name", "wakealarm_interval");
			        cJSON_AddNumberToObject(cjsonObjResultInReturn, "value", gGlobalSysParam->wakealarm_interval);
		        }
		        else if (13 == type)
		        {
		            cJSON_AddStringToObject(cjsonObjResultInReturn, "name", "passenger_enable");
			        cJSON_AddNumberToObject(cjsonObjResultInReturn, "value", gDmsAlgoParams->passenger_enable);
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
            cJSON_AddStringToObject(paramItem, "name", "language");
            arrayItems= cJSON_CreateArray();
            arrayIndex= cJSON_CreateArray();
			for (int idx=0; idx<LANGUAGE_MAX; idx++)
			{
	            cJSON_AddItemToArray(arrayItems, cJSON_CreateString(LANGUAGE_STR(idx)));
	            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(idx));
			}
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
            cJSON_AddItemToArray(array, paramItem);
            
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "platform_protocol");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("808-2013"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("808-2019"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(2));
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
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "apn_name");
            cJSON_AddItemToArray(array, paramItem);
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "apn_account");
            cJSON_AddItemToArray(array, paramItem);
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "apn_password");
            cJSON_AddItemToArray(array, paramItem);

			if (0 == strcmp(gBoardInfo.mProductModel, "P-DC-2") || 0 == strcmp(gBoardInfo.mProductModel, "P-DC-3"))
			{
	            paramItem = cJSON_CreateObject();
	            cJSON_AddStringToObject(paramItem, "name", "parking_monitor");
	            arrayItems= cJSON_CreateArray();
	            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
	            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("on"));
	            cJSON_AddItemToObject(paramItem, "items", arrayItems);
	            arrayIndex= cJSON_CreateArray();
	            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
	            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
	            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
	            cJSON_AddItemToArray(array, paramItem);
			}
			
            paramItem = cJSON_CreateObject();
            cJSON_AddStringToObject(paramItem, "name", "timelapse_rate");
            arrayItems= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("off"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("1fps"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("3fps"));
            cJSON_AddItemToArray(arrayItems, cJSON_CreateString("5fps"));
            cJSON_AddItemToObject(paramItem, "items", arrayItems);
            arrayIndex= cJSON_CreateArray();
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(0));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(1));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(2));
            cJSON_AddItemToArray(arrayIndex, cJSON_CreateNumber(3));
            cJSON_AddItemToObject(paramItem, "index", arrayIndex);
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
	int ret = 0;
	
    paramItem* result = NULL, *tmp = NULL;
    setParamItems(hm->uri.p, &result);
    if (NULL != result)
    {
        tmp = result->next;
        DBG_PRINT("set %s=%s %s=%s\r\n", result->name, result->value, tmp->name, tmp->value);
        if (!strcmp(result->value, "switchcam"))
        {
            DBG_PRINT("set %s value %s\r\n", tmp->name, tmp->value);
            gCurrentRtspChannel = atoi(tmp->value);
        }
		else if (!strcmp(result->value, "speaker"))
		{
			gGlobalSysParam->mVolume = atoi(tmp->value);
			setSpeakerVolume(gGlobalSysParam->mVolume);
			playAudio("starting.pcm");
		}
		else if (!strcmp(result->value, "mic"))
		{
			gGlobalSysParam->micEnable = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "osd"))
		{
			gGlobalSysParam->osdEnable = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "language"))
		{
			gGlobalSysParam->language = atoi(tmp->value);
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
            setFlagStatus("simSpeed", gSimulateSpeed);
		}
		else if (!strcmp(result->value, "apn_name"))
		{
			strcpy((char*)gGlobalSysParam->APN, tmp->value);
		}
		else if (!strcmp(result->value, "apn_account"))
		{
			strcpy((char*)gGlobalSysParam->apnUser, tmp->value);
		}
		else if (!strcmp(result->value, "apn_password"))
		{
			strcpy((char*)gGlobalSysParam->apnPswd, tmp->value);
		}
		else if (!strcmp(result->value, "parking_monitor"))
		{
			gGlobalSysParam->parking_monitor = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "timelapse_rate"))
		{
			gGlobalSysParam->timelapse_rate = atoi(tmp->value);
		}
		else if (!strcmp(result->value, "dms_cover"))
		{
			if (gDmsAlgoParams)
			{
				gDmsAlgoParams->otherarea_cover = atoi(tmp->value);
				gDmsAlgoParams->otherarea_cover |= 0x80; // update
			}
		}
        else if (!strcmp(result->value, "face_enable")) // 开启人脸识别
        {
        	gDmsAlgoParams->driver_recgn = atoi(tmp->value);
        }
        else if (!strcmp(result->value, "face_reg")) // 人脸注册
        {
			FILE* f = fopen("/tmp/register", "wb");
			fwrite(tmp->value, 1, sizeof(tmp->value)-1, f);
			fclose(f);
        }
        else if (!strcmp(result->value, "face_score_th"))
        {
        	gDmsAlgoParams->facerecg_match_score = atof(tmp->value);
        }
		else if (!strcmp(result->value, "face_del")) // 人脸注册
		{
			char name[128] = { 0 };
			ret = facerecg_nn_del_driver(atoi(tmp->value), name);
			if (ret)
			{
		        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 1);
		        cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, "delete failed");
			}
			else
			{
				char name[128] = { 0 };
				int idx = 0;
				
				cJSON *array = cJSON_CreateArray();

				while (1)
				{
					if (facerecg_nn_get_drvier(idx, name))
					{
						break;
					}
		            cJSON *paramItem = cJSON_CreateObject();
		            cJSON_AddStringToObject(paramItem, "name", name);
					cJSON_AddNumberToObject(paramItem, "index", idx+1);
		            cJSON_AddItemToArray(array, paramItem);

					idx ++;
				}
				
		        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);
				cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, array);
			}
			ret = -1;
		}
        else if (!strcmp(result->value, "face_recg"))
        {
			FILE* f = fopen("/tmp/recg", "wb");
			fwrite(tmp->value, 1, sizeof(tmp->value)-1, f);
			fclose(f);
        }
        else if (!strcmp(result->value, "wakealarm_enable"))
        {
        	gGlobalSysParam->wakealarm_enable = atoi(tmp->value);
        }
        else if (!strcmp(result->value, "wakealarm_interval"))
        {
        	gGlobalSysParam->wakealarm_interval = atoi(tmp->value);
        }
        else if (!strcmp(result->value, "passenger_enable"))
        {
        	gDmsAlgoParams->passenger_enable = atoi(tmp->value);
        }
		
		free(tmp);	tmp = NULL;
        free(result);	result = NULL;
    }

    if (cjsonObjReturn != NULL && 0 == ret)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, "set success");
    }
    else
    {
        //ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
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
#if (defined WIFI_STATION)
        if (!strcmp(result->name, "wifissid"))
        {
        	strncpy(gGlobalSysParam->wifiSSIDEx, result->value, sizeof(gGlobalSysParam->wifiSSIDEx)-1);
        }
        else if (!strcmp(result->name, "wifipwd"))
        {
        	strncpy(gGlobalSysParam->wifiPswdEx, result->value, sizeof(gGlobalSysParam->wifiPswdEx)-1);
        }
#else
        if (!strcmp(result->name, "wifissid"))
        {
        	strncpy(gGlobalSysParam->wifiSSID, result->value, sizeof(gGlobalSysParam->wifiSSID)-1);
        }
        else if (!strcmp(result->name, "wifipwd"))
        {
        	strncpy(gGlobalSysParam->wifiPswd, result->value, sizeof(gGlobalSysParam->wifiPswd)-1);
        }
#endif
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

	WifiApReboot(1);

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

		if (RET_SUCCESS == checkTFCard(SDCARD_PATH))
		{
			system("touch /tmp/formatSDCard && chmod 755 /tmp/formatSDCard");

	        cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, "set success");			
		}
		else
		{
	        cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, "set failed");
		}
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * setRecoveryFactoryRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

		system("rm -rf /data/etc/*");
		
	   	cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, "set success");
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * getVideoRecordCurrDuration(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON *cjsonObjResultInReturn = cJSON_CreateObject();

        if (cjsonObjResultInReturn != NULL)
        {
            cJSON_AddNumberToObject(cjsonObjResultInReturn, "duration", MediaVideo_GetRecordDuration(gCurrentRtspChannel));
			
            cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, cjsonObjResultInReturn);
        }
    }
    else
    {
        ERR_PRINT("ERR: cJSON_CreateObject failed.\n");
    }

    return cjsonObjReturn;
}

static cJSON * getCameraInfoRequest(struct http_message *hm)
{
    cJSON * cjsonObjReturn = cJSON_CreateObject();

    if (cjsonObjReturn != NULL)
    {
        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);

        cJSON *cjsonObjResultInReturn = cJSON_CreateObject();

        if (cjsonObjResultInReturn != NULL)
        {
            cJSON_AddNumberToObject(cjsonObjResultInReturn, "camnum", 3);
			cJSON_AddStringToObject(cjsonObjResultInReturn, "online", "0,1,2");
            cJSON_AddNumberToObject(cjsonObjResultInReturn, "curcamid", gCurrentRtspChannel);
			
            cJSON_AddItemToObject(cjsonObjReturn, STR_RESINFO, cjsonObjResultInReturn);
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

    int i=0, j = 0, needReboot = 0;
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
		cjsonObjReturn = getVideoRecordCurrDuration(hm);
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
		cjsonObjReturn = setRecoveryFactoryRequest(hm);
		needReboot = 1;
    }
    else if (mg_vcmp(&hm->uri, REQUEST_GET_CAMERAINFO) == 0)
    {
        DBG_PRINT("REQUEST_GET_CAMERAINFO\r\n");
		cjsonObjReturn = getCameraInfoRequest(hm);
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
			#if (defined VDENCRYPT)
			
			char filename[128] = { 0 };
			strcpy(filename, VIDEO_PATH"/tmp.mp4");
			remove(filename);
			if (0 == copyFile(filename, cmd))
			{
				if (RET_SUCCESS == decrypt_video_file(filename))
					mg_http_serve_file(nc, hm, filename, mg_mk_str("video/mp4"), mg_mk_str(""));
			}
			
			#else
			
			mg_http_serve_file(nc, hm, cmd, mg_mk_str("video/mp4"), mg_mk_str(""));
			
			#endif
		}
		else if (strstr(hm->uri.p, PARKING_PATH"/CH"))
		{
			mg_http_serve_file(nc, hm, cmd, mg_mk_str("image/jpeg"), mg_mk_str(""));
		}
		else
		{
			mg_http_serve_file(nc, hm, cmd, mg_mk_str("application/octet-stream"), mg_mk_str(""));
		}
		return;
    }
	else if (mg_vcmp(&hm->uri, REQUEST_SET_DRVAREA_PARAMVALUES) == 0)
    {
        DBG_PRINT("REQUEST_SET_ADAS_PARAMVALUES\r\n");
        cjsonObjReturn = setDriverAreaParamsRequest(hm);
    }
    else if (mg_vcmp(&hm->uri, "/debug") == 0)
    {
        DBG_PRINT("REQUEST_SET_ADAS_PARAMVALUES\r\n");
		system("telnetd &");
    	cjsonObjReturn = cJSON_CreateObject();
	    if (cjsonObjReturn != NULL)
	    {
	        cJSON_AddNumberToObject(cjsonObjReturn, STR_RESULT, 0);
	        cJSON_AddStringToObject(cjsonObjReturn, STR_RESINFO, "set success");
	    }
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

	if (access("/tmp/formatSDCard", F_OK) == 0)
	{
		extern void SigSegvProc(int signo);
		SigSegvProc(0);
	}
	if (needReboot)
	{
		MediaStorage_Exit();
		sleep(2);
		system("reboot");
	}
}