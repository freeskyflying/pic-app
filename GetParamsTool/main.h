#ifndef _MAIN_H_
#define _MAIN_H_


#include "tinyxml2.h"
#include <iostream>

#define ROOT_SYSTEM				"system"
#define ROOT_OTHERS				"others"
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
	int(*Encode)(XMLDocument* xml, XMLElement* p_node);
	
} RootNode_t;

int SystemNodeEncode(XMLDocument* xml, XMLElement* p_node);
int OthersNodeEncode(XMLDocument* xml, XMLElement* p_node);
int ServersNodeEncode(XMLDocument* xml, XMLElement* p_node);
int InstallationNodeEncode(XMLDocument* xml, XMLElement* p_node);
int AlgorithmNodeEncode(XMLDocument* xml, XMLElement* p_node);
int AlarmNodeEncode(XMLDocument* xml, XMLElement* p_node);

#endif
