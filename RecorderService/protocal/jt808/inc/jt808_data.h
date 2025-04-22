#ifndef _JT808_DATA_H_
#define _JT808_DATA_H_

#include "queue_list.h"

#define PHONE_NUM_LEN       		6
#define PRODUCERID_LEN      		5
#define PRODUCTMODEL_LEN    		20
#define TERMINALID_LEN      		7
#define DEFAULTLENGTH       		64

#define PHONE_NUM_LEN_2019  		10
#define PRODUCERID_LEN_2019      	11
#define PRODUCTMODEL_LEN_2019    	30
#define TERMINALID_LEN_2019      	30

#define PHONE_NUM_LEN_MAX      		10
#define PRODUCERID_LEN_MAX     		11
#define PRODUCTMODEL_LEN_MAX   		30
#define TERMINALID_LEN_MAX     		30

#define BODY_PACKET_LEN				(0x3FF)

typedef unsigned short WORD;

typedef unsigned char BYTE;

typedef unsigned int DWORD;

#define JT808_FLAG_SET(a,f,v)               { a &=(~f); a |= v; }
#define JT808_FLAG_CLR(a,f)                 { a &=(~f) }

/* 通用应答结果定义 */
typedef enum
{

    COM_RESULT_SUCCESS = 0,
    COM_RESULT_FAILED,
    COM_RESULT_ERROR,
    COM_RESULT_UNSUPPORT,
    COM_RESULT_ALRMPROCESS_CONFIRM,

} JT808_TERMIAL_COMMON_RESULT;

/* 终端注册结果定义 */
typedef enum
{

    TERM_REG_SUCCESS = 0,
    TERM_REG_VEIHICAL_REGISTERED,
    TERM_REG_VEIHICAL_NOT_IN_DB,
    TERM_REG_TERMINAL_REGISTERED,
    TERM_REG_TERMINAL_NOT_IN_DB,

} JT808_TERMINAL_REGISTER_RESULT;

/* 终端状态位定义 */
typedef enum
{

    JT808_TERMINAL_STATE_ACC_STATE				= (1<<0),           /* ACC状态：1-ON 0-OFF */
    JT808_TERMINAL_STATE_GPS_STATE				= (1<<1),           /* 定位状态：1-已定位 0-未定位 */
    JT808_TERMINAL_STATE_SOUTH_NORTH			= (1<<2),           /* 0-北纬 1-南纬 */
    JT808_TERMINAL_STATE_EAST_WEST				= (1<<3),           /* 0-东经 1-西经 */
    JT808_TERMINAL_STATE_OPERATION_STATE		= (1<<4),           /* 0-运营状态 1-停运状态 */
    JT808_TERMINAL_STATE_LOCATION_CRYPTE		= (1<<5),           /* 0-经纬度未加密 1-经纬度已加密 */
    JT808_TERMINAL_STATE_EBS_PRE_FCW			= (1<<6),           /* 1-紧急刹车系统采集的前撞预警 */
    JT808_TERMINAL_STATE_PRE_LDW				= (1<<7),           /* 1-车道偏移预警 */
    JT808_TERMINAL_STATE_MANNED_STATE			= (3<<8),           /* 00-空车  01-半载  10-保留  11-满载 */
    JT808_TERMINAL_STATE_OILCIRCUIT_STATE		= (1<<10),          /* 0-车辆油路正常  1-车辆油路断开 */         
    JT808_TERMINAL_STATE_CIRCUIT_STATE			= (1<<11),          /* 0-车辆电路正常  1-车辆电路断开 */
    JT808_TERMINAL_STATE_DOORLOCK_STATE			= (1<<12),          /* 0-车门解锁  1-车门加锁 */
    JT808_TERMINAL_STATE_DOOR1_STATE			= (1<<13),          /* 0-门1关  1-门1开（前门） */
    JT808_TERMINAL_STATE_DOOR2_STATE			= (1<<14),          /* 0-门2关  1-门2开（中门） */
    JT808_TERMINAL_STATE_DOOR3_STATE			= (1<<15),          /* 0-门3关  1-门3开（后门） */
    JT808_TERMINAL_STATE_DOOR4_STATE			= (1<<16),          /* 0-门4关  1-门4开（驾驶席门） */
    JT808_TERMINAL_STATE_DOOR5_STATE			= (1<<17),          /* 0-门5关  1-门5开（自定义） */
    JT808_TERMINAL_STATE_USING_GPS				= (1<<18),          /* 0-未使用GPS定位  1-已使用GPS定位 */
    JT808_TERMINAL_STATE_USING_BD				= (1<<19),          /* 0-未使用北斗定位  1-已使用北斗定位 */
    JT808_TERMINAL_STATE_USING_GLONASS			= (1<<20),          /* 0-未使用GLONASS定位  1-已使用GLONASS定位 */
    JT808_TERMINAL_STATE_USING_GALILEO			= (1<<21),          /* 0-未使用GALILEO定位  1-已使用GALILEO定位 */
    JT808_TERMINAL_STATE_VEHICAL_STATE			= (1<<22),          /* 0-车辆静止状态  1-车辆行驶状态 */
    

} JT808_TERMINAL_STATE_BIT;

/* 报警标志位定义 */
typedef enum
{

    JT808_ALERT_FLAG_SOS					= (1<<0),               /* 紧急报警 */
    JT808_ALERT_FLAG_OVERSPEED				= (1<<1),               /* 超速报警 */
    JT808_ALERT_FLAG_FATIGUEDRIVE			= (1<<2),               /* 疲劳驾驶（超时间驾驶） */
    JT808_ALERT_FLAG_DANGERDRIVE			= (1<<3),               /* 危险驾驶 */
    JT808_ALERT_FLAG_GNSSFAULT				= (1<<4),               /* GNSS模块故障 */
    JT808_ALERT_FLAG_GNSSANTOPEN			= (1<<5),               /* GNSS天线断开或未接 */
    JT808_ALERT_FLAG_GNSSANTCLOSE			= (1<<6),               /* GNSS天线短路 */
    JT808_ALERT_FLAG_MAJORPOWLOWVOLT		= (1<<7),               /* 终端主电源欠压 */
    JT808_ALERT_FLAG_MAJORPOWOPEN			= (1<<8),               /* 终端主电源掉电 */
    JT808_ALERT_FLAG_MONITORFAULT			= (1<<9),               /* 显示器故障 */
    JT808_ALERT_FLAG_TTSFAULT				= (1<<10),              /* TTS模块故障 */
    JT808_ALERT_FLAG_CAMERAFAULT			= (1<<11),              /* 摄像头故障 */
    JT808_ALERT_FLAG_ICREADERFAULT			= (1<<12),              /* 道路运输证IC卡模块故障 */
    JT808_ALERT_FLAG_PRE_OVERSPEED			= (1<<13),              /* 超速预警 */
    JT808_ALERT_FLAG_PRE_FATIGUEDRIVE		= (1<<14),              /* 疲劳驾驶预警 */
    JT808_ALERT_FLAG_VIOLATIONDRIVE			= (1<<15),              /* 违规行驶 */
    JT808_ALERT_FLAG_TIRPRESSURE			= (1<<16),              /* 胎压预警 */
    JT808_ALERT_FLAG_RTURNBSDABNORMAL		= (1<<17),              /* 右转盲区异常 */
    JT808_ALERT_FLAG_CURRENTDAYOVERDRIVE	= (1<<18),              /* 当天累计驾驶超时报警 */
    JT808_ALERT_FLAG_STOPTIMEOUT			= (1<<19),              /* 停车超时 */
    JT808_ALERT_FLAG_GETINOUTAREA			= (1<<20),              /* 进出区域 */
    JT808_ALERT_FLAG_GETINOUTROAD			= (1<<21),              /* 进出路线 */
    JT808_ALERT_FLAG_ROADFASTORSLOW			= (1<<22),              /* 路段行驶时间不足/过长 */
    JT808_ALERT_FLAG_WRONGROAD				= (1<<23),              /* 偏离路线 */
    JT808_ALERT_FLAG_VSSFAULT				= (1<<24),              /* 车辆VSS模块故障 */
    JT808_ALERT_FLAG_OILABNORMAL			= (1<<25),              /* 车辆油量异常 */
    JT808_ALERT_FLAG_BURGLARALARM			= (1<<26),              /* 防盗器报警 */
    JT808_ALERT_FLAG_ACCILLEGAL				= (1<<27),              /* 非法点火 */
    JT808_ALERT_FLAG_MOVINGILLEGAL			= (1<<28),              /* 非法位移 */
    JT808_ALERT_FLAG_ROLLOVER				= (1<<29),              /* 侧翻 */
    JT808_ALERT_FLAG_PRE_ROLLOVER			= (1<<30),              /* 侧翻预警 */

} JT808_ALERT_FALG_BIT;

/* 拓展车辆信号状态位定义 */
typedef enum
{

    JT808_EXTRA_VEHICEL_STATE_JINGUANGDENG			= 1 <<0,		/* 1-近光灯信号 */
    JT808_EXTRA_VEHICEL_STATE_YUANGUANGDENG			= 1 <<1,		/* 1-远光灯信号 */
    JT808_EXTRA_VEHICEL_STATE_YOUZHUANXIANGDENG		= 1 <<2,		/* 1-右转向灯信号 */
    JT808_EXTRA_VEHICEL_STATE_ZUOZHUANXIANGDENG		= 1 <<3,		/* 1-左转向灯信号 */
    JT808_EXTRA_VEHICEL_STATE_ZHIDONG				= 1 <<4,		/* 1-制动信号 */
    JT808_EXTRA_VEHICEL_STATE_DAODANG				= 1 <<5,		/* 1-倒挡信号 */
    JT808_EXTRA_VEHICEL_STATE_WUDENG				= 1 <<6,		/* 1-雾灯信号 */
    JT808_EXTRA_VEHICEL_STATE_SHILANGDENG			= 1 <<7,		/* 1-示廊灯信号 */
    JT808_EXTRA_VEHICEL_STATE_LABA				 	= 1 <<8,		/* 1-喇叭信号 */
    JT808_EXTRA_VEHICEL_STATE_KONGTIAO				= 1 <<9,		/* 1-空调状态 */
    JT808_EXTRA_VEHICEL_STATE_KONGDANG				= 1 <<10,		/* 1-空挡信号 */
    JT808_EXTRA_VEHICEL_STATE_HUANSUQI				= 1 <<11,		/* 1-缓速器工作 */
    JT808_EXTRA_VEHICEL_STATE_ABS				    = 1 <<12,		/* 1-ABS工作 */
    JT808_EXTRA_VEHICEL_STATE_JIAREQI				= 1 <<13,		/* 1-加热器工作 */
    JT808_EXTRA_VEHICEL_STATE_LIHEQI				= 1 <<14,		/* 1-离合器状态 */

} JT808_EXTRA_VEHICEL_STATE_BIT;

/* IO状态定义 */
typedef enum
{

    JT808_VEHICEL_IO_STATE_DEEPSLEEP                = 1 << 0,       /* 1-深度休眠状态 */
    JT808_VEHICEL_IO_STATE_SLEEP                    = 1 << 1,       /* 休眠状态 */

} JT808_VEHICEL_IO_STATE_BIT;

/* 终端控制命令字定义 */
typedef enum
{

    JT808_TERMINAL_CTRL_CMD_CONNECT_SERVER,                     /* 控制终端连接指定服务器，参数为string */
    JT808_TERMINAL_CTRL_CMD_RESET,                              /* 复位 */
    JT808_TERMINAL_CTRL_CMD_FACTORY_RESET,                      /* 恢复出厂设置 */

} JT808_TERMINAL_CTRL_CMD;

