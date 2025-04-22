#ifndef _JT808_CMD_NODE_H_
#define _JT808_CMD_NODE_H_

/*
* 定义命令项结构，用于添加命令项列表
* 方便管理当前设备所支持的命令项
*/

#include "typedef.h"

#define EXPORT_CMD(id)				int JT808_##id##_CmdEncoder(void*, uint8_t*, int, void*);\
									int JT808_##id##_CmdParser(uint8_t*, void**, int, void*);

#define CMD_ENCODER(id)				JT808_##id##_CmdEncoder
#define CMD_PARSER(id)				JT808_##id##_CmdParser

#define CMD_TYPE_INITIATE			0
#define CMD_TYPE_RESPONSE			1

#define CMD_LEVEL_DISCARDABLE		0		    /* 可丢弃的消息，若有优先级高的消息，可以直接丢弃不做处理 */
#define CMD_LEVEL_IGNORABLE			1		    /* 可忽略的消息，不可丢弃不做处理，但重传失败后可不进行保存 */
#define CMD_LEVEL_IMPORTANT			2		    /* 重要的消息，不可忽略，不可丢弃，重传失败后需要保存重传后重新处理 */

typedef int(*JT808_CmdEncoder)(void* in, uint8_t* out, int len, void* pdev);
typedef int(*JT808_CmdParser)(uint8_t* in, void** out, int len, void* pdev);

typedef struct
{

	JT808_CmdEncoder        mEncoder;		    /* 对应消息体编码过程 */
	JT808_CmdParser         mParser;			/* 对应消息体数据解析到结构体过程 */
	void*                   mParam;    			/* 预留 */
	uint16_t                mMsgID;				/* 对应消息ID */
	uint8_t                 mType;				/* 类型：主动发送/应答 */
	uint8_t                 mLevel;				/* 重要性等级，决定重传依然失败后，是否需要保存 */

} JT808_CmdNode_t;

/* 01 BYTE, 02 WORD, 03 DWORD, 04 BCD, 05 STRING 06 STRUCT(保存到Bin文件) */
uint8_t JT808_Get_ParamID_Datatype(uint32_t paramID);

int JT808_HEAD_Parser(uint8_t* in, void** out, int len, void* pOwner);

int JT808_HEAD_Encoder(void* in, uint8_t* out, int len, void* pOwner);

