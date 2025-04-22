#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <errno.h>
#include <sys/prctl.h>
#include <linux/falloc.h>

#include "main.h"
#include "adas_config.h"
#include "dms_config.h"
#include "system_param.h"

#define LOGE		printf

const RootNode_t ROOT_NODES[] =
{
	{ROOT_SYSTEM, SystemNodeParser},
	{ROOT_FTPUPDATE, FtpUpdateNodeParser},
	{ROOT_SERVERS, ServersNodeParser},
	{ROOT_INSTALLATION, InstallationNodeParser},
	{ROOT_ALGORITHM, AlgorithmNodeParser},	
	{ROOT_ALARM, AlarmNodeParser},	
};

int ParseXmlFile(char* filename)
{
	XMLDocument doc;
	doc.LoadFile(filename);
	
	XMLElement*root=doc.RootElement();//获取根元素,通过XMLDocument来获取
	if (NULL != root)
	{
		XMLElement*tmpnode=root->FirstChildElement();
		while (tmpnode)
		{
			for (int i=0; i<sizeof(ROOT_NODES)/sizeof(ROOT_NODES[0]); i++)
			{
				if (strcmp(tmpnode->Name(), ROOT_NODES[i].name) == 0)
				{
					if (ROOT_NODES[i].parser)
						ROOT_NODES[i].parser(tmpnode);
				}
			}
			
			tmpnode=tmpnode->NextSiblingElement();
		}
	}
	else
	{
		LOGE("load xml file %s failed.\n", filename);
	}
	
	return 0;
}

int main(int argc, char** argv)
{
	if (argc < 2 || NULL == argv[1])
	{
		LOGE("param error.usage: SettingTool xxx\n");
		return -1;
	}

	LOGE("pwd: %s.\n", argv[0]);

	char filePath[512] = { 0 };
	DIR *dirp;
	struct dirent *dp;
	dirp = opendir(argv[1]); //打开目录指针
	if (dirp == NULL) {
		LOGE("not open %s.\n", argv[1]);
	}
	else
	{
		while ((dp = readdir(dirp)) != NULL) { //通过目录指针读目录
			if (NULL != strstr(dp->d_name, ".xml")) {
				sprintf(filePath,"%s/%s", argv[1], dp->d_name);
				
				// 解析xml文件
				ParseXmlFile(filePath);
			}
		}
	}

	if (NULL != gGlobalSysParam)
	{
		Global_SystemParam_DeInit();
	}

	if (NULL != gAdasAlgoParams)
	{
		adas_al_para_deinit();
	}

	if (NULL != gDmsAlgoParams)
	{
		dms_al_para_deinit();
	}

    return 0;
}

