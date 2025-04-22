#include<iostream>

#include "utils.h"
#include "adas_config.h"
#include "dms_config.h"

#include "main.h"

#define LOGE		printf

int FtpUpdateNodeParser(XMLElement* p_node)
{
    XMLElement* node = p_node->FirstChildElement();

    char ftpUrl[128] = { 0 }, ip[64] = { 0 };
    int port = -1;

    while (node)
    {
        if(node->GetText())
        {
            LOGE("name=%s value=%s\n", node->Name(), node->GetText());

            if (MATCH_NODE(node, "ip"))
            {
                strcpy(ip, node->GetText());
            }
            else if (MATCH_NODE(node, "port"))
            {
                port = atoi(node->GetText());
            }
        }
        node = node->NextSiblingElement();
    }

    if (strlen(ip) > 0)
    {
        if (-1 != port)
        {
            sprintf(ftpUrl, "ftp://%s:%d", ip, port);
        }
        else
        {
            sprintf(ftpUrl, "%s", ip);
        }
		remove("/data/etc/ftpUpdateURL");
        FILE* pfile = fopen("/data/etc/ftpUpdateURL", "wb");
        if (NULL == pfile)
        {
            LOGE("open /data/etc/ftpUpdateURL failed.\n");
            return -1;
        }

        fwrite(ftpUrl, 1, strlen(ftpUrl), pfile);

        fclose(pfile);
    }

    return 0;
}
