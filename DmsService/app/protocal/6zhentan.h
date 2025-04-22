#ifndef _6ZHENTAN_H_
#define _6ZHENTAN_H_

#include "mongoose.h"

#define RETURN_HTTP_HEAD            "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
#define ERR_RETURN_HTTP_HEAD        "HTTP/1.1 404 OK\r\nTransfer-Encoding: chunked\r\n\r\n"

#define STR_RESULT                  "result"
#define STR_RESINFO                 "info"

#define REQUEST_GET_PRODUCTINFO         "/app/getproductinfo"
#define REQUEST_GET_DEVICEATTR          "/app/getdeviceattr"
#define REQUEST_GET_MEDIAINFO           "/app/getmediainfo"
#define REQUEST_GET_TFCARDINFO          "/app/getsdinfo"
#define REQUEST_GET_BATTERYINFO         "/app/getbatteryinfo"

#define REQUEST_SET_SYSTIME             "/app/setsystime"
#define REQUEST_SET_TIMEZONE            "/app/settimezone"

#define REQUEST_GET_RECDURATION         "/app/getrecduration"

#define REQUEST_SET_SETTING             "/app/setting"
#define REQUEST_SET_PLAYBACK            "/app/playback"
#define REQUEST_SET_WIFI                "/app/setwifi"

#define REQUEST_SD_FORMAT               "/app/sdformat"

#define REQUEST_PARAMS_RESET            "/app/reset"

#define REQUEST_GET_CAMERAINFO          "/app/ getcamerainfo"

#define REQUEST_SET_PARAMVALUE          "/app/setparamvalue"

#define REQUEST_SNAPSHOT                "/app/snapshot"

#define REQUEST_LOCK_VIDEO              "/app/lockvideo"

#define REQUEST_WIFI_REBOOT             "/app/wifireboot"

#define REQUEST_GET_CAPABILITY          "/app/capability"

#define REQUEST_GET_FILELIST            "/app/getfilelist"

#define REQUEST_GET_THUMBNAIL           "/app/getthumbnail"

#define REQUEST_DELETE_FILE             "/app/deletefile"

#define REQUEST_QUERY_PARAMITEMS        "/app/getparamitems"

#define REQUEST_GET_PARAMVALUES         "/app/getparamvalue"

#define REQUEST_GET_ADAS_PARAMITEMS     "/app/getadasitems"
#define REQUEST_GET_ADAS_PARAMVALUES    "/app/getadasvalue"
#define REQUEST_SET_ADAS_PARAMVALUES    "/app/setadasvalue"

void HttpGetRequestProcess(struct mg_connection *nc, struct http_message *hm);

#endif