/* 位置上报附加信息ID定义 */
typedef enum
{

    JT808_EXTRA_MSG_ID_MILLAGE = 0x01, 				 								/* 里程，DWORD，1/10km，对应车上里程表读数 */
    JT808_EXTRA_MSG_ID_OIL_QUANTITY = 0x02, 				 						/* 油量，WORD，1/10L，对应车上油量表读数 */
    JT808_EXTRA_MSG_ID_RECORD_SPEED = 0x03, 				 						/* 行驶记录功能获取的速度，WORD，1/10km/h */
    JT808_EXTRA_MSG_ID_NEED_MANUAL_CONFIRM = 0x04, 				 					/* 需要人工确认报警事件的 ID，WORD，从 1 开始计数 */
    JT808_EXTRA_MSG_ID_TIRE_POW = 0x05,   				 							/* 胎压 */
    JT808_EXTRA_MSG_ID_TEMP = 0x06,   				 								/* 车厢温度 */
    JT808_EXTRA_MSG_ID_SPEEDING = 0x11, 				 							/* 超速报警附加信息见"表-28" */
    JT808_EXTRA_MSG_ID_ENTER_OR_EXIT_SPECIFIED_AREA_OR_ROUTE_WARNING = 0x12, 		/* 进出区域/路线报警附加信息见"表-29" */
    JT808_EXTRA_MSG_ID_INSUFFICIENT_OR_EXCEED_DRIVING_TIME_WARNING = 0x13, 			/* 路段行驶时间不足/过长报警附加信息见"表-30" */
    
    JT808_EXTRA_MSG_ID_VEHICLE_WORDING_STATUS = 0x22, 				 				/* 车辆工作状态 */
    
    JT808_EXTRA_MSG_ID_EXTEND_CAR_SIGNAL_STATE = 0x25, 				 				/* 扩展车辆信号状态位，定义见"表-31" @ JT808_EXTRA_VEHICEL_STATE_BIT */
    JT808_EXTRA_MSG_ID_IO_STATE = 0x2A, 				 							/* IO状态位，定义见"表-32" @ JT808_VEHICEL_IO_STATE_BIT */
    JT808_EXTRA_MSG_ID_ANALOG_QUANTITY = 0x2B, 				 						/* 模拟量,bit0-15,AD0;bit16-31,AD1 */
    JT808_EXTRA_MSG_ID_WIRELESS_NETWORK_SIGNAL_STRENTH = 0x30, 				 		/* 无线通信网络信号强度 */
    JT808_EXTRA_MSG_ID_GNSS_SATELLITE_NUM = 0x31, 				 					/* GNSS 定位卫星数 */
    
    JT808_EXTRA_MSG_ID_FOLLOW_UP_MSG_LEN = 0xE0, 				 					/* 后续自定义信息长度 */
    
    JT808_EXTRA_MSG_ID_ADAS_WARNING = 0x64, 				 						/* 高级驾驶辅助系统报警信息 */
    JT808_EXTRA_MSG_ID_DMS_WARNING = 0x65, 					 						/* 驾驶员状态监测系统报警信息 */
    JT808_EXTRA_MSG_ID_TPMS_WARNING = 0x66, 				 						/* 胎压监测系统报警信息 */
    JT808_EXTRA_MSG_ID_BSD_WARNING = 0x67, 				 							/* 盲区监测系统报警信息 */

    JT808_VIDEO_RELATED_ALARM = 0x14, 				 								/* 视频相关报警 DWORD 按位设置 */
    JT808_VIDEO_SIGNAL_MISSING_ALARM_STATUS = 0x15, 				 				/* 视频信号丢失报警状态 DWORD 按位设置 */
    JT808_VIDEO_SIGNAL_OCCLUSION_ALARM_STATUS = 0x16, 				 				/* 视频信号遮挡报警状态 DWORD 按位设置 */
    JT808_MEMORY_FAILURE_ALARM_STATUS = 0x17, 				 						/* 存储器故障报警状态 DWORD 按位设置 */
    JT808_ABNORMAL_DRIVING_BEHAVIOR_ALARM_DETAILED_DESCRIPTION = 0x18, 				/* 异常驾驶行为报警详细描述 DWORD 按位设置 */
    JT808_EXTRA_MSG_ID_MAKE_TRUN = 0x19,				 							/* 转弯状态. */

    JT808_EXTRA_MSG_ID_LIFT_STATUS = 0x20, 				 							/* 举升状态 */

    JT808_EXTRA_MSG_ID_POSITIVE_INVERSION_STATUS = 0x52, 				 			/* 正反转状态 */

    JT808_EXTRA_MSG_ID_POSITIVE_INVERSION_STATUS_NJ = 0x08, 				 		/* 正反转状态 - 南京亚士德项目 */
    JT808_EXTRA_MSG_ID_PASSENGER_FLOW_STA_VALUE = 0xE1, 				 			/* 莱弗特客流统计 */
    JT808_EXTRA_MSG_ID_EQUIPMENT_FAULT = 0xE3, 				 						/* 设备故障 */
    JT808_EXTRA_MSG_ID_CONSTRUCT_INSPEC_WARNING = 0xE4, 				 			/* 施工巡检报警 */
    JT808_EXTRA_MSG_ID_TEMPERATURE_VALUE = 0xEA, 				 					/* 温度 */
    JT808_EXTRA_MSG_ID_HUMIDITY_VALUE = 0xEB 				 						/* 湿度或者油量 */

} JT808_EXTRA_MSG_ID;

