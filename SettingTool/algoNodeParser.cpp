#include<iostream>

#include "utils.h"
#include "adas_config.h"
#include "dms_config.h"

#include "main.h"

#define LOGE        printf

int AdasInstallationNodeParser(XMLElement* p_node)
{
    XMLElement* node = p_node->FirstChildElement();

    if (NULL == gAdasAlgoParams)
    {
        adas_al_para_init();
    }
    
    while(node)
    {
        if (node->GetText())
        {
            LOGE("name=%s value=%s\n", node->Name(), node->GetText());
            
            if (MATCH_NODE(node, "vanish_point_x"))
            {
                gAdasAlgoParams->vanish_point_x = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "vanish_point_y"))
            {
                gAdasAlgoParams->vanish_point_y = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "bottom_line_y"))
            {
                gAdasAlgoParams->bottom_line_y = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "camera_to_ground"))
            {
                gAdasAlgoParams->camera_to_ground = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "car_head_to_camera"))
            {
                gAdasAlgoParams->car_head_to_camera = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "car_width"))
            {
                gAdasAlgoParams->car_width = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "camera_to_middle"))
            {
                gAdasAlgoParams->camera_to_middle = atoi(node->GetText());
            }
        }

        node = node->NextSiblingElement();
    }

    return 0;
}

int AdasAlgoNodeParser(XMLElement* p_node)
{
    XMLElement* node = p_node->FirstChildElement();

    if (NULL == gAdasAlgoParams)
    {
        adas_al_para_init();
    }
    
    while(node)
    {
        if (node->GetText())
        {
            LOGE("name=%s value=%s\n", node->Name(), node->GetText());
        }

        node = node->NextSiblingElement();
    }

    return 0;
}

int DmsAlgoNodeParser(XMLElement* p_node)
{
    XMLElement* node = p_node->FirstChildElement();

    if (NULL == gDmsAlgoParams)
    {
        dms_al_para_init();
    }
    
    while(node)
    {
        if (node->GetText())
        {
            LOGE("name=%s value=%s\n", node->Name(), node->GetText());
        }

        node = node->NextSiblingElement();
    }

    return 0;
}

int AdasAlarmNodeParser(XMLElement* p_node)
{
    XMLElement* node = p_node->FirstChildElement();

    if (NULL == gAdasAlgoParams)
    {
        adas_al_para_init();
    }
    
    while(node)
    {
        if (node->GetText())
        {
            LOGE("name=%s value=%s\n", node->Name(), node->GetText());

            if (MATCH_NODE(node, "ldw_en"))
            {
                gAdasAlgoParams->ldw_en = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "ldw_photo_count"))
            {
                gAdasAlgoParams->ldw_photo_count = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "ldw_photo_intervel_ms"))
            {
                gAdasAlgoParams->ldw_photo_intervel_ms = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "ldw_video_time"))
            {
                gAdasAlgoParams->ldw_video_time = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "ldw_intervel"))
            {
                gAdasAlgoParams->ldw_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "ldw_speed_level2"))
            {
                gAdasAlgoParams->ldw_speed_level2 = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "hmw_en"))
            {
                gAdasAlgoParams->hmw_en = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "hmw_photo_count"))
            {
                gAdasAlgoParams->hmw_photo_count = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "hmw_photo_intervel_ms"))
            {
                gAdasAlgoParams->hmw_photo_intervel_ms = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "hmw_video_time"))
            {
                gAdasAlgoParams->hmw_video_time = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "hmw_intervel"))
            {
                gAdasAlgoParams->hmw_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "hmw_speed_level2"))
            {
                gAdasAlgoParams->hmw_speed_level2 = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fcw_en"))
            {
                gAdasAlgoParams->fcw_en = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fcw_photo_count"))
            {
                gAdasAlgoParams->fcw_photo_count = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fcw_photo_intervel_ms"))
            {
                gAdasAlgoParams->fcw_photo_intervel_ms = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fcw_video_time"))
            {
                gAdasAlgoParams->fcw_video_time = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fcw_intervel"))
            {
                gAdasAlgoParams->fcw_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fcw_speed_level2"))
            {
                gAdasAlgoParams->fcw_speed_level2 = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "pcw_en"))
            {
                gAdasAlgoParams->pcw_en = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "pcw_photo_count"))
            {
                gAdasAlgoParams->pcw_photo_count = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "pcw_photo_intervel_ms"))
            {
                gAdasAlgoParams->pcw_photo_intervel_ms = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "pcw_video_time"))
            {
                gAdasAlgoParams->pcw_video_time = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "pcw_intervel"))
            {
                gAdasAlgoParams->pcw_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "pcw_speed_level2"))
            {
                gAdasAlgoParams->pcw_speed_level2 = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fmw_en"))
            {
                gAdasAlgoParams->fmw_en = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fmw_photo_count"))
            {
                gAdasAlgoParams->fmw_photo_count = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fmw_photo_intervel_ms"))
            {
                gAdasAlgoParams->fmw_photo_intervel_ms = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fmw_video_time"))
            {
                gAdasAlgoParams->fmw_video_time = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fmw_intervel"))
            {
                gAdasAlgoParams->fmw_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "alarm_speed"))
            {
                gAdasAlgoParams->alarm_speed = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "ldw_sensitivity"))
            {
                gAdasAlgoParams->ldw_sensitivity = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "hmw_sensitivity"))
            {
                gAdasAlgoParams->hmw_sensitivity = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fcw_sensitivity"))
            {
                gAdasAlgoParams->fcw_sensitivity = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "pcw_sensitivity"))
            {
                gAdasAlgoParams->pcw_sensitivity = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fmw_sensitivity"))
            {
                gAdasAlgoParams->fmw_sensitivity = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "ldw_voice_intervel"))
            {
                gAdasAlgoParams->ldw_voice_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "hmw_voice_intervel"))
            {
                gAdasAlgoParams->hmw_voice_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fcw_voice_intervel"))
            {
                gAdasAlgoParams->fcw_voice_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "pcw_voice_intervel"))
            {
                gAdasAlgoParams->pcw_voice_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fmw_voice_intervel"))
            {
                gAdasAlgoParams->fmw_voice_intervel = atoi(node->GetText());
            }
        }

        node = node->NextSiblingElement();
    }

    return 0;
}

