#include<iostream>

#include "utils.h"
#include "adas_config.h"
#include "dms_config.h"

#include "main.h"
#include "system_param.h"

#define LOGE		printf

int SystemNodeParser(XMLElement* p_node)
{
    XMLElement* node = p_node->FirstChildElement();

    if (NULL == gGlobalSysParam)
    {
        Global_SystemParam_Init();        
    }
    
    while (node)
    {
		if (MATCH_NODE(node, "apn"))
		{
			LOGE("node=%s\n", node->Name());
			XMLElement* subNode = node->FirstChildElement();
			memset(gGlobalSysParam->APN, 0, sizeof(gGlobalSysParam->APN));
			memset(gGlobalSysParam->apnUser, 0, sizeof(gGlobalSysParam->apnUser));
			memset(gGlobalSysParam->apnPswd, 0, sizeof(gGlobalSysParam->apnPswd));
			while(subNode)
			{
				if (subNode->GetText())
				{
					LOGE("name=%s value=%s\n", subNode->Name(), subNode->GetText());
					if (MATCH_NODE(subNode, "name"))
					{
						strncpy((char*)gGlobalSysParam->APN, subNode->GetText(), sizeof(gGlobalSysParam->APN)-1);
					}
					else if (MATCH_NODE(subNode, "user"))
					{
						strncpy((char*)gGlobalSysParam->apnUser, subNode->GetText(), sizeof(gGlobalSysParam->apnUser)-1);
					}
					else if (MATCH_NODE(subNode, "password"))
					{
						strncpy((char*)gGlobalSysParam->apnPswd, subNode->GetText(), sizeof(gGlobalSysParam->apnPswd)-1);
					}
				}
				subNode = subNode->NextSiblingElement();
			}
		}
        else if(node->GetText())
        {
			LOGE("name=%s value=%s\n", node->Name(), node->GetText());
		
			int value = atoi(node->GetText());

			if (MATCH_NODE(node, "vol_level"))
			{
				if (0 <= value && 3 >= value)
					gGlobalSysParam->mVolume = value;
			}
			else if (MATCH_NODE(node, "dms_en"))
			{
				gGlobalSysParam->dsmEnable = (value!=0);
			}
			else if (MATCH_NODE(node, "adas_en"))
			{
				gGlobalSysParam->adasEnable = (value!=0);
			}
			else if (MATCH_NODE(node, "osd_en"))
			{
				gGlobalSysParam->osdEnable = (value!=0);
			}
			else if (MATCH_NODE(node, "mic_en"))
			{
				gGlobalSysParam->micEnable = (value!=0);
			}
			else if (MATCH_NODE(node, "parking_monitor"))
			{
				gGlobalSysParam->parking_monitor = (value!=0);
			}
			else if (MATCH_NODE(node, "timelapse_rate"))
			{
				gGlobalSysParam->timelapse_rate = (value>3?3:value);
			}
			else if (MATCH_NODE(node, "overspeed_report_intv"))
			{
				gGlobalSysParam->overspeed_report_intv = value;
			}
			else if (MATCH_NODE(node, "wakealarm_enable"))
			{
				gGlobalSysParam->wakealarm_enable = (value!=0);
			}
			else if (MATCH_NODE(node, "wakealarm_interval"))
			{
				gGlobalSysParam->wakealarm_interval = value;
			}
        }
        node = node->NextSiblingElement();
    }

    return 0;
}