/* 终端参数ID定义 */
typedef enum
{

    HEART_BEAT_SEND_INTERVAL = 0X0001,                          // 终端心跳发送间隔，单位为秒（s）
    TCP_MSG_RESPONSE_TIMEOUT = 0X0002,                          // TCP 消息应答超时时间，单位为秒（s）
    TCP_MSG_RETRY_COUNT = 0x0003,                               // TCP 消息重传次数

    UDP_MSG_RESPONSE_TIMEOUT = 0X0004,                          // UDP 消息应答超时时间，单位为秒（s）
    UDP_MSG_RETRY_COUNT = 0x0005,                               // UDP 消息重传次数
    SMS_MSG_RESPONSE_TIMEOUT = 0X0006,                          // SMS 消息应答超时时间，单位为秒（s）
    SMS_MSG_RETRY_COUNT = 0x0007,                               // SMS 消息重传次数
    MAIN_SERVER_APN = 0x0010,                                   // 主服務器APN訪問點
    MAIN_SERVER_WIRELESS_NAME = 0x0011,                         // 主服务器无线通信拨号用户名
    MAIN_SERVER_WIRELESS_PASSWD = 0x0012,                       // 主服务器无线通信拨号密码

    MAIN_SERVER_DOMAIN_ADDRESS = 0x0013,                        // 主服务器地址,IP 或域名:端口，多个服务器使用分号分割

    BACKUP_SERVER_APN = 0x0014,                                 //备用服務器APN訪問點
    BACKUP_SERVER_WIRELESS_NAME = 0x0015,                       //备用服务器无线通信拨号用户名
    BACKUP_SERVER_WIRELESS_PASSWD = 0x0016,                     //备用服务器无线通信拨号密码

    BACKUP_SERVER_DOMAIN_ADDRESS = 0x0017,                      // 备份服务器地址,IP 或域名:端口，多个服务器使用分号分割
    //SERVER_TCP_PORT = 0x0018,                                 // 服务器 TCP 端口

    IC_AUTH_SERVER_IP = 0x001a,                                 // IC卡认证主服务器IP或域名
    IC_AUTH_SERVER_TCP_PORT = 0x001b,                           // IC卡认证主服务器TCP端口
    IC_AUTH_SERVER_UDP_PORT = 0x001c,                           // IC卡认证主服务器UDP端口
    IC_AUTH_BACKUP_SERVER_IP = 0x001d,                          // IC卡认证备份服务器IP或域名，端口同主服务器

    LOCATION_UPLOAD_STRATEGY = 0x0020,                          // 位置汇报策略，0：定时汇报；1：定距汇报；2：定时和定距汇报
    LOCATION_UPLOAD_PROGRAM = 0x0021,                           // 位置汇报方案，0：根据 ACC 状态； 1：根据登录状态和 ACC 状态，先判断登录状态，若登录再根据 ACC 状态

    DRIVER_NO_LOGIN_REPORT_TIME_INTERVAL = 0x0022,              // 驾驶员未登录汇报时间间隔，单位为s，值大于0

    SLAVE_SERVER_APN = 0x0023,                                  // 从服務器APN訪問點，值为空时，使用与主服务器相同配置
    SLAVE_SERVER_WIRELESS_NAME = 0x0024,                        // 从服务器无线通信拨号用户名
    SLAVE_SERVER_WIRELESS_PASSWD = 0x0025,                      // 从服务器无线通信拨号密码
    SLAVE_SERVER_BACKUP_IP = 0x0026,                            // 从服务器地址,IP 或域名:端口，多个服务器使用分号分割

    UPLOAD_INTERVAL_WHILE_SLEEP = 0x0027,                       // 休眠时汇报时间间隔，单位为秒（s），>0
    UPLOAD_INTERVAL_WHILE_EMERGENCY_WARNING = 0x0028,           // 紧急报警时汇报时间间隔，单位为秒（s），>0
    DEFAULT_UPLOAD_INTERVAL = 0x0029,                           // 缺省时间汇报间隔，单位为秒（s），>0
    DEFAULT_UPLOAD_DISTANCE = 0x002c,                           // 缺省距离汇报间隔，单位为秒（m），>0

    DRIVER_NO_LOGIN_REPORT_LOCATION_INTERVAL = 0x002d,          // 驾驶员未登录汇报距离间隔，单位为m，值大于0


    UPLOAD_DISTANCE_WHILE_SLEEP = 0x002E,                       // 休眠时汇报距离间隔，单位为米（m），>0
    UPLOAD_DISTANCE_WHILE_EMERGENCY_WARNING = 0x002F,           // 紧急报警时汇报距离间隔，单位为米（m），>0

    //拐点
    //        Inflexion point
    INFLXION_POINT_ANGLE = 0x0030,                              // 拐点补传角度，值小于180
    GEO_R = 0x0031,                                             // 电子围栏半径（非法位移阈值），单位为m
    VIOLATION_DRIVING_TIME  = 0x0032,                           // 违规行驶时段范围，精确到分

    MONITOR_PLATFORM_PHONE_NUM = 0x0040,                        // 监控平台电话号码
    RESET_PHONE_NUM = 0x0041,                                   // 复位电话号码，可采用此电话号码拨打终端电话复位终端
    RESUMPTION_TO_FACTORY = 0x0042,                             // 恢复出厂设置电话号码，可采用此电话号码拨打终端使终端恢复出厂设置
    MONITOR_PLATFORM_SMS_NUM = 0x0043,                          // 监控平台SMS电话号码
    RECV_TERMINAL_SMS_ALARM_PHONE_NUM = 0x0044,                 // 接收终端SMS文本报警号码
    TERMINAL_ANSWER_PHONE_STRATEGY = 0x0045,                    // 终端电话接听策略，0-自动接听 1-ACC ON时自动接听，OFF时手动接听
    MAX_ON_THE_PHONE_TIME_ONE_TIME = 0x0046,                    // 每次最长通话时长，单位为s，0-不允许通话  0xFFFFFFFF-不限制
    MAX_ON_THE_PHONE_TIME_ONE_MONTH = 0x0047,                   // 当月最长通话时长，单位为s，0-不允许通话  0xFFFFFFFF-不限制
    LISTEN_NUMBER = 0x0048,                                     // 监听电话号码
    MONITOR_PLATFORM_PRIVILEGE_PHONE_NUM = 0x0049,              // 监管平台特权短信号码

    WARNING_MASK_WORD = 0x0050,                                 // 报警屏蔽字，与位置信息汇报消息中的报警标志相对应，相应位为 1则相应报警被屏蔽 : 控制哪些报警信息不上传 @ JT808_ALERT_FALG_BIT

    WARNING_SEND_TEXT_SMS_SWITCH = 0x0051,                      // 报警发送文本SMS开关，与位置信息汇报消息中的报警标志相对应，相应位为 1则相应报警时发送SMS文本 @ JT808_ALERT_FALG_BIT

    WARNING_SHOOTING_SWITCH = 0x0052,                           // 报警拍摄开关，与位置信息汇报消息中的报警标志相对应，相应位为1 则相应报警时摄像头拍摄 @ JT808_ALERT_FALG_BIT
    WARNING_SHOOTING_STORE_SWITCH = 0x0053,                     // 报警拍摄存储标志，与位置信息汇报消息中的报警标志相对应，相应位为 1 则对相应报警时拍的照片进行存储，否则实时上传 @ JT808_ALERT_FALG_BIT

    IMPORT_FLAG = 0x0054,                                       // 关键标志，与位置信息汇报消息中的报警标志相对应，相应位为1 则相应报警为关键报警 @ JT808_ALERT_FALG_BIT

    MAXIMUM_SPEED = 0x0055,                                     // 最高速度，单位为公里每小时（km/h）
    EXCEED_SPEED_LIMIT_DURATION = 0x0056,                       // 超速持续时间，单位为秒（s）


    CONTINUE_DRIVING_TIME_LIMIT = 0x0057,                       // 连续驾驶时间门限，单位为s
    CURRENT_DAY_DRIVING_TIME_LIMIT = 0x0058,                    // 当天累计驾驶时间门限，单位为s
    MIN_RSET_TIME = 0x0059,                                     // 最小休息时间，单位为s
    MAX_STOP_TIME = 0x005a,                                     // 最长停车时间,单位为秒(s)

    EXCEED_SPEED_LIMIT_EARLY_WARNING_DIFFERENCE = 0x005B,       // 超速报警预警差值，单位为 1/10Km/h
    FATIGUE_DRIVING_EARYLY_WARNING_DIFFERENCE = 0x005C,         // 疲劳驾驶预警差值，单位为秒（s），>0
    COLLISION_ALRAM_PARA = 0x005d,                              // 碰撞报警参数设置：b7-b0：碰撞时间，单位ms；b15-b8：碰撞加速度，单位0.1g，设置范围为0-79，默认10
    ROLLOVER_ALARM_PARA = 0x005e,                               // 侧翻报警参数设置：侧翻角度，单位为度（°），默认为30°

    TIMED_SHOOTING_CONTROL = 0x0064,                            // 定时拍照控制
    FIXED_DISTANCE_SHOOTING_CONTROL = 0x0065,                   // 定距拍照控制
    IMG_OR_VIDEO_QUALITY = 0x0070,                              // 图像/视频质量，1-10，1 最好
    LUMINANCE = 0x0071,                                         // 亮度，0-255
    PARAM_CONTRAST = 0x0072,                                    // 对比度，0-127
    PARAM_SATURATION = 0x0073,                                  // 饱和度，0-127
    PARAM_CHROMA = 0x0074,                                      // 色度，0-255

    //1078
    AV_PARMS_SETTING1078 		          = 0x0075,             //音视参数设置
    AV_CHANNEL_LIST_SETTING1078  		  = 0x0076,             //音视通道列表设置
    ALONE_VIDEO_CHANNEL_PARMS_SETTING1078 = 0x0077,             //单独视频通道参数设置
    SPECIAL_ALARM_VIDEO_PARMS_SETTING1078 = 0x0079,             //特殊报警参数设置
    VIDEO_RELAT_ALARM_FLAG1078            = 0x007A,             //视频相关报警屏蔽字
    VIDEO_IMAGE_ANALYSIS_SETTING1078      = 0x007B ,            //视频图像分析
    TERMINAL_WAKEUP1078                   = 0x007C,             //终端休眠唤醒

    ODOMETER_VALUE = 0x0080,                                    // 车辆里程表读数，单位：1/10km

    CAR_PROVINCIAL_ID = 0x0081,                                 // 车辆所在的省域 ID
    CAR_COUNTY_ID = 0x0082,                                     // 车辆所在的市域 ID
    LICENSE_PLATE = 0x0083,                                     // 公安交通管理部门颁发的机动车号牌
    LICENSE_PLATE_COLOR = 0x0084,                               // 车牌颜色，按照 JT/T415-2006 的 5.4.12

    GNSS_MODE = 0x0090,                                         // GNSS定位模式
    GNSS_BAUDRATE = 0x0091,                                     // GNSS波特率
    GNSS_OUTPUT_RATE = 0x0092,                                  // GNSS模块详细定位数据输出频率
    GNSS_GETINFO_RATE = 0x0093,                                 // GNSS模块详细定位数据采样频率，单位s
    GNSS_UPLOAD_MODE = 0x0094,                                  // GNSS模块详细定位数据上传方式
    GNSS_UPLOAD_SET = 0x0095,
    CAN1_GET_INFO_TIME_INTERVAL = 0x0100,                       // CAN总线通道1采集时间间隔，单位ms，0表示不采集
    CAN1_UPLOAD_INFO_TIME_INTERVAL = 0x0101,                    // CAN总线通道1上传时间间隔，单位s，0表示不上传
    CAN2_GET_INFO_TIME_INTERVAL = 0x0102,                       // CAN总线通道2采集时间间隔，单位ms，0表示不采集
    CAN2_UPLOAD_INFO_TIME_INTERVAL = 0x0103,                    // CAN总线通道2上传时间间隔，单位s，0表示不上传
    CAN_BUS_ID_SET = 0x0110,                                    // CAN总线ID单独采集设置
    CAN_OTHERID_SET = 0x0111,                                   // 用于其他CAN总线ID单独采集设置


    ADAS_PARAMS = 0xF364,                                       // 高级驾驶辅助系统参数 -- 苏标
    DMS_PARAMS = 0xF365,                                        // 驾驶员状态监测系统参数 -- 苏标
    TPMS_PARAMS = 0xF366,                                       // 胎压监测系统参数 -- 苏标
    BDS_PARAMS = 0xF367,                                        // 盲区监测系统参数 -- 苏标

    //新川标增加的参数设置ID
    CB_INTENSE_DRIVING_PARAMS = 0xF370,                         // 激烈驾驶检测功能参数 -- 川标
    CB_LOAD_MONITOR_PARAMS = 0xF372,                            // 载重监测功能参数 -- 川标
    CB_HEIGHT_MONITOR_PARAMS = 0xF373,                          // 限高检测功能参数 -- 川标
    CB_NIGHT_OVERSPEED_PARAMS = 0xF001,                         // 设置夜间超速参数 -- 川标

    SE_ADAS_CHECK_PARAMS = 0xF0A0,                              // ADAS 检测参数 -- 苏标 - 防止偷煤
    SE_EXTEND_PARAMS = 0xF400,                                  // 扩展参数 -- 苏标 - 防止偷煤
    SE_P9Z_MONITOR_SYSTEM_EVENT_PARAMS = 0xF368,                // P9Z 监测系统事件参数 -- 苏标 - 防止偷煤

    AUDIO_VIDEO_PARAMETER = 0x0075,                             // 音视频参数设置 -- JT T 1078
    AUDIO_VIDEO_CHANNEL_LIST = 0x0076,                          // 音视频通道列表设置 --JT T 1078
    SEPARATE_VIDEO_CHANNEL_PARAMETER = 0x0077,                  // 单独视频通道参数设置 --JT T 1078
    SPECIAL_ALARM_RECORDINH_PARAMETER = 0x0079,                 // 特殊报警录像参数设置 --JT T 1078
    VIDEO_RELATED_ALARM_MASK_WORD = 0x007A,                     // 视频相关报警屏蔽字设置 --JT T 1078
    IMAGE_ANALYSIS_ALARM_PARAMETER = 0x007B,                    // 图像分析报警参数设置
    TERMINAL_SLEEP_WAKE_MODE = 0x007c,                          // 终端休眠唤醒模式   --JT T 1078
    PARAMS_COUNT = 43,                                          // 用于统计当前的enum当中具体有多少个元素(目前还没有更好的方法来统计enum当中元素的个数，所以只能使用这种比较笨的方法)

/*******************平台管控扩展数据结构*************************************************/
    /*平台管控扩展参数id*/
    QUERY_PLATFORM_PARAMS = 0xF100, 	                        // 查询连接的平台和端口
    ADD_PLATFORM_PARAMS = 0xF101,	 	                        // 添加平台
    LOGOUT_PLATFORM_PARAMS = 0xF102,	                        // 注销平台
    QUERY_STATUS = 0xF103,				                        // 状态查询(定位信号、4G信号、远光灯、近光灯、左转灯、右转灯、制动、部标平台、录像、设备锁)
    DOWNLOAD_LOG = 0xF104,				                        // 终端日志下载
    /********************/
    INQUIRE_TERMINAL_DEV_MAC = 0xFF00,                          // 查询终端的MAC地址
    JT_DRIVER_COMPARE_PARAMS_ID = 0xF0E9,                       // 九通 - 驾驶员比对参数设置

    // the following params id are for hei-biao
    HB_DRIVER_IDENTITY_RECOGNIZE_PARAMS_ID = 0xE138,        // 驾驶员身份识别参数
    HB_CAR_MONITOR_PARAMS_ID = 0xE139,                      // 车辆运行监测参数
    HB_DMS_PARAMS_ID = 0xE140,                              // 驾驶员驾驶行为参数
    HB_DEV_MALFUNC_PARAMS_ID = 0xE141                       // 设备失效监测参数

} JT808_PARAMS_ID;

/* 人工确认报警类型定义 */
typedef enum
{

    JT808_ALERT_MANMADE_TYPE_SOS                    = 1 << 0,       /* 1-确认紧急报警 */
    JT808_ALERT_MANMADE_TYPE_PRE_DANGER             = 1 << 1,       /* 1-确认危险预警 */
    JT808_ALERT_MANMADE_TYPE_INOUTAREA              = 1 << 20,      /* 1-确认进出区域报警 */         
    JT808_ALERT_MANMADE_TYPE_INOUTROAD              = 1 << 21,      /* 1-确认进出路线报警 */
    JT808_ALERT_MANMADE_TYPE_ROADFASTORSLOW         = 1 << 22,      /* 1-确认路段行驶时间不足/过长报警 */
    JT808_ALERT_MANMADE_TYPE_ACCILLEGAL             = 1 << 27,      /* 1-确认非法点火报警 */
    JT808_ALERT_MANMADE_TYPE_MOVINGILLEGAL          = 1 << 28,      /* 1-确认非法位移报警 */

} JT808_ALERT_MANMADE_TYPE_BIT;

/* 文本信息标志位定义 */
typedef enum
{

    JT808_TEXT_INFO_FLAG_TYPE                       = 3 << 0,       /* 01:服务  10：紧急  11：通知 */
    JT808_TEXT_INFO_FLAG_MONITOR_SHOW               = 1 << 2,       /* 1-终端显示器显示 */
    JT808_TEXT_INFO_FLAG_TTS_REPORT                 = 1 << 3,       /* 1-终端TTS播报 */
    JT808_TEXT_INFO_FLAG_CONTENT_TYPE               = 1 << 5,       /* 0-中心导航信息  1-CAN故障码信息 */

} JT808_TEXT_INFO_FLAG_BIT;

