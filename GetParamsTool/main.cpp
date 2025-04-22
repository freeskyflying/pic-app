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
#include <string>

#include "main.h"
#include "adas_config.h"
#include "dms_config.h"
#include "system_param.h"

#define LOGE		printf

const RootNode_t ROOT_NODES[] =
{
	{ROOT_SYSTEM, SystemNodeEncode},
	{ROOT_OTHERS, OthersNodeEncode},
	{ROOT_SERVERS, ServersNodeEncode},
	{ROOT_INSTALLATION, InstallationNodeEncode},
	{ROOT_ALGORITHM, AlgorithmNodeEncode},	
	{ROOT_ALARM, AlarmNodeEncode},	
};

int SystemNodeEncode(XMLDocument* xml, XMLElement* p_node)
{
	if (NULL == gGlobalSysParam)
	{
		return -1;
	}
	
	XMLElement* node = xml->NewElement("vol_level");
	XMLText* node_text = xml->NewText(std::to_string(gGlobalSysParam->mVolume).c_str());	node->InsertFirstChild(node_text);	p_node->InsertEndChild(node);
	
	node = xml->NewElement("dms_en");
	node_text = xml->NewText(std::to_string(gGlobalSysParam->dsmEnable).c_str());	node->InsertFirstChild(node_text);	p_node->InsertEndChild(node);
	
	node = xml->NewElement("adas_en");
	node_text = xml->NewText(std::to_string(gGlobalSysParam->adasEnable).c_str());	node->InsertFirstChild(node_text);	p_node->InsertEndChild(node);
	
	node = xml->NewElement("osd_en");
	node_text = xml->NewText(std::to_string(gGlobalSysParam->osdEnable).c_str());	node->InsertFirstChild(node_text);	p_node->InsertEndChild(node);
	
	node = xml->NewElement("mic_en");
	node_text = xml->NewText(std::to_string(gGlobalSysParam->micEnable).c_str());	node->InsertFirstChild(node_text);	p_node->InsertEndChild(node);
	
	node = xml->NewElement("parking_monitor");
	node_text = xml->NewText(std::to_string(gGlobalSysParam->parking_monitor).c_str());	node->InsertFirstChild(node_text);	p_node->InsertEndChild(node);
	
	node = xml->NewElement("timelapse_rate");
	node_text = xml->NewText(std::to_string(gGlobalSysParam->timelapse_rate).c_str());	node->InsertFirstChild(node_text);	p_node->InsertEndChild(node);
	
	node = xml->NewElement("overspeed_report_intv");
	node_text = xml->NewText(std::to_string(gGlobalSysParam->overspeed_report_intv).c_str());	node->InsertFirstChild(node_text);	p_node->InsertEndChild(node);
	
	XMLElement* subRoot = xml->NewElement("apn");
	node = xml->NewElement("name");
	node_text = xml->NewText((const char*)gGlobalSysParam->APN);	node->InsertFirstChild(node_text);	subRoot->InsertEndChild(node);
	node = xml->NewElement("user");
	node_text = xml->NewText((const char*)gGlobalSysParam->apnUser);	node->InsertFirstChild(node_text);	subRoot->InsertEndChild(node);
	node = xml->NewElement("password");
	node_text = xml->NewText((const char*)gGlobalSysParam->apnPswd);	node->InsertFirstChild(node_text);	subRoot->InsertEndChild(node);
	p_node->InsertEndChild(subRoot);

	return 0;
}

