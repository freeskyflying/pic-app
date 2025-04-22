#include<iostream>

#include "utils.h"
#include "adas_config.h"
#include "dms_config.h"

#include "main.h"

#define LOGE		printf

int ServersNodeParser(XMLElement* p_node)
{
    XMLElement* node = p_node->FirstChildElement();

    AppPlatformParams_t params = { 0 };
    int id = 0;

    while (node)
    {
        if (MATCH_NODE(node, "server"))
        {
            id = 0;
            XMLElement* subNode = node->FirstChildElement();
            while(subNode)
            {
                if (subNode->GetText())
                {
                    LOGE("name=%s value=%s\n", subNode->Name(), subNode->GetText());
                    
                    if (MATCH_NODE(subNode, "id"))
                    {
                        id = atoi(subNode->GetText());
                        if (id > 0)
                        {
                            getAppPlatformParams(&params, id - 1);
                        }
                    }
                    else if (MATCH_NODE(subNode, "ip"))
                    {
                        strncpy(params.ip, subNode->GetText(), sizeof(params.ip)-1);
                    }
                    else if (MATCH_NODE(subNode, "phonenum"))
                    {
                        strncpy(params.phoneNum, subNode->GetText(), sizeof(params.phoneNum)-1);
                    }
                    else if (MATCH_NODE(subNode, "protocal"))
                    {
                        params.protocalType = atoi(subNode->GetText());
                    }
                    else if (MATCH_NODE(subNode, "port"))
                    {
                        params.port = atoi(subNode->GetText());
                    }
                    else if (MATCH_NODE(subNode, "platenum"))
                    {
                        strncpy(params.plateNum, subNode->GetText(), sizeof(params.plateNum) - 1);
                    }
                    else if (MATCH_NODE(subNode, "platecolor"))
                    {
                        params.plateColor = atoi(subNode->GetText());
                    }
                }

                subNode = subNode->NextSiblingElement();
            }
            if (id != 0)
            {
                setAppPlatformParams(&params, id - 1);
            }
        }
        node = node->NextSiblingElement();
    }
    
    return 0;
}
