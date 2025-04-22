#ifndef _MCU_KWATCH_H_
#define _MCU_KWATCH_H_

#include "typedef.h"

typedef enum TsType {

    TYPE_TS_NONE,//无
    TYPE_TS_LIMIT_SPEED,//限速值.
    
} TsType;


/**
 *
1 .前车碰撞预警2级
2 .疲劳驾驶预警2级-闭眼
3 .盲区3级
4 .行人碰撞预警2级
5 .偏离驾驶位2级
6 .分神驾驶报警2级
7 .车距过近2级
8 .盲区2级
9 .疲劳驾驶预警1级-闭眼
10.车道左右偏离2级
11.前车碰撞预警1级
12.疲劳驾驶预警2级-打哈欠
13.限速牌超速预警
14.车距过近1级
15.行人碰撞预警1级
16.分神驾驶报警1级
17.车道左右偏离1级
18.盲区1级
19.疲劳驾驶预警1级-打哈欠
20.抽烟2级
21.打电话2级
22.抽烟1级
23.打电话1级
 */
typedef struct AlarmInfo {

    /**
     * 报警类型.
     * 从右边起为第一位.
     */
    uint32_t warningType;

    /**
     * 取值范围0-99,单位10S
     * 前车碰撞.
     */
    uint8_t fcwWarningValue;
    /**
    * 取值范围0-99,单位10S
    * 行人.
    */
    uint8_t pcwWarningValue;
    /*预留*/
    uint8_t bspWarningValue;

    /*预留*/
    uint8_t yuliuwarningValue;
    /*
     0:无效
     1：左
     2：右
   */
    uint32_t leftorright;
    /**
     * 交通标志牌标志
     */
    TsType tsType;

    /**
     * 交通标志的值.
     */
    uint32_t tsValue;

	volatile int alertFlag;
	
} AlarmInfo;

int KWatchReportAlarm(int level, int type, int extra, int extra2, float ttc);

void KWatchInit(void);

#endif