int OthersNodeEncode(XMLDocument* xml, XMLElement* p_node)
{
	XMLElement* node = xml->NewElement("ftpUrl");
	
	char ftpUrl[128] = { 0 };
    FILE* pfile = fopen("/data/etc/ftpUpdateURL", "rb");
    if (NULL != pfile)
    {
	    fread(ftpUrl, 1, strlen(ftpUrl), pfile);
	    fclose(pfile);
    }
	else
	{
		sprintf(ftpUrl, "ftp://39.108.149.82:8021");
	}
	
	XMLText* node_text = xml->NewText((const char*)ftpUrl);	node->InsertFirstChild(node_text);	p_node->InsertEndChild(node);


	BoardSysInfo_t info = { 0 };
	getBoardSysInfo(&info);
	
	node = xml->NewElement("model");
	node_text = xml->NewText((const char*)info.mProductModel);	node->InsertFirstChild(node_text);	p_node->InsertEndChild(node);
	
	node = xml->NewElement("sn");
	node_text = xml->NewText((const char*)gGlobalSysParam->serialNum);	node->InsertFirstChild(node_text);	p_node->InsertEndChild(node);
	
	node = xml->NewElement("hwversion");
	node_text = xml->NewText((const char*)info.mHwVersion);	node->InsertFirstChild(node_text);	p_node->InsertEndChild(node);
	
	node = xml->NewElement("swversion");
	node_text = xml->NewText((const char*)info.mSwVersion);	node->InsertFirstChild(node_text);	p_node->InsertEndChild(node);
	
	return 0;
}

int ServersNodeEncode(XMLDocument* xml, XMLElement* p_node)
{
	AppPlatformParams_t param = { 0 };

	for (int i=0; i<5; i++)
	{
		memset(&param, 0, sizeof(param));
		
		getAppPlatformParams(&param, i);

		if (strlen(param.ip) > 0)
		{
			XMLElement* subRoot = xml->NewElement("server");
			
			XMLElement* node = xml->NewElement("id");
			XMLText* node_text = xml->NewText((const char*)std::to_string(i+1).c_str());	node->InsertFirstChild(node_text);	subRoot->InsertEndChild(node);
			node = xml->NewElement("ip");
			node_text = xml->NewText((const char*)param.ip);	node->InsertFirstChild(node_text);	subRoot->InsertEndChild(node);
			node = xml->NewElement("port");
			node_text = xml->NewText((const char*)std::to_string(param.port).c_str());	node->InsertFirstChild(node_text);	subRoot->InsertEndChild(node);
			node = xml->NewElement("phonenum");
			node_text = xml->NewText((const char*)param.phoneNum);	node->InsertFirstChild(node_text);	subRoot->InsertEndChild(node);
			node = xml->NewElement("protocal");
			node_text = xml->NewText((const char*)std::to_string(param.protocalType).c_str());	node->InsertFirstChild(node_text);	subRoot->InsertEndChild(node);
			
			p_node->InsertEndChild(subRoot);
		}
	}
	
	return 0;
}

int InstallationNodeEncode(XMLDocument* xml, XMLElement* p_node)
{
	return 0;
}

int AlgorithmNodeEncode(XMLDocument* xml, XMLElement* p_node)
{
	return 0;
}

int AlarmNodeEncode(XMLDocument* xml, XMLElement* p_node)
{
	return 0;
}

int main(int argc, char** argv)
{
	if (argc < 2 || NULL == argv[1])
	{
		LOGE("param error.usage: SettingTool xxx\n");
		return -1;
	}
	
	Global_SystemParam_Init();
	adas_al_para_init();
	dms_al_para_init();

	XMLDocument xml;
	// 插入声明
	XMLDeclaration* declaration = xml.NewDeclaration();
	xml.InsertFirstChild(declaration);
 
	// 插入根节点
	XMLElement* rootNode = xml.NewElement("root");
	xml.InsertEndChild(rootNode);

	for (int i=0; i<sizeof(ROOT_NODES)/sizeof(ROOT_NODES[0]); i++)
	{
		XMLElement* subRoot = xml.NewElement(ROOT_NODES[i].name);
			
		if (NULL != ROOT_NODES[i].Encode)
		{			
			ROOT_NODES[i].Encode(&xml, subRoot);
		}
		xml.InsertEndChild(subRoot);
	}

	xml.SaveFile(argv[1]);
	
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

