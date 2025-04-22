#ifndef _MAIN_H_
#define _MAIN_H_


#include "tinyxml2.h"
#include <iostream>

#define ROOT_SYSTEM				"system"
#define ROOT_FTPUPDATE			"ftp_update"
#define ROOT_SERVERS			"servers"
#define ROOT_INSTALLATION		"installation"
#define ROOT_ALGORITHM			"algorithm"
#define ROOT_ALARM				"alarm"


using namespace std;
using namespace tinyxml2; //使用名称空间

#define MATCH_NODE(node,name)		(0 == strcmp(node->Name(),name))

typedef struct
{
	
	char name[32];
	int(*parser)(XMLElement* p_node);
	
} RootNode_t;

int SystemNodeParser(XMLElement* p_node);
int FtpUpdateNodeParser(XMLElement* p_node);
int ServersNodeParser(XMLElement* p_node);
int InstallationNodeParser(XMLElement* p_node);
int AlgorithmNodeParser(XMLElement* p_node);
int AlarmNodeParser(XMLElement* p_node);

#endif
