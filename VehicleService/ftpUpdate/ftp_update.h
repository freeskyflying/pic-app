#ifndef _FTP_UPDATE_H_
#define _FTP_UPDATE_H_

#define FTP_USERNAME			"admin"
#define FTP_PASSWORD			"sb.123"
#define CONFIG_JSON_FILE		"/data/config.json"
#define OTA_JSON_FILE			"/data/ota.json"
#define OTA_FILE_DIR			"/data/ota"

int ftpUpdateCreate(void);

#endif