EXPORT_CMD(0001) // 终端通用应答
EXPORT_CMD(8001) // 平台通用应答
EXPORT_CMD(0002) // 终端心跳
EXPORT_CMD(8003) // 服务器补传分包请求
EXPORT_CMD(0005) // 终端补传分包请求
EXPORT_CMD(0100) // 终端注册
EXPORT_CMD(8100) // 平台终端注册应答
EXPORT_CMD(0003) // 终端注销
EXPORT_CMD(0004) // 查询服务器时间
EXPORT_CMD(8004) // 查询服务器时间应答
EXPORT_CMD(0102) // 终端鉴权
EXPORT_CMD(8103) // 设置终端参数
EXPORT_CMD(8104) // 查询终端参数
EXPORT_CMD(0104) // 查询终端参数应答
EXPORT_CMD(8105) // 终端控制
EXPORT_CMD(8106) // 查询指定终端参数
EXPORT_CMD(8107) // 查询终端属性
EXPORT_CMD(0107) // 查询终端属性应答
EXPORT_CMD(8108) // 下发终端升级包
EXPORT_CMD(0108) // 终端升级结果通知
EXPORT_CMD(0200) // 位置信息汇报
EXPORT_CMD(8201) // 位置信息查询
EXPORT_CMD(0201) // 位置信息查询应答
EXPORT_CMD(8202) // 临时位置跟踪控制
EXPORT_CMD(8301) // 事件设置
EXPORT_CMD(0301) // 事件报告
EXPORT_CMD(8302) // 提问下发
EXPORT_CMD(0302) // 提问应答
EXPORT_CMD(8303) // 信息点播菜单设置
EXPORT_CMD(0303) // 信息点播/取消
EXPORT_CMD(8304) // 信息服务
EXPORT_CMD(8400) // 电话回拨
EXPORT_CMD(8401) // 设置电话本
EXPORT_CMD(8500) // 车辆控制
EXPORT_CMD(0500) // 车辆控制应答
EXPORT_CMD(8600) // 设置圆形区域
EXPORT_CMD(8601) // 删除圆形区域
EXPORT_CMD(8602) // 设置矩形区域
EXPORT_CMD(8603) // 删除矩形区域
EXPORT_CMD(8604) // 设置多边形区域
EXPORT_CMD(8605) // 删除多边形区域
EXPORT_CMD(8606) // 设置路线
EXPORT_CMD(8607) // 删除路线
EXPORT_CMD(8700) // 行驶记录仪数据采集命令
EXPORT_CMD(0700) // 行驶记录仪数据上传
EXPORT_CMD(8701) // 行驶记录仪参数下传命令
EXPORT_CMD(8203) // 人工确认报警消息
EXPORT_CMD(8204) // 服务器向终端发起链路检测请求
EXPORT_CMD(8300) // 文本信息下发
EXPORT_CMD(8702) // 上报驾驶员身份信息请求
EXPORT_CMD(0702) // 驾驶员身份信息采集上报
EXPORT_CMD(0704) // 定位数据批量上传
EXPORT_CMD(0705) // CAN总线数据上传
EXPORT_CMD(0800) // 多媒体事件信息上传
EXPORT_CMD(0801) // 多媒体数据上传
EXPORT_CMD(8800) // 多媒体数据上传应答
EXPORT_CMD(8801) // 摄像头立即拍摄命令
EXPORT_CMD(0805) // 摄像头立即拍摄命令应答
EXPORT_CMD(8802) // 存储多媒体数据检索
EXPORT_CMD(0802) // 存储多媒体数据检索应答
EXPORT_CMD(8608) // 查询区域或线路数据
EXPORT_CMD(0608) // 查询区域或线路数据应答
EXPORT_CMD(0701) // 电子运单上报
EXPORT_CMD(8803) // 存储多媒体数据上传
EXPORT_CMD(8804) // 录音开始命令
EXPORT_CMD(8805) // 单挑存储多媒体数据检索上传命令
EXPORT_CMD(8900) // 数据下行透传
EXPORT_CMD(0900) // 数据上行透传
EXPORT_CMD(0901) // 数据压缩上报
EXPORT_CMD(8A00) // 平台RSA公钥
EXPORT_CMD(0A00) // 终端RSA公钥

// 1078拓展
EXPORT_CMD(9003) // 查询终端音视频属性
EXPORT_CMD(1003) // 终端音视频属性上传
EXPORT_CMD(9101) // 实时音视频传输请求
EXPORT_CMD(1005) // 终端上传乘客流量
EXPORT_CMD(9102) // 音视频实时传输控制
EXPORT_CMD(9105) // 实时音视频传输状态通知
EXPORT_CMD(9205) // 查询资源列表
EXPORT_CMD(1205) // 终端上传音视频资源列表
EXPORT_CMD(9201) // 平台下发远程录像回放请求
EXPORT_CMD(9202) // 平台下发远程录像回放控制
EXPORT_CMD(9206) // 文件上传指令
EXPORT_CMD(1206) // 文件上传完成通知
EXPORT_CMD(9207) // 文件上传控制
EXPORT_CMD(9301) // 云台旋转
EXPORT_CMD(9302) // 云台调整焦距控制
EXPORT_CMD(9303) // 云台调整光圈控制
EXPORT_CMD(9304) // 云台雨刷控制
EXPORT_CMD(9305) // 红外补光控制
EXPORT_CMD(9306) // 云台变倍控制

// 主动安全拓展
EXPORT_CMD(9208) // 报警附件上传指令
EXPORT_CMD(1210) // 报警附件信息消息
EXPORT_CMD(1211) // 文件信息上传
EXPORT_CMD(1212) // 文件上传完成消息
EXPORT_CMD(9212) // 文件上传完成消息应答


#endif