/* 区域属性 */
typedef enum
{

    JT808_AREA_ATTRIBUTE_USE_TIMEREGION				= 1 << 0,		/* 是否启用起始时间和结束时间判断规则，0-否  1-是 */
    JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT				= 1 << 1,		/* 是否启用限速相关判断规则，0-否  1-是 */
    JT808_AREA_ATTRIBUTE_IN_REPORT2DRIVER			= 1 << 2,		/* 进区域是否报警给驾驶员，0-否  1-是 */
    JT808_AREA_ATTRIBUTE_IN_REPORT2PLATFORM			= 1 << 3,		/* 进区域是否报警给平台，0-否  1-是 */
    JT808_AREA_ATTRIBUTE_OUT_REPORT2DRIVER			= 1 << 4,		/* 出区域是否报警给驾驶员，0-否  1-是 */
    JT808_AREA_ATTRIBUTE_OUT_REPORT2PLATFORM		= 1 << 5,		/* 出区域是否报警给平台，0-否  1-是 */
    JT808_AREA_ATTRIBUTE_LATITUDE_FLAG				= 1 << 6,		/* 0-北纬  1-南纬 */
    JT808_AREA_ATTRIBUTE_LONGITUDE_FLAG				= 1 << 7,		/* 0-东经  1-西经 */
    JT808_AREA_ATTRIBUTE_DOOR_LIMIT					= 1 << 8,		/* 0-允许开门  1-禁止开门 */
    JT808_AREA_ATTRIBUTE_IN_LTE_ONOFF				= 1 << 14,		/* 0-进区域开启通信模块  1-进区域关闭通信模块 */
    JT808_AREA_ATTRIBUTE_IN_GNSS_GATHER				= 1 << 15,		/* 0-进区域不采集GNSS详细定位数据  1-进区域采集GNSS详细定位数据 */

} JT808_AREA_ATTRIBUTE_BIT;

/* 路线属性 */
typedef enum
{

    JT808_ROUTE_ATTRIBUTE_USE_TIMEREGION			= 1 << 0,	    /* 是否启用起始时间和结束时间判断规则，0-否  1-是 */
    JT808_ROUTE_ATTRIBUTE_IN_REPORT2DRIVER			= 1 << 2,		/* 进路线是否报警给驾驶员，0-否  1-是 */
    JT808_ROUTE_ATTRIBUTE_IN_REPORT2PLATFORM		= 1 << 3,	    /* 进路线是否报警给平台，0-否  1-是 */
    JT808_ROUTE_ATTRIBUTE_OUT_REPORT2DRIVER			= 1 << 4,		/* 出路线是否报警给驾驶员，0-否  1-是 */
    JT808_ROUTE_ATTRIBUTE_OUT_REPORT2PLATFORM		= 1 << 5,		/* 出路线是否报警给平台，0-否  1-是 */

} JT808_ROUTE_ATTRIBUTE_BIT;

/* 路段属性 */
typedef enum
{

    JT808_ROAD_ATTRIBUTE_USE_TIMELIMIT              = 1 << 0,       /* 1：启用行驶时间 */
    JT808_ROAD_ATTRIBUTE_USE_SPEEDLIMIT             = 1 << 1,       /* 1：启用限速 */
    JT808_ROAD_ATTRIBUTE_LATITUDE_FLAG              = 1 << 2,       /* 0-北纬  1-南纬 */
    JT808_ROAD_ATTRIBUTE_LONGITUDE_FLAG             = 1 << 3,       /* 0-东经  1-西经 */

} JT808_ROAD_ATTRIBUTE_BIT;

/* 下行数据透传消息类型 */
typedef enum
{

    JT808_MSG_TRANSPARENT_TYPE_GNSS_DETAILDATA = 0x00,              /* GNSS详细定位数据 */
    JT808_MSG_TRANSPARENT_TYPE_ICCARD = 0x0B,                       /* 道路运输IC卡信息 */
    JT808_MSG_TRANSPARENT_TYPE_UART_CH1 = 0x41,                     /* 串口1透传消息 */
    JT808_MSG_TRANSPARENT_TYPE_UART_CH2 = 0x42,                     /* 串口2透传消息 */

} JT808_MSG_TRANSPARENT_TYPE;

#define JT808MSG_ATTR_LENGTH(attr)                      (attr&0x03FF)
#define JT808MSG_ATTR_IS_PACK(attr)                     (attr&0x2000)
#define JT808MSG_ATTR_VERSION(attr)                     (attr&0x4000) // 为0则为2011-2013版本，1-2019版本

#define JT808MSG_BODY_OFFSET(attr)						((JT808MSG_ATTR_VERSION(attr)?1:0) + (JT808MSG_ATTR_IS_PACK(attr)?4:0)  + 7)

typedef struct
{
    
    WORD        mMsgID;                                 /* 消息ID */
    WORD        mMsgAttribute;                          /* 消息体属性 */
    BYTE        mProtocalVer;                           /* 协议版本 */
    BYTE        mPhoneNum[PHONE_NUM_LEN_MAX];           /* 手机终端号BCD码，2013/2011版本为6位 */
    WORD        mMsgSerialNum;                          /* 消息流水号，从0开始循环累加 */

    // 12 如果消息体属性中相关标识位确定消息分包处理，则该项有内容，否则无该项
    WORD        mPacketTotalNum;                        /* 消息总包数:该消息分包后的总包数 */
    WORD        mPacketIndex;                           /* 15 包序号: 从 1 开始 */

} JT808MsgHeader_t;

typedef struct
{
    
    WORD        mPacketTotalNum;                        /* 消息总包数:该消息分包后的总包数 */
    WORD        mPacketIndex;                           /* 15 包序号: 从 1 开始 */

} JT808MsgPackItem_t;

// 每条消息由标识位、 消息头、消息体和校验码组成
// the message construction
// | 标识位 |         消息头       | 消息体 | 检验码 | 标识位 |
// | 0x7E  | MessageHeader[分包项] |  ---  | ----  | 0x7E  |
typedef struct
{

    JT808MsgHeader_t*       pMsgHead;                   /* 消息头 */
    JT808MsgPackItem_t      mMsgPackInto;
    BYTE                    mMsgBody[1024];
    WORD                    mBodyLen;
    
} JT808MsgRaw_t;

/* 终端通用应答 */
typedef struct 
{ 

    WORD                    mResponseSerialNum;         /* 对应平台消息的流水号 */
    WORD                    mMsgID;                     /* 对应平台消息的ID */
    BYTE                    mResult;                    /* 结果：0-成功/确认 1-失败 2-消息有误 3-不支持 @ JT808_TERMIAL_COMMON_RESULT */

} JT808MsgBody_0001_t; 

/* 平台通用应答 */
typedef struct 
{ 

    WORD                    mResponseSerialNum;         /* 对应终端消息的流水号 */
    WORD                    mMsgID;                     /* 对应终端消息的ID */
    BYTE                    mResult;                    /* 结果：0-成功/确认 1-失败 2-消息有误 3-不支持 4-报警处理确认 @ JT808_TERMIAL_COMMON_RESULT */

} JT808MsgBody_8001_t; 

/* 终端心跳 */
typedef struct 
{ 
    // none
} JT808MsgBody_0002_t; 

/* 服务器补传分包请求 */
typedef struct 
{ 

    WORD                    mTargetMsgSerialNum;        /* 原始消息流水号，对应要求补传的原始消息第一包的流水号 */
    BYTE                    mReUploadPacksCount;        /* 重传包总数 */
    WORD*                   pReUploadPackIDs;           /* 重传包总数，序号顺序排列 */

} JT808MsgBody_8003_t; 

/* 终端补传分包请求 */
typedef struct 
{ 

    WORD                    mTargetMsgSerialNum;        /* 原始消息流水号，对应要求补传的原始消息第一包的流水号 */
    WORD                    mReUploadPacksCount;        /* 重传包总数 */
    WORD*                   pReUploadPackIDs;           /* 重传包总数，序号顺序排列 */

} JT808MsgBody_0005_t; 

/* 终端注册 */
typedef struct 
{ 

    WORD                    mProvincialId;                      /* 省域 ID : 省域 ID 采用 GB/T 2260 中规定的行政区划代码六位中前两位 44 */
    WORD                    mCityCountyId;                      /* 市县域 ID: 市县域 ID 采用 GB/T 2260 中规定的行政区划代码六位中后四位 300 */
    BYTE                    mProducerId[PRODUCERID_LEN_MAX];    /* 制造商 ID    808-2019-8.8 */
    BYTE                    mProductModel[PRODUCTMODEL_LEN_MAX];/* 终端型号   808-2019-8.8 */
    BYTE                    mTerminalId[TERMINALID_LEN_MAX];    /* 终端 ID   808-2019-8.8 */
    BYTE                    mLicensePlateColor;                 /* 车牌颜色(未上牌车辆填0) 按照 JT/T415-2006 的 5.4.12 */
    BYTE                    mLinceseSize;
    char                    mLicensePlate[DEFAULTLENGTH];       /* 车牌颜色为 0 时，表示车辆 VIN； 否则，表示公安交通管理部门颁发的机动车号牌 */

} JT808MsgBody_0100_t; 

/* 注册应答 */
typedef struct 
{ 

    WORD                    mResponseSerialNum;                 /* 应答流水号: 对应的终端注册消息的流水号 */
    BYTE                    mResult;                            /* 结果：0-成功 1-车辆已被注册 2-数据库中无该车辆 3-终端已被注册 4-数据库中无该终端 @ JT808_TERMINAL_REGISTER_RESULT */
	char					mAuthenticationCodeLen;				/* 非协议字段 */
	char                    mAuthenticationCode[64];            /* 鉴权码: 只有在成功后才有该字段,我们需要使用鉴权码进行之后的操作 */

} JT808MsgBody_8100_t; 

/* 终端注销 */
typedef struct 
{ 
    // none
} JT808MsgBody_0003_t; 

/* 查询服务器时间请求 */
typedef struct 
{ 
//none
} JT808MsgBody_0004_t; 

/* 查询服务器时间应答 */
typedef struct 
{ 

    BYTE                    mTimeData[6];                       /* BCD 时间 */

} JT808MsgBody_8004_t; 

/* 终端鉴权 */
typedef struct 
{ 

    BYTE                    mAuthenticationCodeLen;             /* 终端鉴权码长度 */
    char                    mAuthenticationCode[64];            /* 鉴权码 */
    BYTE                    mIMEINumber[15];                    /* IMEI号 */
    BYTE                    mSoftVersion[20];                   /* 软件版本号 */

} JT808MsgBody_0102_t; 

/* 设置终端参数 */
typedef struct
{

	

} JT808MsgParam_0075_t;
typedef struct
{

	

} JT808MsgParam_0076_t;
typedef struct
{

	

} JT808MsgParam_0077_t;
typedef struct
{

	

} JT808MsgParam_0079_t;
typedef struct
{

	

} JT808MsgParam_007B_t;
typedef struct
{

	

} JT808MsgParam_007C_t;
typedef struct
{

	

} JT808MsgParam_F364_t;
typedef struct
{

	

} JT808MsgParam_F365_t;
typedef struct
{

	

} JT808MsgParam_F366_t;
typedef struct
{

	

} JT808MsgParam_F367_t;