int DmsAlarmNodeParser(XMLElement* p_node)
{
    
    XMLElement* node = p_node->FirstChildElement();

    if (NULL == gDmsAlgoParams)
    {
        dms_al_para_init();
    }
    
    while(node)
    {
        if (node->GetText())
        {
            LOGE("name=%s value=%s\n", node->Name(), node->GetText());

            if (MATCH_NODE(node, "call_enable"))
            {
                gDmsAlgoParams->call_enable = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "call_photo_count"))
            {
                gDmsAlgoParams->call_photo_count = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "call_photo_intervel_ms"))
            {
                gDmsAlgoParams->call_photo_intervel_ms = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "call_video_time"))
            {
                gDmsAlgoParams->call_video_time = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "call_intervel"))
            {
                gDmsAlgoParams->call_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "call_speed_level2"))
            {
                gDmsAlgoParams->call_speed_level2 = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "smok_enable"))
            {
                gDmsAlgoParams->smok_enable = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "smok_photo_count"))
            {
                gDmsAlgoParams->smok_photo_count = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "smok_photo_intervel_ms"))
            {
                gDmsAlgoParams->smok_photo_intervel_ms = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "smok_video_time"))
            {
                gDmsAlgoParams->smok_video_time = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "smok_intervel"))
            {
                gDmsAlgoParams->smok_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "smok_speed_level2"))
            {
                gDmsAlgoParams->smok_speed_level2 = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "dist_enable"))
            {
                gDmsAlgoParams->dist_enable = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "dist_photo_count"))
            {
                gDmsAlgoParams->dist_photo_count = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "dist_photo_intervel_ms"))
            {
                gDmsAlgoParams->dist_photo_intervel_ms = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "dist_video_time"))
            {
                gDmsAlgoParams->dist_video_time = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "dist_intervel"))
            {
                gDmsAlgoParams->dist_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "dist_speed_level2"))
            {
                gDmsAlgoParams->dist_speed_level2 = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fati_enable"))
            {
                gDmsAlgoParams->fati_enable = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fati_photo_count"))
            {
                gDmsAlgoParams->fati_photo_count = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fati_photo_intervel_ms"))
            {
                gDmsAlgoParams->fati_photo_intervel_ms = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fati_video_time"))
            {
                gDmsAlgoParams->fati_video_time = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fati_intervel"))
            {
                gDmsAlgoParams->fati_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fati_speed_level2"))
            {
                gDmsAlgoParams->fati_speed_level2 = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "nodriver_enable"))
            {
                gDmsAlgoParams->nodriver_enable = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "nodriver_photo_count"))
            {
                gDmsAlgoParams->nodriver_photo_count = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "nodriver_photo_intervel_ms"))
            {
                gDmsAlgoParams->nodriver_photo_intervel_ms = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "nodriver_video_time"))
            {
                gDmsAlgoParams->nodriver_video_time = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "nodriver_intervel"))
            {
                gDmsAlgoParams->nodriver_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "abnm_speed_level2"))
            {
                gDmsAlgoParams->abnm_speed_level2 = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "cover_enable"))
            {
                gDmsAlgoParams->cover_enable = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "cover_photo_count"))
            {
                gDmsAlgoParams->cover_photo_count = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "cover_photo_intervel_ms"))
            {
                gDmsAlgoParams->cover_photo_intervel_ms = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "cover_video_time"))
            {
                gDmsAlgoParams->cover_video_time = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "cover_intervel"))
            {
                gDmsAlgoParams->cover_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "dist_enable"))
            {
                gDmsAlgoParams->glass_enable = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "glass_photo_count"))
            {
                gDmsAlgoParams->glass_photo_count = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "glass_photo_intervel_ms"))
            {
                gDmsAlgoParams->glass_photo_intervel_ms = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "glass_video_time"))
            {
                gDmsAlgoParams->glass_video_time = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "glass_intervel"))
            {
                gDmsAlgoParams->glass_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "alarm_speed"))
            {
                gDmsAlgoParams->alarm_speed = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fati_sensitivity"))
            {
                gDmsAlgoParams->fati_sensitivity = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "dist_sensitivity"))
            {
                gDmsAlgoParams->dist_sensitivity = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "smok_sensitivity"))
            {
                gDmsAlgoParams->smok_sensitivity = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "call_sensitivity"))
            {
                gDmsAlgoParams->call_sensitivity = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "belt_sensitivity"))
            {
                gDmsAlgoParams->belt_sensitivity = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "driver_rect_topleft_x"))
            {
                gDmsAlgoParams->driver_rect_xs = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "driver_rect_topleft_y"))
            {
                gDmsAlgoParams->driver_rect_ys = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "driver_rect_botright_x"))
            {
                gDmsAlgoParams->driver_rect_xe = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "driver_rect_botright_y"))
            {
                gDmsAlgoParams->driver_rect_ye = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "passenger_enable"))
            {
                gDmsAlgoParams->passenger_enable = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "passenger_photo_cnt"))
            {
                gDmsAlgoParams->passenger_photo_cnt = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "call_voice_intervel"))
            {
                gDmsAlgoParams->call_voice_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "smok_voice_intervel"))
            {
                gDmsAlgoParams->smok_voice_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "dist_voice_intervel"))
            {
                gDmsAlgoParams->dist_voice_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "fati_voice_intervel"))
            {
                gDmsAlgoParams->fati_voice_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "nobelt_voice_intervel"))
            {
                gDmsAlgoParams->nobelt_voice_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "nodriver_voice_intervel"))
            {
                gDmsAlgoParams->nodriver_voice_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "glass_voice_intervel"))
            {
                gDmsAlgoParams->glass_voice_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "cover_voice_intervel"))
            {
                gDmsAlgoParams->cover_voice_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "active_rect_xs"))
            {
                gDmsAlgoParams->active_rect_xs = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "active_rect_ys"))
            {
                gDmsAlgoParams->active_rect_ys = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "active_rect_xe"))
            {
                gDmsAlgoParams->active_rect_xe = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "active_rect_ye"))
            {
                gDmsAlgoParams->active_rect_ye = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "otherarea_cover"))
            {
                gDmsAlgoParams->otherarea_cover = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "driverchg_enable"))
            {
                gDmsAlgoParams->driverchg_enable = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "driverchg_photo_count"))
            {
                gDmsAlgoParams->driverchg_photo_count = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "driverchg_photo_intervel_ms"))
            {
                gDmsAlgoParams->driverchg_photo_intervel_ms = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "driverchg_video_time"))
            {
                gDmsAlgoParams->driverchg_video_time = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "driverchg_intervel"))
            {
                gDmsAlgoParams->driverchg_intervel = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "driver_recgn"))
            {
                gDmsAlgoParams->driver_recgn = atoi(node->GetText());
            }
            else if (MATCH_NODE(node, "facerecg_match_score"))
            {
                gDmsAlgoParams->facerecg_match_score = atof(node->GetText());
            }
        }

        node = node->NextSiblingElement();
    }

    return 0;
}

int InstallationNodeParser(XMLElement* p_node)
{
    XMLElement* node = p_node->FirstChildElement();

    while (node)
    {
        if (MATCH_NODE(node, "adas"))
        {
            AdasInstallationNodeParser(node);
        }

        node = node->NextSiblingElement();
    }
    
    return 0;
}

int AlgorithmNodeParser(XMLElement* p_node)
{
    XMLElement* node = p_node->FirstChildElement();

    while (node)
    {
        if (MATCH_NODE(node, "adas"))
        {
            AdasAlgoNodeParser(node);
        }
        else if (MATCH_NODE(node, "dms"))
        {
            DmsAlgoNodeParser(node);
        }

        node = node->NextSiblingElement();
    }
    
    return 0;
}

int AlarmNodeParser(XMLElement* p_node)
{
    XMLElement* node = p_node->FirstChildElement();

    while (node)
    {
        if (MATCH_NODE(node, "adas"))
        {
            AdasAlarmNodeParser(node);
        }
        else if (MATCH_NODE(node, "dms"))
        {
            DmsAlarmNodeParser(node);
        }

        node = node->NextSiblingElement();
    }
    
    return 0;
}