typedef struct
{
    
    DWORD                   mParamID;                           /* 参数ID，详见 @ JT808_PARAMS_ID */
    BYTE                    mParamDataLen;                      /* 参数长度 */
    BYTE            		mDataType;							/* 非协议字段，数据类型 01 BYTE, 02 WORD, 03 DWORD, 04 BCD, 05 STRING, 06 STRUCT */
//    BYTE                    mParamData[0xff];                   /* 参数值 */
	DWORD					mParamData;							/* BYTE、WORD、DWORD类型代表数值，BCD、STRING、STRUCT类型代表指针 */

} JT808MsgBody_8103_Item_t;
typedef struct 
{ 

    BYTE                    mParamItemCount;                    /* 参数项个数 */
    JT808MsgBody_8103_Item_t* pParamList;                     	/* 参数项列表 */

} JT808MsgBody_8103_t; 

/* 查询终端参数 */
typedef struct 
{ 
    // none
} JT808MsgBody_8104_t; 

/* 查询终端参数应答 */
typedef struct 
{

    WORD                    mResponseSerialNum;                 /* 应答流水号: 对应终端参数查询消息的流水号 */
    BYTE                    mParamItemCount;                    /* 参数项个数 */
    queue_list_t   			pParamList;                     /* 参数项列表 */

} JT808MsgBody_0104_t; 

/* 终端控制 */
typedef struct 
{ 

    BYTE                    mTerminalCtrlCmd;                   /* 终端控制命令字 @ JT808_TERMINAL_CTRL_CMD */
    WORD                    mTerminalCtrlParamLen;              /* 控制参数长度 */
    char*                   pTerminalCtrlParam;                 /* 控制参数 */

} JT808MsgBody_8105_t; 

/* 查询指定终端参数，采用0104应答 */
typedef struct 
{ 
    
    BYTE                    mParamItemCount;                    /* 参数项个数 */
    DWORD                   mParamIDList[255];                  /* 参数项ID列表 */

} JT808MsgBody_8106_t; 

/* 查询终端属性 */
typedef struct 
{ 
//none
} JT808MsgBody_8107_t; 

/* 查询终端属性应答 */
typedef struct 
{ 

    WORD					mTerminalType; 						/* 终端类型 */
    BYTE					mProducerId[PRODUCERID_LEN_MAX]; 	/* 制造商 ID */
    BYTE					mTerminalModel[PRODUCTMODEL_LEN_MAX];/* 终端型号20 个字节，此终端型号由制造商自行定义，位数不足时，后补“0X00” */
    BYTE					mTerminalId[TERMINALID_LEN_MAX];	/* 终端 ID: 7 个字节，由大写字母和数字组成，此终端 ID 由制造商自行定义，位数不足时，后补“0X00”。 */
    BYTE					mTerminalSIMICCIDCode[10]; 			/* 终端 SIM 卡 ICCID 号 */
    BYTE					mTerminalHardwareVersionLen; 		/* 终端硬件版本号长度 */
    char					mTerminalHardwareVersion[32]; 		/* 终端硬件版本号 */
    BYTE					mTerminalFirmwareVersionLen; 		/* 终端固件版本号长度 */
    char					mTerminalFirmwareVersion[32]; 		/* 终端固件版本号 */
    BYTE					mGNSSModuleAttribute; 				/* 模块属性 */
    BYTE					mCommunicationModuleAttibute; 		/* 通信模块属性 */

} JT808MsgBody_0107_t; 

/* 下发终端升级包 */
typedef struct 
{ 

    BYTE                    mUpgradeType;                       /* 升级终端类型，0-终端 12-道路运输证IC卡读卡器 52-卫星定位模块 */
    BYTE                    mProducerID[PRODUCERID_LEN_MAX];    /* 制造商ID */
    BYTE					mTerminalFirmwareVersionLen; 		/* 终端固件版本号长度 */
    char					mTerminalFirmwareVersion[255]; 		/* 终端固件版本号 */
    DWORD                   mTerminalFirmwareDataLen;
    char*                   pTerminalFirmwareData;

} JT808MsgBody_8108_t; 

/* 终端升级结果应答 */
typedef struct 
{ 

    BYTE                    mUpgradeType;                       /* 升级终端类型，0-终端 12-道路运输证IC卡读卡器 52-卫星定位模块 */
    BYTE                    mUpgradeResult;                     /* 升级结果：0-成功 1-失败 2-取消 */

} JT808MsgBody_0108_t; 

/* 位置汇报附加信息 */
typedef struct
{

    BYTE                    mExtraInfoID;                          /* 附加信息ID @ JT808_EXTRA_MSG_ID */
    BYTE                    mExtraInfoLen;                         /* 附加信息长度 */
    BYTE                    mExtraInfoData[0xff];                  /* 附加信息内容 */
    
} JT808MsgBody_0200Ex_t;
/* 位置信息汇报 */
typedef struct 
{ 

    DWORD                   mAlertFlag;                         /* 报警标志，位定义见808-2019 表25 @ JT808_ALERT_FALG_BIT */
    DWORD                   mStateFlag;                         /* 状态标志，位定义见808-2019 表24 @ JT808_TERMINAL_STATE_BIT */
    DWORD                   mLatitude;                          /* 纬度，纬度值*10^6 */
    DWORD                   mLongitude;                         /* 经度，经度值*10^6 */
    WORD                    mAltitude;                          /* 海拔高度，单位m */
    WORD                    mSpeed;                             /* 当前速度，单位1/10Km/h，实际速度Km/h值*10 */
    WORD                    mDirection;                         /* 方向，0-359，正北为0，顺时针 */
    BYTE                    mDateTime[6];                       /* BCD时间，YY-MM-DD-hh-mm-ss，GMT+8时区 */

    // 附加信息
    BYTE                    mExtraInfoCount;                   /* 附加信息个数 */
//    JT808MsgBody_0200Ex_t*  pExtraInfoList;                    /* 附加信息列表 */
	queue_list_t			mExtraInfoList;

} JT808MsgBody_0200_t; 

/* 位置信息查询 */
typedef struct 
{ 
//none
} JT808MsgBody_8201_t; 

/* 位置信息查询应答 */
typedef struct 
{ 

    WORD                    mResponseSerialNum;                 /* 应答流水号: 对应位置信息查询消息的流水号 */
    JT808MsgBody_0200_t     mLocationReport;                    /* 位置信息汇报 */

} JT808MsgBody_0201_t; 

/* 临时位置跟踪控制 */
typedef struct 
{ 

    WORD                    mTimeIntervel;                      /* 单位s，时间间隔为0时停止跟踪，停止跟踪无需带后续字段 */
    DWORD                   mValidDate;                         /* 单位s，有效期时间内，根据时间间隔发送位置汇报 */

} JT808MsgBody_8202_t; 

/* 人工确认报警消息 */
typedef struct 
{ 

    WORD                mAlertMsgSerialNumber;                  /* 需人工确认的报警消息流水号，0表示该报警类型所有消息 */
    DWORD               mAlertType;                             /* 需人工确认报警类型 @ JT808_ALERT_MANMADE_TYPE_BIT */

} JT808MsgBody_8203_t; 

/* 链路检测 */
typedef struct 
{ 
//none
} JT808MsgBody_8204_t; 

/* 文本信息下发 */
typedef struct 
{ 

    BYTE                mTextFlag;                              /* 文本信息标志，位定义 @ JT808_TEXT_INFO_FLAG_BIT */
    BYTE                mTextType;                              /* 文本类型：1-通知  2-服务 */
    char                mTextContent[1024];                     /* GBK编码 */                       

} JT808MsgBody_8300_t;

/* 电话回拨 */
typedef struct 
{ 

    BYTE                    mFlag;                              /* 标志：0-普通通话  1-监听 */
    char                    mPhoneNum[20];                      /* 电话号码 */

} JT808MsgBody_8400_t; 

/* 设置电话本 */
typedef struct
{
    
    BYTE                    mFlag;                              /* 标志：1-呼入  2-呼出  3-呼入呼出 */
    BYTE                    mPhoneNumLen;                       /* 电话号码长度 */
    char                    mPhoneNum[20];                      /* 电话号码 */
    BYTE                    mContactNameLen;                    /* 联系人名字长度 */
    char                    mContactName[255];                  /* 联系人名字，GBK编码 */

} JT808MsgBody_8401_Item_t;
typedef struct 
{ 

    BYTE                    mSettingType;                       /* 设置类型：0-删除电话本所有联系人  1-更新电话本（删除原有）  2-追加电话本  3-修改电话本（以联系人为索引） */
    BYTE                    mNumberCount;                       /* 联系人总数 */
    JT808MsgBody_8401_Item_t* pNumberItems;                     /* 联系人项 */

} JT808MsgBody_8401_t; 

/* 车辆控制 */
typedef struct
{
    
    WORD                    mControlTypeID;                     /* 控制类型ID：0001-车门  0002~8000-标准修订预留  F001-FFFF-厂家自定义类型 */
    BYTE                    mDatas[1024];                       /* 0001时为BYTE，0-车门锁闭   1-车门开启 */

} JT808MsgBody_8500_Item_t;
typedef struct 
{ 

    WORD                    mControlCount;                      /* 控制项数量 */
    JT808MsgBody_8500_Item_t* pControlItems;

} JT808MsgBody_8500_t; 

/* 车辆控制应答 */
typedef struct 
{ 

    WORD                    mResponseSerialNum;                 /* 应答流水号: 对应车辆控制消息的流水号 */
    JT808MsgBody_0200_t     mLocationReport;                    /* 位置信息汇报，根据对应状态位判断控制是否成功 */

} JT808MsgBody_0500_t; 

/* 设置圆形区域 */
typedef struct
{

    DWORD                   mAreaID;                            /* 区域ID */
    WORD                    mAreaAttribute;                     /* 区域属性 @ JT808_AREA_ATTRIBUTE_BIT */
    DWORD                   mAreaRoundotLatitude;               /* 区域圆点纬度，单位同位置信息中纬度 */
    DWORD                   mAreaRoundotLongitude;              /* 区域圆点经度，单位同位置信息中经度 */
    DWORD                   mAreaRadius;                        /* 区域半径，单位m */
    BYTE                    mStartTime[6];                      /* BCD时间，若区域属性0位为0则无该字段 */
    BYTE                    mStopTime[6];                       /* BCD时间，若区域属性0位为0则无该字段 */
    WORD                    mSpeedLimit;                        /* 区域最高速度，单位为KM/H，若区域属性1位为0则没有该字段 */
    BYTE                    mOverspeedContinueTime;             /* 超速持续时间，单位为s，若区域属性1位为0则没有该字段 */
    WORD                    mSpeedLimitNight;                   /* 夜间区域最高速度，单位为KM/H，若区域属性1位为0则没有该字段 */
    WORD                    mAreaNameLen;                       /* 区域名称长度 */
    char                    mAreaName[1024];                    /* 区域名 */
    
} JT808_Circle_Area_t;
typedef struct 
{ 

    BYTE                    mSettingOption;                     /* 设置操作方式：0-更新区域  1-追加区域  2-修改区域 */
    BYTE                    mAreaCount;                         /* 设置区域项总数 */
    JT808_Circle_Area_t*    pCircleAreaItems;                   /* 区域项列表 */

} JT808MsgBody_8600_t; 

/* 删除圆形区域 */
typedef struct 
{ 

    BYTE                    mDeleteAreaCount;                   /* 删除的区域个数，本条消息中数量不超过125，多于125建议分多条消息处理， 0为删除所有圆形区域 */
    DWORD                   mDeleteAreaIDs[125];                /* 区域ID列表 */

} JT808MsgBody_8601_t; 

// 设置矩形区域
typedef struct
{

    DWORD                   mAreaID;                            /* 区域ID */
    WORD                    mAreaAttribute;                     /* 区域属性 @ JT808_AREA_ATTRIBUTE_BIT */
    DWORD                   mAreaLeftTopLatitude;               /* 左上点纬度，单位同位置信息中纬度 */
    DWORD                   mAreaLeftTopLongitude;              /* 左上点经度，单位同位置信息中经度 */
    DWORD                   mAreaRightBottomLatitude;           /* 右下点纬度，单位同位置信息中纬度 */
    DWORD                   mAreaRightBottomLongitude;          /* 右下点经度，单位同位置信息中经度 */
    BYTE                    mStartTime[6];                      /* BCD时间，若区域属性0位为0则无该字段 */
    BYTE                    mStopTime[6];                       /* BCD时间，若区域属性0位为0则无该字段 */
    WORD                    mSpeedLimit;                        /* 区域最高速度，单位为KM/H，若区域属性1位为0则没有该字段 */
    BYTE                    mOverspeedContinueTime;             /* 超速持续时间，单位为s，若区域属性1位为0则没有该字段 */
    WORD                    mSpeedLimitNight;                   /* 夜间区域最高速度，单位为KM/H，若区域属性1位为0则没有该字段 */
    WORD                    mAreaNameLen;                       /* 区域名称长度 */
    char                    mAreaName[1024];                    /* 区域名 */
    
} JT808_Rectangle_Area_t;
typedef struct 
{ 

    BYTE                    mSettingOption;                     /* 设置操作方式：0-更新区域  1-追加区域  2-修改区域 */
    BYTE                    mAreaCount;                         /* 设置区域项总数 */
    JT808_Rectangle_Area_t*    pRectangleAreaItems;             /* 区域项列表 */

} JT808MsgBody_8602_t; 

/* 删除矩形区域 */
typedef struct 
{ 

    BYTE                    mDeleteAreaCount;                   /* 删除的区域个数，本条消息中数量不超过125，多于125建议分多条消息处理， 0为删除所有矩形区域 */
    DWORD                   mDeleteAreaIDs[125];                /* 区域ID列表 */

} JT808MsgBody_8603_t; 

/* 设置多边形区域 */
typedef struct
{
    
    DWORD                   mLatitude;                          /* 顶点纬度，单位同位置信息中纬度 */
    DWORD                   mLongitude;                         /* 顶点点经度，单位同位置信息中经度 */

} JT808_Area_Point_t;
typedef struct 
{ 

    DWORD                   mAreaID;                            /* 区域ID */
    WORD                    mAreaAttribute;                     /* 区域属性 @ JT808_AREA_ATTRIBUTE_BIT */
    BYTE                    mStartTime[6];                      /* BCD时间，若区域属性0位为0则无该字段 */
    BYTE                    mStopTime[6];                       /* BCD时间，若区域属性0位为0则无该字段 */
    WORD                    mSpeedLimit;                        /* 区域最高速度，单位为KM/H，若区域属性1位为0则没有该字段 */
    BYTE                    mOverspeedContinueTime;             /* 超速持续时间，单位为s，若区域属性1位为0则没有该字段 */
    WORD                    mPointCount;                        /* 顶点个数 */
    JT808_Area_Point_t*     pPoints;                            /* 区域顶点列表 */
    WORD                    mSpeedLimitNight;                   /* 夜间区域最高速度，单位为KM/H，若区域属性1位为0则没有该字段 */
    WORD                    mAreaNameLen;                       /* 区域名称长度 */
    char                    mAreaName[1024];                    /* 区域名 */

} JT808MsgBody_8604_t; 

/* 删除多边形区域 */
typedef struct 
{ 

    BYTE                    mDeleteAreaCount;                   /* 删除的区域个数，本条消息中数量不超过125，多于125建议分多条消息处理， 0为删除所有矩形区域 */
    DWORD                   mDeleteAreaIDs[125];                /* 区域ID列表 */

} JT808MsgBody_8605_t; 

/* 设置路线 */
typedef struct
{
    
    DWORD                   mInflectionPointID;                 /* 拐点ID */
    DWORD                   mRoadID;                            /* 路段ID */
    DWORD                   mInflectPointLatitude;              /* 拐点纬度，单位同位置信息中纬度 */
    DWORD                   mInflectPointLongitude;             /* 拐点经度，单位同位置信息中纬度 */
    BYTE                    mRoadWidth;                         /* 路段宽度，单位m，路段指的是该拐点到下一个拐点 */
    BYTE                    mRoadAttribute;                     /* 路段属性 @ JT808_ROAD_ATTRIBUTE_BIT */
    WORD                    mRoadLongTimeThreashold;            /* 路段行驶时间过长阈值，单位s，若路段属性0位为0则没有该字段 */
    WORD                    mRoadInsufficientThreashold;        /* 路段行驶时间不足阈值，单位s，若路段属性0位为0则没有该字段 */
    WORD                    mSpeedLimit;                        /* 区域最高速度，单位为KM/H，若区域属性1位为0则没有该字段 */
    BYTE                    mOverspeedContinueTime;             /* 超速持续时间，单位为s，若区域属性1位为0则没有该字段 */
    WORD                    mSpeedLimitNight;                   /* 夜间区域最高速度，单位为KM/H，若区域属性1位为0则没有该字段 */

} JT808_InflectionPoint_t;
typedef struct 
{ 

    DWORD                   mRouteID;                            /* 路线ID */
    WORD                    mRouteAttribute;                     /* 路线属性 @ JT808_ROUTE_ATTRIBUTE_BIT */
    BYTE                    mStartTime[6];                      /* BCD时间，若区域属性0位为0则无该字段 */
    BYTE                    mStopTime[6];                       /* BCD时间，若区域属性0位为0则无该字段 */
    WORD                    mInflectionPointCount;              /* 路线总拐点数 */
    JT808_InflectionPoint_t* pInflectionPoints;                 /* 路线拐点项列表 */
    WORD                    mAreaNameLen;                       /* 区域名称长度 */
    char                    mAreaName[1024];                    /* 区域名 */

} JT808MsgBody_8606_t; 

/* 删除路线 */
typedef struct 
{ 

    BYTE                    mDeleteRouteCount;                  /* 删除的路线个数，本条消息中数量不超过125，多于125建议分多条消息处理， 0为删除所有路线 */
    DWORD                   mDeleteRouteIDs[125];               /* 路线ID列表 */

} JT808MsgBody_8607_t; 

/* 查询区域或路线数据 */
typedef struct 
{ 

    BYTE                    mQueryType;                         /* 查询类型：1-查询圆形区域  2-查询矩形区域  3-查询多边形区域  4-查询路线 */
    DWORD                   mQueryIDCount;                      /* 查询ID个数，0表示查询当前类型所有数据 */
    DWORD*                  pQueryIDs;                          /* 查询的ID列表 */

} JT808MsgBody_8608_t; 

/* 查询区域或路线数据应答 */
typedef struct 
{ 

    BYTE                    mQueryType;                         /* 查询类型：1-查询圆形区域  2-查询矩形区域  3-查询多边形区域  4-查询路线 */
    DWORD                   mQueryDataCount;                    /* 查询到的区域或线路数据数量 */
	DWORD					mDataLen;
	void*                   mDatas;                             /* 数据结构格式与查询类型数据结果对应 */

} JT808MsgBody_0608_t; 

/* 行驶记录数据采集命令 */
typedef struct 
{ 

    BYTE                    mCmdByte;                           /* 命令字，命令字列表符合GB/T 19056中相关要求 */
    ////????????????????????????????????                        /* 数据块，数据块内容格式应符合GB/T 19056要求的完整数据包，可为空 */

} JT808MsgBody_8700_t; 

/* 行驶记录数据上传 */
typedef struct 
{ 

    WORD                    mResponseSerialNum;                 /* 形式记录数据采集命令流水号 */
    BYTE                    mCmdByte;                           /* 命令字，命令字列表符合GB/T 19056中相关要求 */
    ////????????????????????????????????                        /* 数据块，数据块内容格式应符合GB/T 19056要求的完整数据包，可为空 */

} JT808MsgBody_0700_t; 

/* 行驶记录参数下传命令 */
typedef struct 
{ 

    BYTE                    mCmdByte;                           /* 命令字，命令字列表符合GB/T 19056中相关要求 */
    ////????????????????????????????????                        /* 数据块，数据块内容格式应符合GB/T 19056要求的完整数据包，可为空 */

} JT808MsgBody_8701_t; 

/* 电子运单上报 */
typedef struct 
{ 

    DWORD                   mElectronicWaybillLen;              /* 电子运单长度 */
    char*                   pElectronicWaybillContent;          /* 电子运单内容 */

} JT808MsgBody_0701_t; 

/* 上报驾驶员身份信息请求 */
typedef struct 
{ 
//none
} JT808MsgBody_8702_t; 

/* 驾驶员身份信息采集上报，8702应答，或终端从业资格证IC卡插入或拔出后触发此命令 */
typedef struct 
{ 

    BYTE                    mICCardStatus;                      /* 1-从业资格证插入  2-从业资格证拔出 */
    BYTE                    mICCardOptTime[6];                  /* 插拔卡时间，以下字段在插卡动作有效并填充 */
    BYTE                    mReadICCardResult;                  /* 读卡结果，0-读卡成功  1-读卡失败，密钥认证未通过  2-读卡失败，卡已被锁定  3-读卡失败，卡被拔出  4-读卡失败，数据校验错误。一下字段在读卡成功有效 */
    BYTE                    mDriverNameLen;                     /* 驾驶员名字长度 */
    char                    mDriverName[32];                    /* 驾驶员名字 */
    BYTE                    mCertificateNum[20];                /* 从业资格证编码 */
    BYTE                    mIssuingAthorNameLen;               /* 发证机构名字长度 */
    char                    mIssuingAthorName[64];              /* 发证机构名字 */
    BYTE                    mCertificationValidity[4];          /* 身份证有效期，BCD，YYYYMMDD */
    BYTE                    mDriverIdentity[20];                /* 身份证号 */

} JT808MsgBody_0702_t;  

/* 定位数据批量上传 */
typedef struct
{
    
    WORD                    mDataLen;                           /* 数据长度 */
//    JT808MsgBody_0200_t     mData;                              /* 位置信息 */
	BYTE					mData[255];							/* 位置信息 */

} JT808MsgBody_0704_Item_t;

typedef struct 
{ 

    WORD                    mItemsCount;                        /* 数据项个数 */
    BYTE                    mItemsType;                         /* 位置数据类型，0-正常位置批量汇报  1-盲区补报 */
    JT808MsgBody_0704_Item_t* pItemsList;                       /* 数据项列表 */

} JT808MsgBody_0704_t; 

/* CAN总线数据上传 */
typedef struct
{
    
    DWORD                   mCANID;                             /* bit31表示CAN通道号，0-CAN1，1-CAN2 */
                                                                /* bit30表示帧类型，0-标准帧，1-拓展帧 */
                                                                /* bit29表示数据采集方式，0-原始数据，1-采集区间的平均值 */
                                                                /* bit28-bit0表示CAN总线ID */
    BYTE                    mCANData[8];                        /* CAN数据 */

} JT808MsgBody_0705_Item_t;
typedef struct 
{ 

    WORD                    mItemsCount;                        /* 数据项个数 */
    BYTE                    mCANRecieveStartTime[5];            /* 第一条CAN总线数据的接收时间，hh-mm-ss-msms */
    JT808MsgBody_0705_Item_t* pItemsList;                       /* 数据项列表 */

} JT808MsgBody_0705_t; 

/* 多媒体事件信息上传 */
typedef struct 
{ 

    DWORD                   mMultiMediaDataID;                  /* 多媒体数据ID */
    BYTE                    mMultiMediaDataType;                /* 多媒体类型，0-图像  1-音频  2-视频 */
    BYTE                    mMultiMediaDataEncodeType;          /* 多媒体格式编码，0-JPEG  1-TIF  2-MP3  3-WAV  4-WMV */
    BYTE                    mEventType;                         /* 事件项编码，0-平台下发指令 */
                                                                /* 1-定时动作  2-抢劫报警触发  3-碰撞侧翻报警触发 */
                                                                /* 4-门开拍照  5-门关拍照  6-车门由开变关，车速从小于20km到超过20km拍照  7-定距拍照 */
    BYTE                    mChannelID;                         /* 通道号 */

} JT808MsgBody_0800_t; 

/* 多媒体数据上传 */
typedef struct 
{ 

    DWORD                   mMultiMediaDataID;                  /* 多媒体数据ID */
    BYTE                    mMultiMediaDataType;                /* 多媒体类型，0-图像  1-音频  2-视频 */
    BYTE                    mMultiMediaDataEncodeType;          /* 多媒体格式编码，0-JPEG  1-TIF  2-MP3  3-WAV  4-WMV */
    BYTE                    mEventType;                         /* 事件项编码，0-平台下发指令 */
                                                                /* 1-定时动作  2-抢劫报警触发  3-碰撞侧翻报警触发 */
                                                                /* 4-门开拍照  5-门关拍照  6-车门由开变关，车速从小于20km到超过20km拍照  7-定距拍照 */
    BYTE                    mChannelID;                         /* 通道号 */
    BYTE                    m0200BaseInfo[28];                  /* 多媒体事件时的，0200基本位置信息 */
    char*                   pMultiMediaDatas;                   /* 多媒体数据包 */

} JT808MsgBody_0801_t; 

/* 多媒体数据上传应答 */
typedef struct 
{ 

    DWORD                   mMultiMediaDataID;                  /* 多媒体数据ID，若传成功，则没有后续字段 */
    BYTE                    mReuploadPackCount;                 /* 重传包个数，不超过125 */
    WORD                    mReuploadPackIdxs[125];             /* 重传包ID列表 */

} JT808MsgBody_8800_t; 

/* 摄像头立即拍摄命令 */
typedef struct 
{ 

    BYTE                    mCameraChannelID;                   /* 摄像头通道号 */
    WORD                    mCaptureCommand;                    /* 拍摄命令，0-停止拍摄  0xFFFF-录像  其它-拍照张数 */
    WORD                    mCaptureIntervel;                   /* 拍摄间隔/录像时间 */
    BYTE                    mStoreFlag;                         /* 保存标志，0-实时上传  1-保存 */
    BYTE                    mResolution;                        /* 分辨率，详见 808-2019 表74 */
    BYTE                    mQuatity;                           /* 拍摄质量，1-10，1损失小，10压缩最大 */
    BYTE                    mBrightness;                        /* 亮度，0-255 */
    BYTE                    mConstraction;                      /* 对比度，0-127 */
    BYTE                    mSaturation;                        /* 饱和度，0-127 */
    BYTE                    mChrominance;                       /* 色度，0-255 */

} JT808MsgBody_8801_t; 

/* 摄像头立即拍摄命令应答 */
typedef struct 
{ 

    WORD                    mResponseSerialNum;                 /* 立即拍摄命令流水号 */
    BYTE                    mResult;                            /* 执行结果，0-成功  1-失败  2-通道不支持   一下字段在0有效 */
    WORD                    mIDCount;                           /* ID个数 */
    DWORD*                  pIDList;                            /* ID列表 */

} JT808MsgBody_0805_t; 

/* 存储多媒体数据检索 */
typedef struct 
{ 

    BYTE                    mMultiMediaDataType;                /* 多媒体类型，0-图像  1-音频  2-视频 */
    BYTE                    mChannelID;                         /* 通道号，0表示检索此类型的所有通道 */
    BYTE                    mEventType;                         /* 事件项编码，0-平台下发指令 */
                                                                /* 1-定时动作  2-抢劫报警触发  3-碰撞侧翻报警触发 */
                                                                /* 4-门开拍照  5-门关拍照  6-车门由开变关，车速从小于20km到超过20km拍照  7-定距拍照 */
    BYTE                    mStartTime[6];                      /* 起始时间，BCD，两个时间字段全0，表示不按时间区间检索 */
    BYTE                    mEndTime[6];                        /* 截至时间，BCD */

} JT808MsgBody_8802_t; 

/* 存储多媒体数据检索应答 */
typedef struct
{
    
    DWORD                   mMultiMediaDataID;                  /* 多媒体数据ID */
    BYTE                    mMultiMediaDataType;                /* 多媒体类型，0-图像  1-音频  2-视频 */
    BYTE                    mChannelID;                         /* 通道号 */
    BYTE                    mEventType;                         /* 事件项编码，0-平台下发指令 */
                                                                /* 1-定时动作  2-抢劫报警触发  3-碰撞侧翻报警触发 */
                                                                /* 4-门开拍照  5-门关拍照  6-车门由开变关，车速从小于20km到超过20km拍照  7-定距拍照 */
    BYTE     				mLocationData[28];                  /* 位置信息，拍摄或录制的起始时刻的汇报消息 */

} JT808MsgBody_0802_Item_t;
typedef struct 
{ 

    WORD                    mResponseSerialNum;
    WORD                    mItemsCount;
	DWORD					mDataLen;
	BYTE*					pItemDatas;

} JT808MsgBody_0802_t; 

/* 存储多媒体数据上传命令 */
typedef struct 
{ 

    BYTE                    mMultiMediaDataType;                /* 多媒体类型，0-图像  1-音频  2-视频 */
    BYTE                    mChannelID;                         /* 通道号 */
    BYTE                    mEventType;                         /* 事件项编码，0-平台下发指令 */
                                                                /* 1-定时动作  2-抢劫报警触发  3-碰撞侧翻报警触发 */
                                                                /* 4-门开拍照  5-门关拍照  6-车门由开变关，车速从小于20km到超过20km拍照  7-定距拍照 */
    BYTE                    mStartTime[6];                      /* 起始时间，BCD，两个时间字段全0，表示不按时间区间检索 */
    BYTE                    mEndTime[6];                        /* 截至时间，BCD */
    BYTE                    mDeleteFlag;                        /* 删除标志，0-保留  1-删除 */

} JT808MsgBody_8803_t; 

/* 录音开始命令 */
typedef struct 
{ 

    BYTE                    mAudioRecordOpt;                    /* 录音控制字，0-停止  1-开始录制 */
    WORD                    mDuration;                          /* 录制时长，单位s，0-一直录制 */
    BYTE                    mStoreFlag;                         /* 保存标志，0-实时上传  1-保存 */
    BYTE                    mSampleRate;                        /* 音频采样率，0-8K  1-11K  2-23K  3-32K */

} JT808MsgBody_8804_t; 

/* 单条存储多媒体数据检索上传命令 */
typedef struct 
{ 

    DWORD                   mMultiMediaDataID;                  /* 多媒体数据ID */
    BYTE                    mDeleteFlag;                        /* 删除标志，0-保留  1-删除 */

} JT808MsgBody_8805_t; 

/* 数据下行透传 */
typedef struct 
{ 

    BYTE                    mDataType;                          /* 透传消息类型 @ JT808_MSG_TRANSPARENT_TYPE */
    WORD                    mDataLen;                           /* 非协议字段 */
    BYTE*                   mDatas;                             /* 消息内容 */

} JT808MsgBody_8900_t; 

/* 数据上行透传 */
typedef struct 
{ 

    BYTE                    mDataType;                          /* 透传消息类型 @ JT808_MSG_TRANSPARENT_TYPE */
    WORD                    mDataLen;                           /* 非协议字段 */
    BYTE*                   mDatas;                             /* 消息内容 */

} JT808MsgBody_0900_t; 

/* 数据压缩上报 */
typedef struct 
{ 

    DWORD                   mMsgBodyLen;                        /* 压缩消息长度 */
    BYTE*                   mMsgBodyDatas;                      /* 经过GZIP压缩后的需要压缩上报的消息体 */

} JT808MsgBody_0901_t; 

/* 平台RSA公钥 */
typedef struct 
{ 

    DWORD                   mRSAe;                              /* 平台RSA公钥中{e,n}中的e */
    BYTE                    mRSAn[128];                         /* 平台RSA公钥中{e,n}中的n */

} JT808MsgBody_8A00_t; 

/* 终端RSA公钥 */
typedef struct 
{ 

    DWORD                   mRSAe;                              /* 终端RSA公钥中{e,n}中的e */
    BYTE                    mRSAn[128];                         /* 终端RSA公钥中{e,n}中的n */

} JT808MsgBody_0A00_t; 

///////////// 808-2013
/* 事件设置 808-2013 8.24 */
typedef struct
{
    
    BYTE                    mEventID;                           /* 事件ID，若有相同事件ID则覆盖 */
    BYTE                    mEventContentLen;                   /* 事件内容长度 */
    char                    mEventContent[0xff];                /* 事件内容，GBK编码 */

} JT808MsgBody_8301_Item_t;
typedef struct 
{ 

    BYTE                    mSettingOpt;                        /* 设置类型，0-删除所有事件，无后续字段  1-更新事件  2-追加事件  3-修改事件  4-删除特定几项事件，无事件内容 */
    BYTE                    mSettingCount;                      /* 设置总数 */
    JT808MsgBody_8301_Item_t* mSettingEvents;                   /* 事件项列表 */

} JT808MsgBody_8301_t; 

/* 事件报告 */
typedef struct 
{ 

    BYTE                    mEventID;                           /* 事件ID */

} JT808MsgBody_0301_t; 

/* 提问下发 */
typedef struct
{
    
    BYTE                    mAnswerID;                          /* 答案ID */
    WORD                    mAnswerContentLen;                  /* 答案内容长度 */
    char*                   pAnswerContent;                     /* 答案内容，经GBK编码 */

} JT808MsgBody_8302_Item_t;
typedef struct 
{ 

    BYTE                    mQuestionFlag;                      /* 问题标志位 bit0-紧急  bit3-终端TTS播报  bit4-广告屏显示 */
    BYTE                    mQuestionContentLen;                /* 问题内容长度 */
    char                    mQuestionContent[0xff];             /* 问题内容，经GBK编码 */

} JT808MsgBody_8302_t; 

/* 提问应答 */
typedef struct 
{ 

    WORD                    mResponseSerialNum;                 /* 提问下发消息流水号 */
    BYTE                    mAnswerID;                          /* 答案ID */

} JT808MsgBody_0302_t; 

/* 信息点播菜单设置 */
typedef struct 
{ 

//

} JT808MsgBody_8303_t; 

/* 信息点播/取消 */
typedef struct 
{ 

//

} JT808MsgBody_0303_t; 

/* 信息服务 */
typedef struct 
{ 

//

} JT808MsgBody_8304_t; 
///////////// 808-2013

///////////// 808拓展  1078
/* 查询终端音视频属性 */
typedef struct 
{ 
} JT808MsgBody_9003_t; 

/* 终端上传音视频属性 */
typedef struct 
{ 

	BYTE					mAudioEncType;						/* 输入音频编码类型，见JT/T 1078-2016 表12 */
	BYTE					mAudioTrackCount;					/* 输入音频声道数 */
	BYTE					mAudioSampleRate;					/* 输入音频采样率，0-8K，1-22.05K，2-44.1K，3-48K */
	BYTE					mAudioSampleBit;					/* 输入音频采样位数，0-8位，1-16位，2-32位 */
	WORD					mAudioFrameLen;						/* 音频帧长度 */
	BYTE					mIsSupportAudioOut;					/* 是否支持音频输出，0-不支持，1-支持 */
	BYTE					mVideoEncType;						/* 视频编码类型，见JT/T 1078-2016 表12 */
	BYTE					mAudioChnCount;						/* 支持的最大音频物理通道数量 */
	BYTE					mVideoChnCount;						/* 支持的最大视频物理通道数量 */

} JT808MsgBody_1003_t; 

/* 实时音视频传输请求 */
typedef struct 
{ 

	BYTE					mIpLen;								/* 服务器IP长度 */
	BYTE					mIp[255];							/* 服务器IP地址 */
	WORD					mTCPPort;							/* 服务器TCP通道端口 */
	WORD					mUDPPort;							/* 服务器UDP通道端口 */
	BYTE					mLogicChnNum;						/* 逻辑通道号，按照JT/T 1076-2016中的表2 */
	BYTE					mDataType;							/* 数据类型，0-音视频，1-视频，2-双向对讲，3-监听，4-中心广播，5-透传 */
	BYTE					mStreamType;						/* 码流类型，0-主码流，1-子码流 */

} JT808MsgBody_9101_t; 

/* 终端上传乘客流量 */
typedef struct 
{ 

	BYTE					mStartBcdTime[6];					/* 起始时间 */
	BYTE					mEndBcdTime[6];						/* 截止时间 */
	WORD					mGetOnCount;						/* 上车人数 */
	WORD					mGetOffCount;						/* 下车人数 */

} JT808MsgBody_1005_t; 

/* 实时音视频传输控制 */
typedef struct 
{ 

	BYTE					mLogicChnNum;						/* 逻辑通道号，按照JT/T 1076-2016中的表2 */
	BYTE					mCtrlCmd;							/* 控制指令，0-关闭音视频传输，1-切换码流，2-暂停该通道所有流的发送，3-恢复暂停前流的发送，4-关闭双向对讲 */
	BYTE					mStopType;							/* 关闭音视频类型，0-关闭所有该通道有关的音视频数据，1-只关闭该通道有关的音频，保留视频，2-只关闭该通道有关的视频，保留音频 */
	BYTE					mSwitchStreamType;					/* 将视频流切换为新类型，0-主码流，1-子码流 */

} JT808MsgBody_9102_t; 

/* 实时音视频传输状态通知 */
typedef struct 
{ 

	BYTE					mLogicChnNum;						/* 逻辑通道号，按照JT/T 1076-2016中的表2 */
	BYTE					mLostPacketRate;					/* 丢包率，数值乘以100以后取整部分 */

} JT808MsgBody_9105_t; 

/* 查询资源列表 */
typedef struct 
{ 

	BYTE					mLogicChnNum;						/* 逻辑通道号，按照JT/T 1076-2016中的表2 */
	BYTE					mStartBcdTime[6];					/* 起始时间 */
	BYTE					mEndBcdTime[6];						/* 截止时间 */
	DWORD					mAlertFlagh;						/* 报警标志位32-64，见JT/T 1078-2016表13 */
	DWORD					mAlertFlagl;						/* 报警标志位00-31，见JT/T 808-2013表24报警标志位 */
	BYTE					mResType;							/* 音视频资源类型，0-音视频，1-音频，2-视频，3-视频或音视频 */
	BYTE					mStreamType;						/* 码流类型，0-所有码流，1-主码流，2-子码流 */
	BYTE					mStorageType;						/* 存储器类型，0-所有存储器，1-主存储器，2-灾备存储器 */

} JT808MsgBody_9205_t; 

/* 终端上传音视频资源列表 */
typedef struct
{

	BYTE					mLogicChnNum;						/* 逻辑通道号，按照JT/T 1076-2016中的表2 */
	BYTE					mStartBcdTime[6];					/* 起始时间 */
	BYTE					mEndBcdTime[6];						/* 截止时间 */
	DWORD					mAlertFlagh;						/* 报警标志位32-64，见JT/T 1078-2016表13 */
	DWORD					mAlertFlagl;						/* 报警标志位00-31，见JT/T 808-2013表24报警标志位 */
	BYTE					mResType;							/* 音视频资源类型，0-音视频，1-音频，2-视频 */
	BYTE					mStreamType;						/* 码流类型，1-主码流，2-子码流 */
	BYTE					mStorageType;						/* 存储器类型，1-主存储器，2-灾备存储器 */
	DWORD					mFileSize;							/* 文件大小 */

} JT808MsgBody_1205_Item_t;
typedef struct 
{ 

	WORD					mReqSerialNum;							/* 对应查询命令的流水号 */
	DWORD					mResCount;							/* 音视频资源总数 */
	queue_list_t			mResList;							/* 音视频资源列表，JT808MsgBody_1205_Item_t */

} JT808MsgBody_1205_t; 

/* 远程录像回放请求 */
typedef struct 
{ 

	BYTE					mIpLen;								/* 服务器IP长度 */
	BYTE					mIp[64];							/* 服务器IP地址 */
	WORD					mTCPPort;							/* 服务器TCP通道端口 */
	WORD					mUDPPort;							/* 服务器UDP通道端口 */
	BYTE					mLogicChnNum;						/* 逻辑通道号，按照JT/T 1076-2016中的表2 */
	BYTE					mResType;							/* 音视频资源类型，0-音视频，1-音频，2-视频，3-视频或音视频 */
	BYTE					mStreamType;						/* 码流类型，0-所有码流，1-主码流，2-子码流 */
	BYTE					mStorageType;						/* 存储器类型，0-所有存储器，1-主存储器，2-灾备存储器 */
	BYTE					mPlaybackMode;						/* 回放方式，0-正常回放，1-快进回放，2-关键帧快退回放，3-关键帧播放，4-单帧上传 */
	BYTE					mFastSlowRate;						/* 快进或快退倍数，0-无效，1-1倍，2-2倍，3-4倍，4-8倍，5-16倍 */
	BYTE					mStartBcdTime[6];					/* 起始时间，回放方式为4时，该字段表示单帧上传时间 */
	BYTE					mEndBcdTime[6];						/* 截止时间，为0表示一直回放，回放方式为4时，该字段无效 */

} JT808MsgBody_9201_t; 

/* 远程录像回放控制 */
typedef struct 
{ 

	BYTE					mLogicChnNum;						/* 逻辑通道号，按照JT/T 1076-2016中的表2 */
	BYTE					mPlaybackCtrl;						/* 回放控制，0-开始，1-暂停，2-结束，3-快进，4-关键帧快退，5-拖动，6-关键帧播放 */
	BYTE					mFastSlowRate;						/* 快进或快退倍数，播放控制3、4时有效，0-无效，1-1倍，2-2倍，3-4倍，4-8倍，5-16倍 */
	BYTE					mDragBcdTime[6];					/* 拖动回放位置，控制为5时有效 */

} JT808MsgBody_9202_t; 

/* 文件上传指令 */
typedef struct 
{ 

	BYTE					mAddressLen;						/* FTP服务器地址长度 */
	BYTE					mAddresss[255];						/* FTP服务器地址 */
	WORD					mFTPPort;							/* FTP服务器端口 */
	BYTE					mUserNameLen;						/* 用户名长度 */
	BYTE					mUserName[64];						/* 用户名 */
	BYTE					mPswdNameLen;						/* 密码长度 */
	BYTE					mPswdName[64];						/* 密码 */
	BYTE					mFilePathLen;						/* 文件上传路径长度 */
	BYTE					mFilePath[255];						/* 文件上传路径 */
	BYTE					mLogicChnNum;						/* 逻辑通道号，按照JT/T 1076-2016中的表2 */
	BYTE					mStartBcdTime[6];					/* 起始时间，回放方式为4时，该字段表示单帧上传时间 */
	BYTE					mEndBcdTime[6];						/* 截止时间，为0表示一直回放，回放方式为4时，该字段无效 */
	DWORD					mAlertFlagh;						/* 报警标志位32-64，见JT/T 1078-2016表13 */
	DWORD					mAlertFlagl;						/* 报警标志位00-31，见JT/T 808-2013表24报警标志位 */
	BYTE					mResType;							/* 音视频资源类型，0-音视频，1-音频，2-视频，3-视频或音视频 */
	BYTE					mStreamType;						/* 码流类型，0-所有码流，1-主码流，2-子码流 */
	BYTE					mStorageType;						/* 存储器类型，0-所有存储器，1-主存储器，2-灾备存储器 */
	BYTE					mExcuteCondition;					/* 任务执行条件，bit0-WIFI,为1时表示WIFI下可下载，bit1-LAN,为1时表示LAN连接时可下载，bit2-3G/4G，为1时表示3G/4G连接时可下载 */

} JT808MsgBody_9206_t; 

/* 文件上传完成通知 */
typedef struct 
{ 

	WORD					mReqSerialNum;						/* 对应文件上传指令流水号 */
	BYTE					mResult;							/* 上传结果，0-成功，1-失败 */

} JT808MsgBody_1206_t; 

/* 文件上传控制 */
typedef struct 
{ 

	WORD					mReqSerialNum;						/* 对应文件上传指令流水号 */
	BYTE					mControlCmd;						/* 控制指令，0-暂停，1-继续，2-取消 */

} JT808MsgBody_9207_t; 

typedef struct 
{ 

//

} JT808MsgBody_9301_t; 

typedef struct 
{ 

//

} JT808MsgBody_9302_t; 

typedef struct 
{ 

//

} JT808MsgBody_9303_t; 

typedef struct 
{ 

//

} JT808MsgBody_9304_t; 

typedef struct 
{ 

//

} JT808MsgBody_9305_t; 

typedef struct 
{ 

//

} JT808MsgBody_9306_t; 

typedef struct 
{ 

	BYTE mIpLen;
	BYTE mIp[128];
	WORD mTCPPort;
	WORD mUDPPort;
	BYTE mAlertFlag[16];
	BYTE mAlertID[32];
	BYTE mResv[16];

} JT808MsgBody_9208_t; 

typedef struct
{

	BYTE mFilenameLen;
	BYTE mFilename[64];
	BYTE mFilenameOrg[64];					/* 非协议字段 */
	BYTE mFileType;							/* 非协议字段 */
	DWORD mFilesize;

} JT808MsgBody_1210_File_t;
typedef struct 
{ 

	BYTE mTerminalID[7];
	BYTE mAlertFlag[16];
	BYTE mAlertID[32];
	BYTE mInfoType;
	BYTE mFileCount;
	JT808MsgBody_1210_File_t* mFileList;

} JT808MsgBody_1210_t; 

typedef struct 
{ 

	BYTE mFilenameLen;
	BYTE mFilename[64];
	BYTE mFileType;
	DWORD mFilesize;

} JT808MsgBody_1211_t; 

typedef struct 
{ 

	BYTE mFilenameLen;
	BYTE mFilename[64];
	BYTE mFileType;
	DWORD mFilesize;

} JT808MsgBody_1212_t; 

typedef struct
{

	DWORD mOffset;
	DWORD mLength;

} JT808MsgBody_9212_Item_t;
typedef struct 
{ 

	BYTE mFilenameLen;
	BYTE mFilename[64];
	BYTE mFileType;
	BYTE mResult;
	BYTE mPackCount;
	JT808MsgBody_9212_Item_t items[255];

} JT808MsgBody_9212_t; 

#endif
