/*
 *------------------------------------------------------------------------------
 * @File      :    mdp_facereg_nn.c
 * @Date      :    2021-9-24
 * @Author    :    lomboswer <lomboswer@lombotech.com>
 * @Brief     :    Source file for MDP(Media Development Platform).
 *
 * Copyright (C) 2020-2021, LomboTech Co.Ltd. All rights reserved.
 *------------------------------------------------------------------------------
 */
 #ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
#define EI_TRACE_LOG(level, fmt, args...)\
 do{ \
	 EI_TRACE(EI_ID_LOG, level, fmt, ##args);\
 } while (0)

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <errno.h>
#include <sys/prctl.h>
#include "dirent.h"
#include <pthread.h>
#include "semaphore.h"
#include "sample_comm.h"
#include "osal.h"
#include "nn_facerecg_api.h"
#include "mi_vdec.h"
#include <sqlite3.h>

#define compare_distance(a0, a1, a2, a3) (((a3-a2)/(a1-a0))> 0.9 && ((a3-a2)/(a1-a0))< 1.1)?1:0

#define NN_FACERECG_AX_PATH	 "/usr/share/ax/dms"
#define COUNT_REGISTER_FACE_FRAME 20
#define PATH_ROOT "/data/" /* save file root path */
#define MQ_MSG_NUM 128 /* message queue number */
#define MQ_MSG_LEN 128 /* every message max lenght */
#define ALARM_INTERVAL 5000 /* same warning message interval time unit:ms */
#define REG_FACE_FAIL_ALARM_INTERVAL 20000 /* same warning message interval time unit:ms */
#define RECG_FAIL_ALARM_INTERVAL 20000 /* same warning message interval time unit:ms */
#define SDR_VISS_FRAMERATE 25
#define RGN_ARRAY_NUM 2  /* rgn  array 0:rect 1:line */
#define RGN_RECT_NUM 16 /* rect max num */
#define RGN_LINE_NUM 16 /* line max num */
#define RGN_DRAW /* rgn draw, if not define this macro, use software draw */
#define FACE_REGISTER_MIN_PIX  64
#define FACE_REGISTER_MAX_PIX  512
#define VDEC_MAX_CHN_NUM 4
#define SCN_WIDTH 1280
#define SCN_HEIGHT 720
/* #define SAVE_JPG_DEC_YUV_SP */ /* save yuv data after jpg dec */
/* #define SAVE_AX_YUV_SP */ /* save yuv frame send to al module */
/* #define SAVE_DRAW_YUV_SP */ /* save yuv frame send to disp module, not use */
/* #define SAVE_REGISTER_FACE_CAM_FRAME */ /* if macro open, save as yuv file, else save as jgp file */

typedef enum _WARNING_MSG_TYPE_ {
	LB_WARNING_BEGIN = 0xF000,
	LB_WARNING_FACERECG_CAM_REG_START,
	LB_WARNING_FACERECG_IMG_REG_START,
	LB_WARNING_FACERECG_REG_SUCCESS,
	LB_WARNING_FACERECG_CAM_REG_FCACE_TOO_BIG,
	LB_WARNING_FACERECG_CAM_REG_FCACE_TOO_SMALL,
	LB_WARNING_FACERECG_REG_FAIL,
	LB_WARNING_FACERECG_MATCH_START,
	LB_WARNING_FACERECG_MATCH_SUCCESS,
	LB_WARNING_FACERECG_MATCH_FAIL,
	LB_WARNING_USERDEF,
	LB_WARNING_END,
} WARNING_MSG_TYPE;
typedef enum _REG_FACE_ACTION_ {
	RECOGNITION_FACE,
	REGISTER_FACE_CAM,
	REGISTER_FACE_IMG,
	REG_ACTION_END,
} REG_FACE_ACTION;

typedef struct _axframe_info_s {
    EI_BOOL bThreadStart;
    VISS_CHN phyChn;
    VISS_DEV dev;
    VISS_CHN chn;
    RECT_S stFrameSize;
    unsigned int frameRate;
    EI_U32 u32Width;
    EI_U32 u32Height;
    pthread_t gs_framePid;
    sem_t frame_del_sem;
	REG_FACE_ACTION action;
	VIDEO_FRAME_INFO_S frame_info;
	nn_facerecg_cfg_t facerecg_cfg;
	nn_facerecg_in_t recg_in;
	nn_facerecg_faces_t faces;
	nn_facerecg_features_t features;
	nn_facerecg_features_t *match_features;
	nn_facerecg_match_result_t result;
	int m_face_rect[4];
	void *nn_hdl;
	int input_id;
	char input_name[30];
	sqlite3 *drivers_db;
	char file_name[30];
	int match_id; /* the match id insert to db */
	char match_name[128]; /* the match name insert to db */
	float match_score;
	int nn_facerecg_det_count;
	int nn_facerecg_det_too_big_count;
	int nn_facerecg_det_too_small_count;
	int reg_flag;
	FILE *ax_fout;
	nn_facerecg_faces_t *p_det_info;
} axframe_info_s;

typedef struct _drawframe_info_s {
    EI_BOOL bThreadStart;
    VISS_CHN phyChn;
    VISS_DEV dev;
    VISS_CHN chn;
	VO_DEV VoDev;
	VO_LAYER VoLayer;
	VO_CHN VoChn;
    RECT_S stFrameSize;
#ifdef RGN_DRAW
	RGN_ATTR_S stRegion[RGN_ARRAY_NUM];
    RGN_CHN_ATTR_S stRgnChnAttr[RGN_ARRAY_NUM];
    RGN_HANDLE Handle[RGN_ARRAY_NUM];
    RECTANGLEEX_ARRAY_SUB_ATTRS stRectArray[RGN_RECT_NUM];
    LINEEX_ARRAY_SUB_ATTRS stLineArray[RGN_LINE_NUM];
	MDP_CHN_S stRgnChn;
	int last_draw_num;
#endif
    EI_U32 u32Width;
    EI_U32 u32Height;
	EI_U32 u32scnWidth;
	EI_U32 u32scnHeight;
    unsigned int frameRate;
    pthread_t gs_framePid;
    sem_t frame_del_sem;
	FILE *draw_fout;
} drawframe_info_s;

typedef struct _warning_info_s {
    EI_BOOL bThreadStart;
    int mq_id;
	int64_t last_warn_time;
	int last_warn_status;
    pthread_t g_Pid;
    sem_t frame_del_sem;
} warning_info_s;

typedef struct tag_warn_msg {
	int32_t		type;
	int32_t		len;
	char		data[120];
} tag_warn_msg_t;

typedef struct __mdp_vdec_thread_para_s__ {
    EI_S32 s32ChnId;
    PAYLOAD_TYPE_E enType;
    EI_CHAR full_name[256];
    EI_S32 s32StreamMode;
    EI_S32 s32MilliSec;
    EI_S32 s32MinBufSize;
    THREAD_CONTRL_E eThreadCtrl;
    EI_U32 frame_cnt;
    EI_S32 s32IntervalTime;
    EI_U64  u64PtsInit;
    EI_U64  u64PtsIncrease;
    EI_BOOL bCircleSend;
} mdp_vdec_thread_para_s;

typedef struct _mdp_vdec_scale_info_ {
    EI_U32              u32Align;
    EI_U32              u32MaxWidth;
    EI_U32              u32MaxHeight;
    EI_BOOL             bFixedRatio;
    EI_BOOL             bScaleEnable;
} mdp_vdec_scale_info;

typedef struct _mdp_vdec_config_s_ {
    PAYLOAD_TYPE_E  enType;
    MOD_ID_E        enModId;
    VIDEO_MODE_E    enMode;
    EI_U32          u32Align;
    EI_U32          u32Width;
    EI_U32          u32Height;
    PIXEL_FORMAT_E  enPixelFormat;
    EI_U32          u32FrameBufCnt;
    VBUF_POOL       Pool;
    mdp_vdec_scale_info scaleInfo;
} mdp_vdec_config_s;

typedef struct _mdp_vdec_info_s {
	VC_CHN s32ChnNum;
	EI_CHAR vdec_full_name[256];
	mdp_vdec_config_s config;
	mdp_vdec_thread_para_s stVdecSend[VDEC_MAX_CHN_NUM];
	pthread_t	VdecThread[VDEC_MAX_CHN_NUM];
} mdp_vdec_info_s;

typedef struct __sample_venc_para_t {
    EI_BOOL bThreadStart;
	VISS_DEV dev;
    VC_CHN VeChn;
	VISS_CHN chn;
    int bitrate;
    int file_idx;
    int muxer_type;
    int init_flag;
	int exit_flag;
    FILE *flip_out;
    char aszFileName[128];
	char szFilePostfix[32];
    int buf_offset;

    void *muxer_hdl;
    pthread_t gs_VencPid;
	
} sdr_sample_venc_para_t;

typedef struct {
    unsigned int total;
    unsigned int free;
} sdcard_info_t;

typedef struct _driver_info_ {
	int id;
	char name[128];
}driver_info_t;

typedef struct _driver_all_info_ {
	driver_info_t info[100];
	nn_facerecg_features_t m_face_features;
	int size;
} driver_all_info_t;

static driver_all_info_t m_all_info;
static axframe_info_s axframe_info;
static drawframe_info_s drawframe_info;
static warning_info_s warning_info;
sdr_sample_venc_para_t stream_para[ISP_MAX_DEV_NUM] = {{0}};
static mdp_vdec_info_s mdp_vdec_info;


void preview_help(void)
{
    printf("usage: mdp_facerecg_nn \n");
    printf("such as: mdp_facerecg_nn\n");
    printf("such as: mdp_facerecg_nn -d 0 -c 0 -r 90 -a 0 -i 0 -n abc\n");

    printf("arguments:\n");
    printf("-d    select input device, Dev:0~1, default: 0\n");
    printf("-c    select chn id, support:0~3, default: 0\n");
    printf("-r    select rotation, 0/90/180/270, default: 90\n");
    printf("-s    sensor or rx macro value, such as 200800, default: 100301\n");
	printf("-a    action for facerecg, 0:recognition face 1:register face from camera; 2:register face from image, default: 0\n");
	printf("-i    index insert to sqlite3 save face feature, default: 0\n");
	printf("-n    name insert to sqlite3 to save face feature, default: n-0\n");
	printf("-f    image file full name, only use for action 1, register face from image file full name, default: NULL\n");
    printf("-h    help\n");
}

EI_U32 SAMPLE_COMM_SYS_Init(VBUF_CONFIG_S *pstVbConfig)
{
    EI_S32 s32Ret = EI_SUCCESS;
    EI_S32 i = 0;

    EI_MI_MBASE_Init();
    EI_MI_VBUF_Init();
    EI_MI_MLINK_Init();

    s32Ret = EI_MI_VBUF_InitVbufConfig(pstVbConfig);
    if (s32Ret) {
        EI_TRACE_VBUF(EI_DBG_ERR, "create pool size %d failed!\n", pstVbConfig->astCommPool[i].u32BufSize);
        return s32Ret;
    }

    return s32Ret;
}
EI_S32 rgn_start(void)
{
	EI_S32 s32Ret = EI_FAILURE;

	drawframe_info.stRgnChn.enModId = EI_ID_VISS;
	drawframe_info.stRgnChn.s32DevId = drawframe_info.dev;
	drawframe_info.stRgnChn.s32ChnId = drawframe_info.chn;
	drawframe_info.Handle[0] = 0;
    drawframe_info.stRegion[0].enType = RECTANGLEEX_ARRAY_RGN;
    drawframe_info.stRegion[0].unAttr.u32RectExArrayMaxNum = RGN_RECT_NUM;
	
    s32Ret = EI_MI_RGN_Create(drawframe_info.Handle[0], &drawframe_info.stRegion[0]);
    if(s32Ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_Create \n");
        goto exit;
    }
	drawframe_info.Handle[1] = 1;
    drawframe_info.stRegion[1].enType = LINEEX_ARRAY_RGN;
    drawframe_info.stRegion[1].unAttr.u32LineExArrayMaxNum = RGN_LINE_NUM;
    s32Ret = EI_MI_RGN_Create(drawframe_info.Handle[1], &drawframe_info.stRegion[1]);
    if(s32Ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_Create \n");
        goto exit;
    }
	drawframe_info.stRgnChnAttr[0].bShow = EI_TRUE;
    drawframe_info.stRgnChnAttr[0].enType = RECTANGLEEX_ARRAY_RGN;
    drawframe_info.stRgnChnAttr[0].unChnAttr.stRectangleExArrayChn.u32MaxRectNum = RGN_RECT_NUM;
    drawframe_info.stRgnChnAttr[0].unChnAttr.stRectangleExArrayChn.u32ValidRectNum = RGN_RECT_NUM;
    drawframe_info.stRgnChnAttr[0].unChnAttr.stRectangleExArrayChn.pstRectExArraySubAttr = drawframe_info.stRectArray;

    s32Ret = EI_MI_RGN_AddToChn(drawframe_info.Handle[0], &drawframe_info.stRgnChn, &drawframe_info.stRgnChnAttr[0]);
	if(s32Ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_AddToChn \n");
    }
	drawframe_info.stRgnChnAttr[1].bShow = EI_TRUE;
    drawframe_info.stRgnChnAttr[1].enType = LINEEX_ARRAY_RGN;
    drawframe_info.stRgnChnAttr[1].unChnAttr.stLineExArrayChn.u32MaxLineNum = RGN_LINE_NUM;
    drawframe_info.stRgnChnAttr[1].unChnAttr.stLineExArrayChn.u32ValidLineNum = RGN_LINE_NUM;
    drawframe_info.stRgnChnAttr[1].unChnAttr.stLineExArrayChn.pstLineExArraySubAttr = drawframe_info.stLineArray;
    s32Ret = EI_MI_RGN_AddToChn(drawframe_info.Handle[1], &drawframe_info.stRgnChn, &drawframe_info.stRgnChnAttr[1]);
	if(s32Ret) {
        EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_AddToChn \n");
    }

	return EI_SUCCESS;
exit:
	return s32Ret;
}

void rgn_stop(void)
{
	int i;

	for (i = 0;i < RGN_ARRAY_NUM; i++) {
		EI_MI_RGN_DelFromChn(drawframe_info.Handle[i], &drawframe_info.stRgnChn);
		EI_MI_RGN_Destroy(drawframe_info.Handle[i]);
	}
}

static void facerecg_draw_location_rgn(VIDEO_FRAME_INFO_S *drawFrame, nn_facerecg_faces_t *faces)
{
	int draw_num;
	EI_S32 s32Ret = EI_FAILURE;

	draw_num = faces->size;
	if (draw_num > RGN_RECT_NUM)
		return;

	if (drawframe_info.last_draw_num > draw_num) {
		for (int i = drawframe_info.last_draw_num-1; i >= draw_num; i--) {
			drawframe_info.stRectArray[i].isShow = EI_FALSE;
			s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[0],
				&drawframe_info.stRgnChn, &drawframe_info.stRgnChnAttr[0]);
			if(s32Ret) {
    			EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_SetChnAttr \n");
			}
		}
	}
    if (faces->size) {
        int loc_x0, loc_y0, loc_x1, loc_y1;
		for (int cnt_faces = 0; cnt_faces < faces->size; cnt_faces++) {
	        loc_x0 = faces->p[cnt_faces].box[0];
	        loc_y0 = faces->p[cnt_faces].box[1];
	        loc_x1 = faces->p[cnt_faces].box[2];
	        loc_y1 = faces->p[cnt_faces].box[3];
	        drawframe_info.stRectArray[cnt_faces].stRectAttr.stRect.s32X = loc_x0;
			drawframe_info.stRectArray[cnt_faces].stRectAttr.stRect.s32Y = loc_y0;
			drawframe_info.stRectArray[cnt_faces].stRectAttr.stRect.u32Height = loc_y1-loc_y0;
			drawframe_info.stRectArray[cnt_faces].stRectAttr.stRect.u32Width = loc_x1-loc_x0;
			drawframe_info.stRectArray[cnt_faces].stRectAttr.u32BorderSize = 4;
			if (axframe_info.action == RECOGNITION_FACE)
				drawframe_info.stRectArray[cnt_faces].stRectAttr.u32Color = 0xff8000ff;
			else
				drawframe_info.stRectArray[cnt_faces].stRectAttr.u32Color = 0xffff0000;
			drawframe_info.stRectArray[cnt_faces].isShow = EI_TRUE;
			drawframe_info.stRgnChnAttr[0].bShow = EI_TRUE;
			s32Ret = EI_MI_RGN_SetChnAttr(drawframe_info.Handle[0], &drawframe_info.stRgnChn, &drawframe_info.stRgnChnAttr[0]);
			if(s32Ret) {
	        	EI_TRACE_LOG(EI_DBG_ERR, "EI_MI_RGN_SetChnAttr \n");
	    	}
		}
    }
	drawframe_info.last_draw_num = draw_num;
}
int sqlite_callback(void *para, int argc, char **argv, char **azColName){
	int i;
	/*char *cpara = (char *)para;
	EI_PRINT( "cpara = %s\n, argc:%d \n", cpara? cpara: "NULL", argc);*/
	for(i=0; i<argc; i++){
		EI_PRINT("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}
static int createTable(sqlite3 *db, char *sql)
{
	int ret;
	char *zErrMsg = 0;

	/* Execute SQL statement */
	ret = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
	if (ret != SQLITE_OK){
		EI_PRINT("createTable SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		return 0;
	} else {
		EI_PRINT("createTable successfully\n");
		return -1;
	}
}
void open_sqlitedb()
{
	char *sql;
	int rc;

	/* Open database */
	rc = sqlite3_open("/data/drivers_db.db", &axframe_info.drivers_db);

	if (rc) {
		EI_PRINT("Can't open database: %s\n", sqlite3_errmsg(axframe_info.drivers_db));
		return ;
	}else{
		EI_PRINT("Opened database successfully\n");
	}

	EI_PRINT("  drivers_db:%p\n", axframe_info.drivers_db);
	// create table sql staement
	sql = "CREATE TABLE IF NOT EXISTS drives("  \
		"ID INT PRIMARY KEY     NOT NULL," \
		"name           TEXT    NOT NULL," \
		"norm           FLOAT   NOT NULL," \
		"feature_len    INT," \
		"feature        BLOB);";
	createTable(axframe_info.drivers_db, sql);
}
void close_sqlitedb()
{
	sqlite3_close(axframe_info.drivers_db);
}

static int insertData(float norm, int feature_len, signed char *feature)
{
	sqlite3_stmt *stmt = 0;
	int ret;
	char sql[512];

	sprintf(sql, "INSERT OR REPLACE INTO drives values(%d,'%s',%f,%d,?);", axframe_info.input_id, axframe_info.input_name, norm, feature_len);
	//sprintf(sql, "insert into drives values(103,'hello',%f,%d,?);", norm, feature_len);
	EI_PRINT("sql:%s\n", sql);
	ret = sqlite3_prepare(axframe_info.drivers_db, sql, strlen(sql), &stmt, 0);
	if (ret != SQLITE_OK) {
		EI_PRINT("sqlite3_prepare filed, ret:%d\n", ret);
		return -1;
	}
	ret = sqlite3_bind_blob(stmt, 1, feature, feature_len, NULL);
	if (ret != SQLITE_OK) {
		EI_PRINT("sqlite3_step filed, ret:%d\n", ret);
		return -1;
	}
	ret = sqlite3_step(stmt);
	if (ret != SQLITE_DONE) {
		EI_PRINT("sqlite3_step filed, ret:%d\n", ret);
	}
	sqlite3_finalize(stmt);

	return 0;

}

static void selectDb()
{
	sqlite3_stmt *stmt;
	char *sql = "select * from drives;" ;

	m_all_info.size = 0;
	m_all_info.m_face_features.p = (nn_facerecg_feature_t *)malloc(sizeof(nn_facerecg_feature_t)*100);
	memset(m_all_info.m_face_features.p,0,sizeof(nn_facerecg_feature_t)*100);
	sqlite3_prepare(axframe_info.drivers_db, sql, strlen(sql), &stmt, 0);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		EI_PRINT("select one record!!!!!!!!!\n");
		int len;
		int index = m_all_info.size;

		m_all_info.info[index].id = sqlite3_column_int(stmt, 0);
		const unsigned char *pname = sqlite3_column_text(stmt, 1);
		len = sqlite3_column_bytes(stmt, 1);
		if (pname) {
			//memcpy(name, pname, len);
			memcpy(m_all_info.info[index].name, pname, len);
		}
		m_all_info.m_face_features.p[index].norm = (float)sqlite3_column_double(stmt, 2);
		m_all_info.m_face_features.p[index].feature_len = sqlite3_column_int(stmt, 3);
		const char *pfeature = sqlite3_column_blob(stmt, 4);
		len = sqlite3_column_bytes(stmt, 4);
		if (pfeature) {
			//memcpy(feature, pfeature, len);
			memcpy(m_all_info.m_face_features.p[index].feature, pfeature, len);
		}
		m_all_info.size += 1;
		EI_PRINT("index:%d, id:%d, name:%s\n", index, m_all_info.info[index].id, m_all_info.info[index].name);
		EI_PRINT("feature_len:%d, norm:%f\n", m_all_info.m_face_features.p[index].feature_len, 
			m_all_info.m_face_features.p[index].norm);
		EI_PRINT("m_all_info.m_face_features.p[%d].feature: ", index);
		for (int i=0; i<m_all_info.m_face_features.p[index].feature_len; i++) {
			EI_PRINT("%d ", m_all_info.m_face_features.p[index].feature[i]);
		}
		EI_PRINT("\n");
	}
	m_all_info.m_face_features.size = m_all_info.size;
	sqlite3_finalize(stmt);
}

int start_nn_facerecg_fun(REG_FACE_ACTION action)
{
	osal_mdelay(3 * 1000);
	/* init nn_facerecg */
	axframe_info.facerecg_cfg.fmt = YUV420_SP;
	axframe_info.facerecg_cfg.img_width = axframe_info.u32Width;
	axframe_info.facerecg_cfg.img_height = axframe_info.u32Height;
	axframe_info.facerecg_cfg.model_index = 0;
	axframe_info.facerecg_cfg.model_basedir = NN_FACERECG_AX_PATH;
	axframe_info.facerecg_cfg.interest_roi[0] = 0;
	axframe_info.facerecg_cfg.interest_roi[1] = 0;
	axframe_info.facerecg_cfg.interest_roi[2] = axframe_info.u32Width - 1;
	axframe_info.facerecg_cfg.interest_roi[3] = axframe_info.u32Height - 1;

	/* open libdms_facerecg.so*/
	axframe_info.nn_hdl = nn_facerecg_create(&axframe_info.facerecg_cfg);
	if(axframe_info.nn_hdl == NULL){
		EI_PRINT("nn_facerecg_create() failed!");
		return -1;
	}
	
	return 0;
}

EI_VOID *mdp_vdec_sendstream(EI_VOID *pArgs)
{
    mdp_vdec_thread_para_s *pstVdecThreadParam =(mdp_vdec_thread_para_s *)pArgs;
    EI_BOOL bEndOfStream = EI_FALSE;
    EI_S32 s32UsedBytes = 0, s32ReadLen = 0;
    FILE *fpStrm=NULL;
    EI_U8 *pu8Buf = NULL;
    VDEC_STREAM_S stStream;
    EI_BOOL bFindStart;
    EI_U64 u64PTS = 0;
    EI_U32 u32Len, u32Start;
    EI_S32 s32Ret, i;
    EI_CHAR cStreamFile[256];

    pstVdecThreadParam->frame_cnt = 0;
	EI_PRINT("pstVdecThreadParam->full_name:%s\n", pstVdecThreadParam->full_name);
    strcpy(cStreamFile, pstVdecThreadParam->full_name);
    if(cStreamFile != 0)
    {
        fpStrm = fopen(cStreamFile, "rb");
        if(fpStrm == NULL)
        {
            EI_TRACE_VDEC(EI_DBG_ERR, "chn %d can't open file %s in send stream thread!\n", pstVdecThreadParam->s32ChnId, cStreamFile);
            return NULL;
        }
    }
    EI_TRACE_VDEC(EI_DBG_ERR, "\n \033[0;36m chn %d, stream file:%s, userbufsize: %d \033[0;39m\n", pstVdecThreadParam->s32ChnId,
        pstVdecThreadParam->full_name, pstVdecThreadParam->s32MinBufSize);

    pu8Buf = malloc(pstVdecThreadParam->s32MinBufSize);
    if(pu8Buf == NULL)
    {
        EI_TRACE_VDEC(EI_DBG_ERR, "chn %d can't alloc %d in send stream thread!\n", pstVdecThreadParam->s32ChnId, pstVdecThreadParam->s32MinBufSize);
        fclose(fpStrm);
        return NULL;
    }

    fflush(stdout);

    u64PTS = pstVdecThreadParam->u64PtsInit;
    pstVdecThreadParam->bCircleSend = EI_FALSE;
    while (1)
    {
        bEndOfStream = EI_FALSE;
        bFindStart   = EI_FALSE;
        u32Start     = 0;

        fseek(fpStrm, s32UsedBytes, SEEK_SET);
		EI_TRACE_VDEC(EI_DBG_ERR, "s32UsedBytes = %d **************\n", s32UsedBytes);
        s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);
        EI_TRACE_VDEC(EI_DBG_ERR, "s32ReadLen0 = %d **************\n", s32ReadLen);
        if (s32ReadLen == 0)
        {
            pstVdecThreadParam->bCircleSend = EI_FALSE;
            EI_TRACE_VDEC(EI_DBG_ERR, "read len == 0 **************\n");
            if (pstVdecThreadParam->bCircleSend == EI_TRUE)
            {
                memset(&stStream, 0, sizeof(VDEC_STREAM_S) );
                stStream.bEndOfStream = EI_TRUE;
                EI_MI_VDEC_SendStream(pstVdecThreadParam->s32ChnId, &stStream, -1);

                s32UsedBytes = 0;
                fseek(fpStrm, 0, SEEK_SET);
                s32ReadLen = fread(pu8Buf, 1, pstVdecThreadParam->s32MinBufSize, fpStrm);
            } else {
                EI_TRACE_VDEC(EI_DBG_ERR, "break **************\n");
                break;
            }
        }
        if (pstVdecThreadParam->s32StreamMode==VIDEO_MODE_FRAME && pstVdecThreadParam->enType == PT_H264) {

        }
        else if (pstVdecThreadParam->s32StreamMode==VIDEO_MODE_FRAME
            && pstVdecThreadParam->enType == PT_H265) {

        }
        else if (pstVdecThreadParam->enType == PT_MJPEG || pstVdecThreadParam->enType == PT_JPEG) {
            for (i=0; i<s32ReadLen-1; i++)
            {
                if (pu8Buf[i] == 0xFF && pu8Buf[i+1] == 0xD8)
                {
                    u32Start = i;
                    bFindStart = EI_TRUE;
                    i = i + 2;
                    break;
                }
            }

            for (; i<s32ReadLen-3; i++)
            {
                if ((pu8Buf[i] == 0xFF) && (pu8Buf[i+1]& 0xF0) == 0xE0)
                {
                     u32Len = (pu8Buf[i+2]<<8) + pu8Buf[i+3];
                     i += 1 + u32Len;
                }
                else
                {
                    break;
                }
            }

            for (; i<s32ReadLen-1; i++)
            {
                if (pu8Buf[i] == 0xFF && pu8Buf[i+1] == 0xD9)
                {
                    break;
                }
            }
            s32ReadLen = i+2;
			EI_TRACE_VDEC(EI_DBG_ERR, "s32ReadLen1 = %d **************\n", s32ReadLen);
            if (bFindStart == EI_FALSE)
            {
                EI_TRACE_VDEC(EI_DBG_ERR, "chn %d can not find JPEG start code!s32ReadLen %d, s32UsedBytes %d.!\n",
                    pstVdecThreadParam->s32ChnId, s32ReadLen, s32UsedBytes);
            }
        }
        else
        {
            if((s32ReadLen != 0) && (s32ReadLen < pstVdecThreadParam->s32MinBufSize))
            {
                bEndOfStream = EI_TRUE;
            }
        }
        stStream.u64PTS       = u64PTS;
        stStream.pu8Addr      = pu8Buf + u32Start;
        stStream.u32Len       = s32ReadLen;
        stStream.bEndOfFrame  = (pstVdecThreadParam->s32StreamMode==VIDEO_MODE_FRAME)? EI_TRUE: EI_FALSE;
        stStream.bEndOfStream = bEndOfStream;
        stStream.bDisplay     = 1;
        stStream.bFlushBuffer = EI_TRUE;
SendAgain:
        s32Ret = EI_MI_VDEC_SendStream(pstVdecThreadParam->s32ChnId, &stStream, pstVdecThreadParam->s32MilliSec);
        if( (EI_FAILURE == s32Ret) && (THREAD_CTRL_START == pstVdecThreadParam->eThreadCtrl) )
        {
            usleep(pstVdecThreadParam->s32IntervalTime);
            goto SendAgain;
        }
        else
        {
            if(s32Ret == EI_SUCCESS)
                pstVdecThreadParam->frame_cnt++;
            bEndOfStream = EI_FALSE;
            s32UsedBytes = s32UsedBytes +s32ReadLen + u32Start;
            u64PTS += pstVdecThreadParam->u64PtsIncrease;
            EI_TRACE_VDEC(EI_DBG_ERR, "test frame_cnt = %d u64PTS %lld**************\n", pstVdecThreadParam->frame_cnt, u64PTS);
        }
        usleep(pstVdecThreadParam->s32IntervalTime);
		if (s32ReadLen < pstVdecThreadParam->s32MinBufSize) {
			pstVdecThreadParam->bCircleSend = EI_FALSE;
			break;
		}
    }
    /* send the flag of stream end */
    memset(&stStream, 0, sizeof(VDEC_STREAM_S) );
    stStream.bEndOfStream = EI_TRUE;
    pstVdecThreadParam->eThreadCtrl = THREAD_CTRL_STOP;

    EI_TRACE_VDEC(EI_DBG_ERR, "\033[0;35m chn %d send steam thread return ...  \033[0;39m\n", pstVdecThreadParam->s32ChnId);
    fflush(stdout);
    if (pu8Buf != EI_NULL)
    {
        free(pu8Buf);
    }
    fclose(fpStrm);
    return (EI_VOID *)EI_SUCCESS;
}

EI_S32 mdp_vdec_startSendStream(EI_S32 s32ChnNum, mdp_vdec_thread_para_s *pstVdecSend, pthread_t *pVdecThread)
{
    pVdecThread[s32ChnNum] = 0;
    pthread_create(&pVdecThread[s32ChnNum], 0, mdp_vdec_sendstream, (EI_VOID *)&pstVdecSend[s32ChnNum]);
    return 0;
}

EI_S32 mdp_vdec_start(VC_CHN s32ChnNum, mdp_vdec_config_s *pstVdec)
{
    EI_S32  s32Ret;
    VDEC_CHN_ATTR_S stChnAttr;

    stChnAttr.enType           = pstVdec->enType;
    stChnAttr.enMode           = pstVdec->enMode;
    stChnAttr.enPixelFormat    = pstVdec->enPixelFormat;
    stChnAttr.u32Align         = pstVdec->u32Align;
    stChnAttr.u32Width         = pstVdec->u32Width;
    stChnAttr.u32Height        = pstVdec->u32Height;
    stChnAttr.u32StreamBufSize = 1024 * 1024;
    stChnAttr.u32FrameBufCnt   = pstVdec->u32FrameBufCnt;
    stChnAttr.enRemapMode      = VBUF_REMAP_MODE_CACHED;

    stChnAttr.stVdecVideoAttr.stVdecScale.u32Align      = pstVdec->scaleInfo.u32Align;
    stChnAttr.stVdecVideoAttr.stVdecScale.u32Width      = pstVdec->scaleInfo.u32MaxWidth;
    stChnAttr.stVdecVideoAttr.stVdecScale.u32Height     = pstVdec->scaleInfo.u32MaxHeight;
    stChnAttr.stVdecVideoAttr.stVdecScale.bFixedRatio   = pstVdec->scaleInfo.bFixedRatio;
    stChnAttr.stVdecVideoAttr.stVdecScale.bScaleEnable  = pstVdec->scaleInfo.bScaleEnable;

    if (PT_H264 == pstVdec->enType || PT_H265 == pstVdec->enType)
    {
        ;
    }
    else if (PT_JPEG == pstVdec->enType || PT_MJPEG == pstVdec->enType)
    {
        ;
    }

    s32Ret = EI_MI_VDEC_CreateChn(s32ChnNum, &stChnAttr);
    if (EI_SUCCESS != s32Ret) {
        EI_TRACE_VENC(EI_DBG_ERR, "EI_MI_VDEC_CreateChn faild with%#x! \n", s32Ret);
        return EI_FAILURE;
    }

    s32Ret = EI_MI_VDEC_StartRecvStream(s32ChnNum);
    if (EI_SUCCESS != s32Ret) {
        EI_TRACE_VENC(EI_DBG_ERR, "EI_MI_VDEC_StartRecvStream faild with%#x! \n", s32Ret);
        return EI_FAILURE;
    }

    return EI_SUCCESS;
}



EI_S32 mdp_vdec_startGetFrame(EI_S32 s32ChnNum,mdp_vdec_thread_para_s *pstVdecSend, pthread_t *pVdecThread, VIDEO_FRAME_INFO_S *frame_info)
{
    EI_S32 s32Ret, buffer_cnt;
    VDEC_CHN_STATUS_S stStatus;

    buffer_cnt = 0;

#ifdef SAVE_JPG_DEC_YUV_SP
    char file_name[100];
    sprintf((char *)file_name, "%s/%s", PATH_ROOT, "mdp_jpeg_dec.yuv");
    FILE *flip_tmp = NULL;

    EI_TRACE_VDEC(EI_DBG_ERR, "file_name : %s\n", file_name);
    flip_tmp = fopen(file_name, "wb");
    if (flip_tmp == NULL) {
        EI_TRACE_VDEC(EI_DBG_ERR, "open file %s error\n", file_name);
        return -1;
    }
#endif

    while(1)
    {
        EI_MI_VDEC_QueryStatus(pstVdecSend[s32ChnNum].s32ChnId, &stStatus);
        if (stStatus.u32LeftPics == 0) {
            usleep(5 * 1000);
            continue;
        }

        s32Ret = EI_MI_VDEC_GetFrame(pstVdecSend[s32ChnNum].s32ChnId, frame_info, 1000);
        if(s32Ret != EI_SUCCESS){
            EI_TRACE_VDEC(EI_DBG_INFO, "get output buffer error\n");
            continue;
        }
#ifdef SAVE_JPG_DEC_YUV_SP
		s32Ret = EI_MI_VBUF_FrameMmap(frame_info, VBUF_REMAP_MODE_CACHED);
	    if (s32Ret != EI_SUCCESS)
	    {
	        EI_TRACE_VDEC(EI_DBG_ERR, "save yuv buffer error %x\n", s32Ret);
	        return -1;
	    }
		if (flip_tmp) {
            fwrite((void *)frame_info->stVFrame.ulPlaneVirAddr[0], 1,  frame_info->stVFrame.u32PlaneSizeValid[0], flip_tmp);
            fwrite((void *)frame_info->stVFrame.ulPlaneVirAddr[1], 1,  frame_info->stVFrame.u32PlaneSizeValid[1], flip_tmp);
        }
#endif
        buffer_cnt++;
#ifdef SAVE_JPG_DEC_YUV_SP
        EI_MI_VBUF_FrameMunmap(frame_info, VBUF_REMAP_MODE_CACHED);
        EI_TRACE_VDEC(EI_DBG_ERR, "decode cnt:%d  rece cnt: %d,send cnt:%d\n",
            stStatus.u32DecodeStreamFrames, buffer_cnt, pstVdecSend[s32ChnNum].frame_cnt);
#endif
        if(pstVdecSend[s32ChnNum].eThreadCtrl == THREAD_CTRL_STOP &&
           buffer_cnt >= pstVdecSend[s32ChnNum].frame_cnt)
        {
            EI_TRACE_VDEC(EI_DBG_ERR, "*getframe cnt:%d ,send packet:%d THREAD_CTRL_STOP EXIT!\n",
                                                buffer_cnt, pstVdecSend[s32ChnNum].frame_cnt-1);
            break;
        }

    }
#ifdef SAVE_JPG_DEC_YUV_SP
    fclose(flip_tmp);
#endif
    return 0;
}

int get_yuv_data(VIDEO_FRAME_INFO_S *frame_info)
{
	EI_S32 s32Ret;

	mdp_vdec_startSendStream(mdp_vdec_info.s32ChnNum, &mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum], mdp_vdec_info.VdecThread);
    s32Ret = mdp_vdec_startGetFrame(mdp_vdec_info.s32ChnNum, &mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum], &mdp_vdec_info.VdecThread[mdp_vdec_info.s32ChnNum], frame_info);

	return s32Ret;
}

int recg_img_dec_start(char *full_name)
{
	EI_S32 s32Ret;

	mdp_vdec_info.s32ChnNum = 0;
	mdp_vdec_info.config.enType          = PT_JPEG;
    mdp_vdec_info.config.enModId         = EI_ID_VPU;
    mdp_vdec_info.config.u32Align        = 32;
    mdp_vdec_info.config.u32Width        = 1280;
    mdp_vdec_info.config.u32Height       = 720;
    mdp_vdec_info.config.enPixelFormat   = PIX_FMT_YUV_SEMIPLANAR_420;
    mdp_vdec_info.config.enMode          = VIDEO_MODE_FRAME;
    mdp_vdec_info.config.u32FrameBufCnt  = 4;

    mdp_vdec_info.config.scaleInfo.u32MaxWidth   = 1280;
    mdp_vdec_info.config.scaleInfo.u32MaxHeight  = 720;
    mdp_vdec_info.config.scaleInfo.u32Align      = 2;
    mdp_vdec_info.config.scaleInfo.bScaleEnable  = EI_FALSE;
    mdp_vdec_info.config.scaleInfo.bFixedRatio   = EI_TRUE;
	EI_PRINT("full_name:%s\n", full_name);
	if (strlen(full_name) < 256 && strlen(full_name) > 0)
		strcpy(mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum].full_name, full_name);
	else {
		EI_TRACE_VDEC(EI_DBG_ERR, "full_name err\n");
		return -1;
	}
	EI_PRINT("mdp_vdec_info.stVdecSend[%d].full_name:%s\n", mdp_vdec_info.s32ChnNum, mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum].full_name);
    mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum].enType          = mdp_vdec_info.config.enType;
    mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum].s32StreamMode   = mdp_vdec_info.config.enMode;
    mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum].s32ChnId        = mdp_vdec_info.s32ChnNum;
    mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum].s32IntervalTime = 1500;
    mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum].u64PtsInit      = 0;
    mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum].u64PtsIncrease  = 1;
    mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum].eThreadCtrl     = THREAD_CTRL_START;
    mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum].bCircleSend     = EI_FAILURE;
    mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum].s32MilliSec     = 1000;
    mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum].s32MinBufSize   = (mdp_vdec_info.config.u32Width * mdp_vdec_info.config.u32Height * 3)>>1;
	s32Ret = mdp_vdec_start(mdp_vdec_info.s32ChnNum, &mdp_vdec_info.config);
    if(s32Ret != EI_SUCCESS)
    {
        EI_TRACE_VDEC(EI_DBG_ERR, "start VDEC fail for %#x!\n", s32Ret);
        return -1;
    }

	return s32Ret;
}
EI_S32 recg_img_dec_end(EI_S32 s32ChnNum, pthread_t *pVdecThread)
{
    EI_S32 s32Ret;

	s32Ret = EI_MI_VDEC_ReleaseFrame(mdp_vdec_info.stVdecSend[mdp_vdec_info.s32ChnNum].s32ChnId, &axframe_info.frame_info);
    if (s32Ret != 0){
        EI_TRACE_VDEC(EI_DBG_ERR, "release error\n");
    }
    s32Ret = EI_MI_VDEC_StopRecvStream(s32ChnNum);
    if(s32Ret != EI_SUCCESS){
        EI_TRACE_VDEC(EI_DBG_ERR, "EI_MI_VDEC_DestroyChn failed\n");
    }
    if(0 != pVdecThread[s32ChnNum])
    {
        pthread_join(pVdecThread[s32ChnNum], EI_NULL);
        pVdecThread[s32ChnNum] = 0;
    }
    else {
        EI_TRACE_VDEC(EI_DBG_ERR, "Mdp_Vdec_StopSendStream FAILURE\n");
        return EI_FAILURE;
    }
    s32Ret = EI_MI_VDEC_DestroyChn(s32ChnNum);
    if(s32Ret != EI_SUCCESS){
        EI_TRACE_VDEC(EI_DBG_ERR, "EI_MI_VDEC_DestroyChn failed\n");
    }
    EI_TRACE_VDEC(EI_DBG_ERR, "Mdp_Vdec_StopSendStream SUCCESS\n");
    return EI_SUCCESS;
}

static int check_face_in_limit_rect(int box[])
{
	if ((box[0] > 0) &&
		(box[1] > 0)
		&& (box[2] < (axframe_info.u32Width - 1))
		&& (box[3] < (axframe_info.u32Height - 1))) {

		return 0;
	}
	EI_PRINT("box: %d %d %d %d\n", box[0], box[1], box[2], box[3]);
	return -1;
}

void storage_info_get(const char *mount_point, sdcard_info_t *info)
{
    struct statfs disk_info;
    int blksize;
    FILE *fp;
    char split[] = " ";
    char linebuf[512];
    char *result = NULL;
    int mounted = 0;
    char mount_path[128];
    int len;
    len = strlen(mount_point);
    if (!len)
        return;
    strcpy(mount_path, mount_point);
    if (mount_path[len - 1] == '/')
        mount_path[len - 1] = '\0';
    fp = fopen("/proc/mounts", "r");
    if (fp == NULL) {
        EI_TRACE_LOG(EI_DBG_ERR, "open error mount proc");
        info->total = 0;
        info->free = 0;
        return;
    }
    while (fgets(linebuf, 512, fp)) {
        result = strtok(linebuf, split);
        result = strtok(NULL, split);
        if (!strncmp(result, mount_path, strlen(mount_path))) {
            mounted = 1;
            break;
        }
    }
    fclose(fp);
    if (mounted ==  0) {
        info->total = 0;
        info->free = 0;
        return;
    }

    memset(&disk_info, 0, sizeof(struct statfs));
    if (statfs(mount_path, &disk_info) < 0)
        return;
    if (disk_info.f_bsize >= (1 << 10)) {
        info->total = ((unsigned int)(disk_info.f_bsize >> 10) *
                disk_info.f_blocks);
        info->free = ((unsigned int)(disk_info.f_bsize >> 10) *
                disk_info.f_bfree);
    } else if (disk_info.f_bsize > 0) {
        blksize = ((1 << 10) / disk_info.f_bsize);
        info->total = (disk_info.f_blocks / blksize);
        info->free = (disk_info.f_bfree / blksize);
    }
}

void get_stream_file_name(sdr_sample_venc_para_t *sdr_venc_param, char szFilePostfix[32], int len)
{
    static sdcard_info_t info;
	struct tm *p_tm; /* time variable */
	time_t now;
	now = time(NULL);
	p_tm = localtime(&now);

    if (sdr_venc_param == NULL)
        return;
    storage_info_get(PATH_ROOT, &info);
    while (info.free < 10 * 1024) {
		EI_PRINT("storage not enough %d\n", info.free);
		usleep(1000*1000);
		storage_info_get(PATH_ROOT, &info);
    }
	memset(sdr_venc_param->szFilePostfix, 0x00, sizeof(sdr_venc_param->szFilePostfix));
	memset(sdr_venc_param->aszFileName, 0x00, sizeof(sdr_venc_param->aszFileName));
	memcpy(sdr_venc_param->szFilePostfix, szFilePostfix, len);
    sprintf(sdr_venc_param->aszFileName,
		"%s/stream_chn%02d-%02d%02d%02d%02d%02d%02d%s", PATH_ROOT, sdr_venc_param->VeChn,
		p_tm->tm_year + 1900, p_tm->tm_mon + 1, p_tm->tm_mday,
		p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec, sdr_venc_param->szFilePostfix);
	EI_PRINT("venc_para->aszFileName:%s\n", sdr_venc_param->aszFileName);
}

static int facerecg_frame_det(void *hdl, VIDEO_FRAME_INFO_S *frame)
{
	EI_S32 s32Ret;
	int get_feature_flag;

	if (hdl == NULL || frame == NULL) {
		EI_PRINT("parameter p is NULL\n");
		return -1;
	}

	s32Ret = EI_MI_VBUF_FrameMmap(frame, VBUF_REMAP_MODE_CACHED);
    if (s32Ret != EI_SUCCESS)
    {
        EI_TRACE_VDEC(EI_DBG_ERR, "save yuv buffer error %x\n", s32Ret);
        return -1;
    }

    axframe_info.recg_in.img.y_phy = (void *)frame->stVFrame.u64PlanePhyAddr[0];
    axframe_info.recg_in.img.y = (void *)frame->stVFrame.ulPlaneVirAddr[0];
    axframe_info.recg_in.img.uv_phy = (void *)frame->stVFrame.u64PlanePhyAddr[1];
    axframe_info.recg_in.img.uv = (void *)frame->stVFrame.ulPlaneVirAddr[1];
    axframe_info.recg_in.img.width = axframe_info.u32Width;
    axframe_info.recg_in.img.height = axframe_info.u32Height;
    axframe_info.recg_in.img.format = YUV420_SP;
	axframe_info.recg_in.interest_roi[0] = 0;
	axframe_info.recg_in.interest_roi[1] = 0;
	axframe_info.recg_in.interest_roi[2] = axframe_info.recg_in.img.width - 1;
	axframe_info.recg_in.interest_roi[3] = axframe_info.recg_in.img.height - 1;

	s32Ret = -1;
	nn_facerecg_detect(hdl, &axframe_info.recg_in, &axframe_info.faces);
	EI_PRINT("axframe_info.faces.size:%d  action:%d %d\n", axframe_info.faces.size,
		axframe_info.action, axframe_info.nn_facerecg_det_count);
	if (axframe_info.action == REGISTER_FACE_CAM) {
		if (axframe_info.reg_flag) {
			EI_MI_VBUF_FrameMunmap(frame, VBUF_REMAP_MODE_CACHED);
			return 0;
		}
		if (axframe_info.faces.size == 1) {
			int ck = check_face_in_limit_rect(axframe_info.faces.p[0].box);
			EI_PRINT("ck : %d %d %d %d\n", ck, axframe_info.nn_facerecg_det_count,
				axframe_info.nn_facerecg_det_too_big_count, axframe_info.nn_facerecg_det_too_small_count);
			if (ck == -1) {
				if (axframe_info.nn_facerecg_det_too_big_count > 300) {
					osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_CAM_REG_FCACE_TOO_BIG, NULL, 0);
					axframe_info.nn_facerecg_det_too_big_count = 0;
				}
				axframe_info.nn_facerecg_det_too_big_count++;
				axframe_info.nn_facerecg_det_count = 0;
				axframe_info.nn_facerecg_det_too_small_count = 0;
			} else if (axframe_info.nn_facerecg_det_count == 0) {
				axframe_info.m_face_rect[0] = axframe_info.faces.p[0].box[0];
				axframe_info.m_face_rect[1] = axframe_info.faces.p[0].box[1];
				axframe_info.m_face_rect[2] = axframe_info.faces.p[0].box[2];
				axframe_info.m_face_rect[3] = axframe_info.faces.p[0].box[3];
				axframe_info.nn_facerecg_det_count++;
			} else {
				nn_facerecg_feature_t face_feature;
				int width = axframe_info.faces.p[0].box[2] - axframe_info.faces.p[0].box[0];
				int height = axframe_info.faces.p[0].box[3] - axframe_info.faces.p[0].box[1];
				if ((width > FACE_REGISTER_MIN_PIX) && (width < FACE_REGISTER_MAX_PIX)
					&& (height > FACE_REGISTER_MIN_PIX) && (height < FACE_REGISTER_MAX_PIX)) {
					axframe_info.nn_facerecg_det_too_big_count = 0;
					axframe_info.nn_facerecg_det_too_small_count = 0;
					get_feature_flag = nn_facerecg_get_feature(hdl,
						&axframe_info.recg_in, &axframe_info.faces.p[0], &face_feature);
					if (get_feature_flag == 0) {
						EI_PRINT("get_feature_flag:%d\n", get_feature_flag);
						int t1 = compare_distance((float)axframe_info.m_face_rect[0],
							(float)axframe_info.m_face_rect[2],
								(float)axframe_info.faces.p[0].box[0], (float)axframe_info.faces.p[0].box[2]);
						if (t1 == 1) {
							if(axframe_info.nn_facerecg_det_count == COUNT_REGISTER_FACE_FRAME){
								axframe_info.features.size = axframe_info.faces.size;
								axframe_info.features.p = &face_feature;
								insertData(axframe_info.features.p[0].norm,
									axframe_info.features.p[0].feature_len, axframe_info.features.p[0].feature);
								s32Ret = EI_SUCCESS;
#ifdef SAVE_REGISTER_FACE_CAM_FRAME
								get_stream_file_name(&stream_para[0], ".nv12", sizeof(".nv12"));
								stream_para[0].flip_out = fopen(stream_para[0].aszFileName, "wb");
								if (stream_para[0].flip_out == NULL) {
									EI_TRACE_VDEC(EI_DBG_ERR, "open file %s error\n", stream_para[0].aszFileName);
									return -1;
								}
								if (stream_para[0].flip_out) {
						            fwrite((void *)frame->stVFrame.ulPlaneVirAddr[0], 1,
										axframe_info.recg_in.img.width*axframe_info.recg_in.img.height, stream_para[0].flip_out);
						            fwrite((void *)frame->stVFrame.ulPlaneVirAddr[1], 1,
										axframe_info.recg_in.img.width*axframe_info.recg_in.img.height/2, stream_para[0].flip_out);
						        }
								fclose(stream_para[0].flip_out);
#endif
							} else {
								axframe_info.nn_facerecg_det_count ++;
							}
						} else {
							axframe_info.nn_facerecg_det_count = 0;
						}
					} else {
						axframe_info.nn_facerecg_det_count = 0;
					}
				} else {
					if (width <= FACE_REGISTER_MIN_PIX || height <= FACE_REGISTER_MIN_PIX) {
						if (axframe_info.nn_facerecg_det_too_small_count > 300) {
							osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_CAM_REG_FCACE_TOO_SMALL, NULL, 0);
							axframe_info.nn_facerecg_det_too_small_count = 0;
						}
						axframe_info.nn_facerecg_det_too_big_count = 0;
						axframe_info.nn_facerecg_det_too_small_count++;
						EI_PRINT("face is too small please keep close: w:%d h:%d\n", width, height);
					} else {
						EI_PRINT("face is too big please don't keep so close:w:%d h:%d\n", width, height);
						if (axframe_info.nn_facerecg_det_too_big_count > 300) {
							osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_CAM_REG_FCACE_TOO_BIG, NULL, 0);
							axframe_info.nn_facerecg_det_too_big_count = 0;
						}
						axframe_info.nn_facerecg_det_too_small_count = 0;
						axframe_info.nn_facerecg_det_too_big_count++;
					}
					axframe_info.nn_facerecg_det_count = 0;
				}
			} 
		}else {
			axframe_info.nn_facerecg_det_count = 0;
		}	
	} else if (axframe_info.action == RECOGNITION_FACE) {
		int match_index = 0;
		axframe_info.match_score = 0.0;
		axframe_info.features.size = axframe_info.faces.size;
		if (axframe_info.features.size > 0) {
			axframe_info.features.p = (nn_facerecg_feature_t *)malloc(sizeof(nn_facerecg_feature_t)*axframe_info.features.size);
		}
		for (int i = 0; i<axframe_info.faces.size; i++) {
			nn_facerecg_match_result_t match_temp;
			get_feature_flag = nn_facerecg_get_feature(hdl, &axframe_info.recg_in,
				&axframe_info.faces.p[i], &axframe_info.features.p[i]);
			if (get_feature_flag == 0) {
				EI_PRINT("axframe_info.features.p[%d].feature_len:%d, %f\n", i, axframe_info.features.p[i].feature_len, axframe_info.features.p[i].norm);
				EI_PRINT("match_features.p[%d].feature: ", i);
				for (int l=0; l<axframe_info.match_features->p[l].feature_len; l++) {
					EI_PRINT("%d ", axframe_info.features.p[i].feature[l]);
				}
				EI_PRINT("\n");
				nn_facerecg_match(hdl, &axframe_info.features.p[i], axframe_info.match_features, &match_temp);
				for (int j=0; j<axframe_info.match_features->size; j++) {
					if (match_temp.score[j] > axframe_info.match_score) {
						axframe_info.match_score = match_temp.score[j];
						match_index = match_temp.max_index;
					}
					EI_PRINT("i:%d, j: %d, match_temp score:%f, max_index:%d\n", i, j, match_temp.score[j], match_temp.max_index);
					EI_PRINT("axframe_info.match_features->p[%d].feature_len %d, %f\n", j, axframe_info.match_features->p[j].feature_len, axframe_info.match_features->p[j].norm);
					EI_PRINT("match_features.p[%d].feature: ", j);
					for (int k=0; k<axframe_info.match_features->p[j].feature_len; k++) {
						EI_PRINT("%d ", axframe_info.match_features->p[j].feature[k]);
					}
					EI_PRINT("\n");
				}
			}
		}
		EI_PRINT("match_index:%d, match_score:%f\n", match_index, axframe_info.match_score);
		if (axframe_info.match_score > 0.45) {
			s32Ret = 0;
			axframe_info.match_id = m_all_info.info[match_index].id;
			if (strlen(m_all_info.info[match_index].name) &&
				(strlen(m_all_info.info[match_index].name) < 128)) {
				strcpy(axframe_info.match_name, m_all_info.info[match_index].name);
			} else {
				EI_PRINT("match name err %d\n", strlen(m_all_info.info[match_index].name));
			}
			EI_PRINT("match id:%d, name:%s\n", axframe_info.match_id, axframe_info.match_name);
		}
		if (axframe_info.features.size > 0) {
			free(axframe_info.features.p);
		}
	}
	EI_MI_VBUF_FrameMunmap(frame, VBUF_REMAP_MODE_CACHED);
#ifndef SAVE_REGISTER_FACE_CAM_FRAME
	if (s32Ret == EI_SUCCESS && axframe_info.action == REGISTER_FACE_CAM) {
		VENC_STREAM_S stStream;
		VENC_CHN_STATUS_S stStatus;
		frame->stVencFrameInfo.bEnableScale = EI_TRUE;

		s32Ret = EI_MI_VENC_SendFrame(stream_para[0].VeChn, frame, 1000);
		if (s32Ret != EI_SUCCESS) {
			EI_PRINT("send frame error\n");
		}
		get_stream_file_name(&stream_para[0], ".jpg", sizeof(".jpg"));
		s32Ret = EI_MI_VENC_QueryStatus(stream_para[0].VeChn, &stStatus);
		if (s32Ret != EI_SUCCESS) {
			EI_TRACE_VENC(EI_DBG_ERR, "query status chn-%d error : %d\n", stream_para[0].VeChn, s32Ret);
			s32Ret = EI_SUCCESS;
			return s32Ret;
		}

		s32Ret = EI_MI_VENC_GetStream(stream_para[0].VeChn, &stStream, 1000);

		if (s32Ret == EI_ERR_VENC_NOBUF) {
			EI_TRACE_VENC(EI_DBG_INFO, "No buffer\n");
			s32Ret = EI_SUCCESS;
			return s32Ret;
		} else if (s32Ret != EI_SUCCESS) {
			EI_TRACE_VENC(EI_DBG_ERR, "get venc chn-%d error : %d\n", stream_para[0].VeChn, s32Ret);
			s32Ret = EI_SUCCESS;
			return s32Ret;;
		}

		stream_para[0].flip_out = fopen(stream_para[0].aszFileName, "wb");
		if (stream_para[0].flip_out == NULL) {
			EI_MI_VENC_ReleaseStream(stream_para[0].VeChn, &stStream);
			EI_TRACE_VENC(EI_DBG_ERR, "open file[%s] failed!\n", stream_para[0].aszFileName);
			s32Ret = EI_SUCCESS;
			return s32Ret;
		} else {
			if ((stStream.pstPack.pu8Addr[0] != EI_NULL) && (stStream.pstPack.u32Len[0] != 0)
				&& stream_para[0].flip_out != NULL) {
				fwrite(stStream.pstPack.pu8Addr[0], 1, stStream.pstPack.u32Len[0], stream_para[0].flip_out);
				if ((stStream.pstPack.pu8Addr[1] != EI_NULL) && (stStream.pstPack.u32Len[1] != 0)) {
					fwrite(stStream.pstPack.pu8Addr[1], 1, stStream.pstPack.u32Len[1], stream_para[0].flip_out);
				}
			} else {
				EI_TRACE_VENC(EI_DBG_ERR, "save stream failed!\n");
				s32Ret = EI_SUCCESS;
				return s32Ret;;
			}
			if (s32Ret != EI_SUCCESS) {
				EI_MI_VENC_ReleaseStream(stream_para[0].VeChn, &stStream);
				EI_TRACE_VENC(EI_DBG_ERR, "save venc chn-%d error : %d\n", stream_para[0].VeChn, s32Ret);
				s32Ret = EI_SUCCESS;
				return s32Ret;
			}
			EI_PRINT("\nGenerate %s \n", stream_para[0].aszFileName);
			fclose(stream_para[0].flip_out);
			stream_para[0].flip_out = NULL;
		}

		s32Ret = EI_MI_VENC_ReleaseStream(stream_para[0].VeChn, &stStream);
		if (s32Ret != EI_SUCCESS) {
			EI_TRACE_VENC(EI_DBG_ERR, "release venc chn-%d error : %d\n", stream_para[0].VeChn, s32Ret);
			s32Ret = EI_SUCCESS;
			return s32Ret;
		}
	}
#endif
	return s32Ret;
}

void stop_nn_facerecg_fun()
{
	nn_facerecg_release(axframe_info.nn_hdl);
}

int facerecg_get_face_features(char *vdec_full_name, nn_facerecg_feature_t *feature)
{
	int ret =-1;

	if (feature == NULL || vdec_full_name == NULL) {
		EI_PRINT("please stop facerecg_svc!");
		return ret;
	}
	start_nn_facerecg_fun(axframe_info.action);
	recg_img_dec_start(vdec_full_name);
	memset(&axframe_info.faces, 0, sizeof(axframe_info.faces));
	memset(&axframe_info.frame_info, 0, sizeof(axframe_info.frame_info));
	if (get_yuv_data(&axframe_info.frame_info)) {
		EI_PRINT("get_yuv_data failed!");
		nn_facerecg_release(axframe_info.nn_hdl);
		return ret;
	}
	
	ret = EI_MI_VBUF_FrameMmap(&axframe_info.frame_info, VBUF_REMAP_MODE_CACHED);
    if (ret != EI_SUCCESS)
    {
        EI_TRACE_VDEC(EI_DBG_ERR, "EI_MI_VBUF_FrameMmap %x\n", ret);
        return -1;
    }

	axframe_info.recg_in.img.y_phy = (void *)axframe_info.frame_info.stVFrame.u64PlanePhyAddr[0];
	axframe_info.recg_in.img.y = (void *)axframe_info.frame_info.stVFrame.ulPlaneVirAddr[0];
	axframe_info.recg_in.img.uv_phy = (void *)axframe_info.frame_info.stVFrame.u64PlanePhyAddr[1];
	axframe_info.recg_in.img.uv = (void *)axframe_info.frame_info.stVFrame.ulPlaneVirAddr[1];
	axframe_info.recg_in.img.width = 1280;
	axframe_info.recg_in.img.height = 720;
	axframe_info.recg_in.img.format = YUV420_SP;
	axframe_info.recg_in.interest_roi[0] = 0;
	axframe_info.recg_in.interest_roi[1] = 0;
	axframe_info.recg_in.interest_roi[2] = axframe_info.recg_in.img.width - 1;
	axframe_info.recg_in.interest_roi[3] = axframe_info.recg_in.img.height - 1;
	nn_facerecg_detect(axframe_info.nn_hdl, &axframe_info.recg_in, &axframe_info.faces);
	ret = -1;
	EI_PRINT("faces.size %d\n", axframe_info.faces.size);
	if (axframe_info.faces.size == 1) {
		int width = axframe_info.faces.p[0].box[2] - axframe_info.faces.p[0].box[0];
		int height = axframe_info.faces.p[0].box[3] - axframe_info.faces.p[0].box[1];
		if ((width > FACE_REGISTER_MIN_PIX) && (width < FACE_REGISTER_MAX_PIX)
			&& (height > FACE_REGISTER_MIN_PIX) && (height < FACE_REGISTER_MAX_PIX)) {
			nn_facerecg_get_feature(axframe_info.nn_hdl, &axframe_info.recg_in, &axframe_info.faces.p[0], feature);
			ret = 0;
			EI_PRINT("feature_len:%d, norm:%f, feature:%s\n",
				feature->feature_len, feature->norm, feature->feature);
		}
	} else {
		ret = -1;
	}
	EI_MI_VBUF_FrameMunmap(&axframe_info.frame_info, VBUF_REMAP_MODE_CACHED);
	recg_img_dec_end(mdp_vdec_info.s32ChnNum, mdp_vdec_info.VdecThread);
	stop_nn_facerecg_fun();
	
	return ret;

}

EI_VOID *get_axframe_proc(EI_VOID *p)
{
    int ret;
    VIDEO_FRAME_INFO_S axFrame = {0};
    axframe_info_s *axframe_info;
	nn_facerecg_feature_t face_feature;

    axframe_info = (axframe_info_s *)p;
    EI_PRINT("get_axframe_proc bThreadStart = %d\n", axframe_info->bThreadStart);
#ifdef SAVE_AX_YUV_SP
    char srcfilename[128];
    sprintf(srcfilename, "%s/facerecg_raw_ch%d.yuv", PATH_ROOT, axframe_info->chn);
    axframe_info->ax_fout = fopen(srcfilename, "wb+");
    if (axframe_info->ax_fout == NULL) {
        EI_TRACE_VDEC(EI_DBG_ERR, "file open error1\n");
    }
#endif
    EI_PRINT("********start_facerecg_nn******\n");
	warning_info.last_warn_time = osal_get_msec();
	open_sqlitedb();
	if (axframe_info->action == REGISTER_FACE_IMG) {
		EI_PRINT("mdp_vdec_info.vdec_full_name:%s\n", mdp_vdec_info.vdec_full_name);
		osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_IMG_REG_START, NULL, 0);
		if (facerecg_get_face_features(mdp_vdec_info.vdec_full_name, &face_feature) == 0) {
			EI_PRINT(" norm:%f, feature_len:%d, feature:%s\n", face_feature.norm, face_feature.feature_len, face_feature.feature);
			insertData(face_feature.norm, face_feature.feature_len, face_feature.feature);
			osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_REG_SUCCESS, NULL, 0);
		} else {
			osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_REG_FAIL, NULL, 0);
		}
		axframe_info->bThreadStart = EI_FALSE;
		goto exit; 
	} else if (axframe_info->action == REGISTER_FACE_CAM) {
		osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_CAM_REG_START, NULL, 0);
	}
	else if (axframe_info->action == RECOGNITION_FACE) {
		osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_MATCH_START, NULL, 0);
		selectDb();
		axframe_info->match_features = &m_all_info.m_face_features;
	}
	start_nn_facerecg_fun(axframe_info->action);
    while (EI_TRUE == axframe_info->bThreadStart) {
        memset(&axFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = EI_MI_VISS_GetChnFrame(axframe_info->dev,
            axframe_info->chn, &axFrame, 1000);
        if (ret != EI_SUCCESS) {
            EI_PRINT("chn %d get frame error 0x%x\n", axframe_info->chn, ret);
            usleep(100 * 1000);
            continue;
        }
        ret = facerecg_frame_det(axframe_info->nn_hdl, &axFrame);
        EI_MI_VISS_ReleaseChnFrame(axframe_info->dev,
            axframe_info->chn, &axFrame);
		if (ret == EI_SUCCESS && axframe_info->action == REGISTER_FACE_CAM) {
			stream_para[0].exit_flag = EI_TRUE;
			axframe_info->reg_flag = 1;
			osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_REG_SUCCESS, NULL, 0);
			break;
		} else if ((axframe_info->action == REGISTER_FACE_CAM) &&
		((osal_get_msec()-warning_info.last_warn_time) > REG_FACE_FAIL_ALARM_INTERVAL)) {
			osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_REG_FAIL, NULL, 0);
			warning_info.last_warn_time = osal_get_msec();
		}
		if (ret == EI_SUCCESS && axframe_info->action == RECOGNITION_FACE) {
			stream_para[0].exit_flag = EI_TRUE;
			osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_MATCH_SUCCESS, NULL, 0);
			break;
		} else if ((axframe_info->action == RECOGNITION_FACE) &&
		((osal_get_msec()-warning_info.last_warn_time) > REG_FACE_FAIL_ALARM_INTERVAL)) {
			osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_MATCH_FAIL, NULL, 0);
			warning_info.last_warn_time = osal_get_msec();
		}
    }
    EI_MI_VISS_DisableChn(axframe_info->dev, axframe_info->chn);
	stop_nn_facerecg_fun();
exit:
	close_sqlitedb();
#ifdef SAVE_AX_YUV_SP
    if (axframe_info->ax_fout) {
        fclose(axframe_info->ax_fout);
        axframe_info->ax_fout = NULL;
    }
#endif
    return NULL;
}
EI_VOID *get_drawframe_proc(EI_VOID *p)
{
    int ret;
    VIDEO_FRAME_INFO_S drawFrame = {0};
    drawframe_info_s *drawframe_info;

    char srcfilename[128];
    drawframe_info = (drawframe_info_s *)p;
    EI_PRINT("get_frame_proc bThreadStart = %d\n", drawframe_info->bThreadStart);

    sprintf(srcfilename, "%s/draw_raw_ch%d.yuv", PATH_ROOT, drawframe_info->chn);
    EI_PRINT("********get_drawframe_proc******\n");
#ifdef SAVE_DRAW_YUV_SP
    drawframe_info->draw_fout = fopen(srcfilename, "wb+");
    if (drawframe_info->draw_fout == NULL) {
        EI_TRACE_VDEC(EI_DBG_ERR, "file open error1\n");
    }
#endif
    while (EI_TRUE == drawframe_info->bThreadStart) {
		if (axframe_info.action == REGISTER_FACE_IMG) {
			if (axframe_info.faces.size == 1) {
				memcpy(&drawFrame, &axframe_info.frame_info, sizeof(drawFrame));
				drawFrame.enFrameType = MDP_FRAME_TYPE_DOSS;
		        EI_MI_VO_SendFrame(0, 0, &drawFrame, 100);
			}
		} else {
	        memset(&drawFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
	        ret = EI_MI_VISS_GetChnFrame(drawframe_info->dev,
	            drawframe_info->chn, &drawFrame, 1000);
	        if (ret != EI_SUCCESS) {
	            EI_PRINT("chn %d get frame error 0x%x\n", drawframe_info->chn, ret);
	            usleep(100 * 1000);
	            continue;
	        }
			facerecg_draw_location_rgn(&drawFrame, &axframe_info.faces);
	        drawFrame.enFrameType = MDP_FRAME_TYPE_DOSS;
	        EI_MI_VO_SendFrame(0, 0, &drawFrame, 100);
	        EI_MI_VISS_ReleaseChnFrame(drawframe_info->dev,
	            drawframe_info->chn, &drawFrame);
		}
    }
    EI_MI_VISS_DisableChn(drawframe_info->dev, drawframe_info->chn);
#ifdef SAVE_DRAW_YUV_SP
    if (drawframe_info->draw_fout) {
        fclose(drawframe_info->draw_fout);
        drawframe_info->draw_fout = NULL;
    }
#endif

    return NULL;
}
void warn_tone_play(char *full_path)
{
	char sys_cmd[256];
	if(strlen(full_path)>249) {
		EI_PRINT("path %s too long\n", full_path);
		return;
	}
	memset(sys_cmd, 0x00, sizeof(sys_cmd));
	sprintf(sys_cmd, "aplay %s", full_path);
	system(sys_cmd);
	return;
}
EI_VOID *warning_proc(EI_VOID *p)
{
	int			err = 0;
	tag_warn_msg_t		msg;
    warning_info_s *warning_info = (warning_info_s *)p;
 
    EI_PRINT("get_frame_proc bThreadStart = %d\n", warning_info->bThreadStart);
	prctl(PR_SET_NAME, "warning_proc thread");

	while (warning_info->bThreadStart) {
		err = osal_mq_recv(warning_info->mq_id, &msg, MQ_MSG_LEN, 50);
		if (err) {
			continue;
		}
		EI_PRINT("msg.type = %d\n", msg.type);

		switch (msg.type) {
			case LB_WARNING_FACERECG_CAM_REG_START:
				warn_tone_play("/usr/share/out/res/facerecg_warning/shexiangtourenlianzhucekaishi.wav");
				break;
			case LB_WARNING_FACERECG_IMG_REG_START:
				warn_tone_play("/usr/share/out/res/facerecg_warning/tupianrenlianzhucekaishi.wav");
				break;
			case LB_WARNING_FACERECG_REG_SUCCESS:
				warn_tone_play("/usr/share/out/res/facerecg_warning/renlianzhucechenggong.wav");
				break;
			case LB_WARNING_FACERECG_REG_FAIL:
				warn_tone_play("/usr/share/out/res/facerecg_warning/renlianzhuceshibai.wav");
				break;
			case LB_WARNING_FACERECG_CAM_REG_FCACE_TOO_BIG:
				warn_tone_play("/usr/share/out/res/facerecg_warning/renliantaida.wav");
				break;
			case LB_WARNING_FACERECG_CAM_REG_FCACE_TOO_SMALL:
				warn_tone_play("/usr/share/out/res/facerecg_warning/renliantaixiao.wav");
				break;
			case LB_WARNING_FACERECG_MATCH_START:
				warn_tone_play("/usr/share/out/res/facerecg_warning/jiashiyuanpipeikaishi.wav");
				break;
			case LB_WARNING_FACERECG_MATCH_SUCCESS:
				warn_tone_play("/usr/share/out/res/facerecg_warning/jiashiyuanpipeichenggong.wav");
				break;
			case LB_WARNING_FACERECG_MATCH_FAIL:
				warn_tone_play("/usr/share/out/res/facerecg_warning/jiashiyuanpipeishibai.wav");
				break;
			default:
				break;
		}
	}

    return NULL;
}

static int start_get_axframe(axframe_info_s *axframe_info)
{
    int ret;
    static VISS_EXT_CHN_ATTR_S stExtChnAttr;

    stExtChnAttr.s32BindChn = axframe_info->phyChn;
    stExtChnAttr.u32Depth = 1;
    stExtChnAttr.stSize.u32Width = axframe_info->u32Width;
    stExtChnAttr.stSize.u32Height = axframe_info->u32Height;
    stExtChnAttr.u32Align = 32;
    stExtChnAttr.enPixFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stExtChnAttr.stFrameRate.s32SrcFrameRate = axframe_info->frameRate;
    stExtChnAttr.stFrameRate.s32DstFrameRate = 15;
    stExtChnAttr.s32Position = 1;

    ret = EI_MI_VISS_SetExtChnAttr(axframe_info->dev, axframe_info->chn, &stExtChnAttr);
    if (ret != EI_SUCCESS) {
        EI_PRINT("EI_MI_VISS_SetExtChnAttr failed with %#x!\n", ret);
        return EI_FAILURE;
    }
    ret = EI_MI_VISS_EnableChn(axframe_info->dev, axframe_info->chn);
    if (ret != EI_SUCCESS) {
        EI_PRINT("EI_MI_VISS_EnableChn failed with %#x!\n", ret);
        return EI_FAILURE;
    }

    axframe_info->bThreadStart = EI_TRUE;
    ret = pthread_create(&(axframe_info->gs_framePid), NULL, get_axframe_proc,
        (EI_VOID *)axframe_info);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
    EI_PRINT("start_get_axframe success %#x! phyChn:%d chn :%d dev:%d\n", ret, axframe_info->phyChn, axframe_info->chn, axframe_info->dev);

    return EI_SUCCESS;
}
static int start_get_drawframe(drawframe_info_s *drawframe_info)
{
    int ret;
    static VISS_EXT_CHN_ATTR_S stExtChnAttr;

    stExtChnAttr.s32BindChn = drawframe_info->phyChn;
    stExtChnAttr.u32Depth = 1;
    stExtChnAttr.stSize.u32Width = drawframe_info->u32Width;
    stExtChnAttr.stSize.u32Height = drawframe_info->u32Height;
    stExtChnAttr.u32Align = 32;
    stExtChnAttr.enPixFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stExtChnAttr.stFrameRate.s32SrcFrameRate = drawframe_info->frameRate;
    stExtChnAttr.stFrameRate.s32DstFrameRate = drawframe_info->frameRate;
    stExtChnAttr.s32Position = 1;

    ret = EI_MI_VISS_SetExtChnAttr(drawframe_info->dev, drawframe_info->chn, &stExtChnAttr);
    if (ret != EI_SUCCESS) {
        EI_PRINT("EI_MI_VISS_SetExtChnAttr failed with %#x!\n", ret);
        return EI_FAILURE;
    }
    ret = EI_MI_VISS_EnableChn(drawframe_info->dev, drawframe_info->chn);
    if (ret != EI_SUCCESS) {
        EI_PRINT("EI_MI_VISS_EnableChn failed with %#x!\n", ret);
        return EI_FAILURE;
    }

    drawframe_info->bThreadStart = EI_TRUE;
    ret = pthread_create(&(drawframe_info->gs_framePid), NULL, get_drawframe_proc,
        (EI_VOID *)drawframe_info);
    if (ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
    EI_PRINT("start_get_drawframe success %#x! phyChn:%d chn :%d dev:%d\n", ret, drawframe_info->phyChn, drawframe_info->chn, drawframe_info->dev);

    return EI_SUCCESS;
}

void stop_get_frame(int signo)
{
	stream_para[0].exit_flag = EI_TRUE;

    printf("%s %d, signal %d\n", __func__, __LINE__, signo);
   	printf("stop_get_frame end start!\n");
}

static int show_1ch(VISS_DEV	dev, VISS_CHN chn, ROTATION_E rot, EI_BOOL mirror, EI_BOOL flip,
    unsigned int rect_x, unsigned int rect_y, unsigned int width, unsigned int hight,
    SNS_TYPE_E sns)
{
	static VBUF_CONFIG_S stVbConfig;
	static SAMPLE_VISS_CONFIG_S stVissConfig;
    EI_S32 s32Ret = EI_FAILURE;
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr = {0};
    VO_CHN_ATTR_S stVoChnAttr = {0};
	SAMPLE_VENC_CONFIG_S stSnapJVC;

    if (((sns >= TP9930_DVP_1920_1080_25FPS_1CH_YUV) &&
    	(sns <= TP9930_DVP_1920_1080_XXFPS_4CH_YUV)) ||
    	((sns >= TP2815_MIPI_1920_1080_25FPS_4CH_YUV) &&
    	(sns <= TP2815_MIPI_1920_1080_30FPS_4CH_YUV))) {
	    stVbConfig.astFrameInfo[0].enFrameType = MDP_FRAME_TYPE_COMMON;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Align = 32;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Width = 1920;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Height = 1080;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
	    stVbConfig.astCommPool[0].u32BufCnt = 40;
	    stVbConfig.astCommPool[0].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    } else {
	    stVbConfig.astFrameInfo[0].enFrameType = MDP_FRAME_TYPE_COMMON;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Align = 32;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Width = 1280;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.u32Height = 720;
	    stVbConfig.astFrameInfo[0].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
	    stVbConfig.astCommPool[0].u32BufCnt = 40;
	    stVbConfig.astCommPool[0].enRemapMode = VBUF_REMAP_MODE_NOCACHE;
    }
    stVbConfig.astFrameInfo[1].enFrameType = MDP_FRAME_TYPE_COMMON;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Align = 32;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width = 1280;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height = 720;
    stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat = PIX_FMT_YUV_SEMIPLANAR_420;
    stVbConfig.astCommPool[1].u32BufCnt = 48;
    stVbConfig.astCommPool[1].enRemapMode = VBUF_REMAP_MODE_CACHED;
    stVbConfig.u32PoolCnt = 2;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConfig);
    if (s32Ret) {
        goto exit0;
    }

    stVissConfig.astVissInfo[0].stDevInfo.VissDev = dev; /*0: DVP, 1: MIPI*/
    stVissConfig.astVissInfo[0].stDevInfo.aBindPhyChn[0] = 0;
    stVissConfig.astVissInfo[0].stDevInfo.aBindPhyChn[1] = 1;
    stVissConfig.astVissInfo[0].stDevInfo.aBindPhyChn[2] = 2;
    stVissConfig.astVissInfo[0].stDevInfo.aBindPhyChn[3] = 3;
    stVissConfig.astVissInfo[0].stDevInfo.enOutPath = VISS_OUT_PATH_DMA;
    stVissConfig.astVissInfo[0].stChnInfo.aVissChn[0] = 0;
    stVissConfig.astVissInfo[0].stChnInfo.aVissChn[1] = 1;
    stVissConfig.astVissInfo[0].stChnInfo.aVissChn[2] = 2;
    stVissConfig.astVissInfo[0].stChnInfo.aVissChn[3] = 3;
    stVissConfig.astVissInfo[0].stChnInfo.enWorkMode = VISS_WORK_MODE_4Chn;
    stVissConfig.astVissInfo[0].stIspInfo.IspDev = 0;
    stVissConfig.astVissInfo[0].stIspInfo.enRunningMode = ISP_MODE_RUNNING_OFFLINE;
    stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.u32Align = 32;
    stVissConfig.astVissInfo[0].stSnsInfo.enSnsType = sns;
    stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.stFrameRate.s32DstFrameRate =
		SDR_VISS_FRAMERATE;

    stVissConfig.astVissInfo[0].stChnInfo.stChnAttr.u32Depth = 2;
    stVissConfig.s32WorkingVissNum = 1;

    s32Ret = SAMPLE_COMM_VISS_StartViss(&stVissConfig);
    EI_TRACE_VENC(EI_DBG_ERR," comm viss start test\n");
    if (s32Ret) {
        EI_TRACE_VENC(EI_DBG_ERR," comm viss start error\n");
    }
    /* display begin */
    EI_MI_DOSS_Init();

	drawframe_info.VoDev = 0;
	drawframe_info.VoLayer = 0;
	drawframe_info.VoChn = 0;
	drawframe_info.u32scnWidth = SCN_WIDTH;
	drawframe_info.u32scnHeight = SCN_HEIGHT;
	s32Ret = EI_MI_VO_Enable(drawframe_info.VoDev);
    if (s32Ret != EI_SUCCESS) {
        EI_PRINT("EI_MI_VO_Enable() failed with %#x!\n", s32Ret);
        goto exit1;
    }

    stVoLayerAttr.u32Align = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Align;
    stVoLayerAttr.stImageSize.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    stVoLayerAttr.stImageSize.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
    stVoLayerAttr.enPixFormat = stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat;
    stVoLayerAttr.stDispRect.s32X = 0;
    stVoLayerAttr.stDispRect.s32Y = 0;
    stVoLayerAttr.stDispRect.u32Width = width;
    stVoLayerAttr.stDispRect.u32Height = hight;
    s32Ret = EI_MI_VO_SetVideoLayerAttr(drawframe_info.VoLayer, &stVoLayerAttr);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit1;
    }

    s32Ret = EI_MI_VO_SetDisplayBufLen(drawframe_info.VoLayer, 3);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit1;
    }

    s32Ret = EI_MI_VO_SetVideoLayerPartitionMode(drawframe_info.VoLayer, VO_PART_MODE_MULTI);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit1;
    }

    s32Ret = EI_MI_VO_SetVideoLayerPriority(drawframe_info.VoLayer, 0);
    if (s32Ret != EI_SUCCESS) {
		EI_TRACE_LOG(EI_DBG_ERR, "\n");
		goto exit1;
    }

    s32Ret = EI_MI_VO_EnableVideoLayer(drawframe_info.VoLayer);
    if (s32Ret != EI_SUCCESS) {
        EI_TRACE_IPPU(EI_DBG_ERR, "\n");
        goto exit1;
    }

    stVoChnAttr.stRect.s32X = 0;
    stVoChnAttr.stRect.s32Y = 0;
    stVoChnAttr.stRect.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    stVoChnAttr.stRect.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;

    EI_MI_VO_SetChnAttr(drawframe_info.VoLayer, drawframe_info.VoChn, &stVoChnAttr);
    EI_MI_VO_EnableChn(drawframe_info.VoLayer, drawframe_info.VoChn);
    EI_MI_VO_SetChnRotation(drawframe_info.VoLayer, drawframe_info.VoChn, rot);
    axframe_info.p_det_info = malloc(sizeof(nn_facerecg_faces_t));
    if (!axframe_info.p_det_info) {
		printf("axframe_info.p_det_info malloc failed!\n");
		goto exit1;
    }
  
    memset(axframe_info.p_det_info, 0x00, sizeof(nn_facerecg_faces_t));
    axframe_info.dev = dev;
    axframe_info.chn = VISS_MAX_PHY_CHN_NUM;/* after phy chn is ext chn index, so mean ext chn 0*/
    axframe_info.phyChn = chn;
    axframe_info.frameRate = SDR_VISS_FRAMERATE;
    axframe_info.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    axframe_info.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
	axframe_info.ax_fout = NULL;
    sem_init(&axframe_info.frame_del_sem, 0, 0);

    start_get_axframe(&axframe_info);
    drawframe_info.dev = dev;
    drawframe_info.chn = VISS_MAX_PHY_CHN_NUM+1;
    drawframe_info.phyChn = chn;
    drawframe_info.frameRate = SDR_VISS_FRAMERATE;
    drawframe_info.u32Width = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Width;
    drawframe_info.u32Height = stVbConfig.astFrameInfo[1].stCommFrameInfo.u32Height;
	drawframe_info.draw_fout = NULL;
	drawframe_info.last_draw_num = RGN_RECT_NUM;
#ifdef RGN_DRAW
	s32Ret = rgn_start();
	if (s32Ret == EI_FAILURE) {
		 EI_PRINT("rgn_start failed with %#x!\n", s32Ret);
	}
#endif
    start_get_drawframe(&drawframe_info);
	memset(stream_para, 0, sizeof(stream_para));
	
	stream_para[0].VeChn = 1;
    stream_para[0].bThreadStart = EI_TRUE;
#ifndef SAVE_REGISTER_FACE_CAM_FRAME
    stSnapJVC.enInputFormat   = stVbConfig.astFrameInfo[1].stCommFrameInfo.enPixelFormat;
    stSnapJVC.u32width        = 1280;
    stSnapJVC.u32height       = 720;
    stSnapJVC.u32bitrate      = 15*1024*1024;

    s32Ret = SAMPLE_COMM_VENC_Start(stream_para[0].VeChn, PT_JPEG,
                            SAMPLE_RC_FIXQP, &stSnapJVC, COMPRESS_MODE_NONE, VENC_GOPMODE_DUAL_P);
    if (EI_SUCCESS != s32Ret) {
        EI_PRINT("SAMPLE_COMM_VENC_Start error\n");
        return EI_FAILURE;
    }
#endif
	while (stream_para[0].exit_flag != EI_TRUE) {
	    usleep(1000 * 1000);
	}
	stream_para[0].bThreadStart = EI_FALSE;
	if (stream_para[0].VeChn < 0 || stream_para[0].VeChn >= VC_MAX_CHN_NUM) {
        EI_PRINT("%s %d  VeChn:%d\n", __func__, __LINE__, stream_para[0].VeChn);
        return EI_FAILURE;
    }
#ifndef SAVE_REGISTER_FACE_CAM_FRAME
	SAMPLE_COMM_VENC_Stop(stream_para[0].VeChn);
#endif
	printf("mdp_facerecg_nn wait exit!\n");
	printf("drawframe wait exit!\n");
	drawframe_info.bThreadStart = EI_FALSE;
	if (drawframe_info.gs_framePid)
 		pthread_join(drawframe_info.gs_framePid, NULL);
	printf("axframe_info wait exit!\n");
    axframe_info.bThreadStart = EI_FALSE;
    sem_post(&axframe_info.frame_del_sem);
    if (axframe_info.gs_framePid)
           pthread_join(axframe_info.gs_framePid, NULL);
    sem_destroy(&axframe_info.frame_del_sem);

#ifdef RGN_DRAW
	rgn_stop();
#endif

    printf("mdp_facerecg_nn start exit!\n");
exit1:
    SAMPLE_COMM_VISS_StopViss(&stVissConfig);
    EI_MI_VO_DisableChn(drawframe_info.VoLayer, drawframe_info.VoChn);
    EI_MI_VO_DisableVideoLayer(drawframe_info.VoLayer);
    SAMPLE_COMM_DOSS_StopDev(drawframe_info.VoDev);
    EI_MI_VO_CloseFd();
    EI_MI_DOSS_Exit();
exit0:
    SAMPLE_COMM_SYS_Exit(&stVbConfig);
    if (axframe_info.p_det_info) {
        free(axframe_info.p_det_info);
        axframe_info.p_det_info = NULL;
    }

    printf("show_1ch end exit!\n");

    return s32Ret;

}

int main(int argc, char **argv)
{
    EI_S32 s32Ret = EI_FAILURE;
	FILE *fp = NULL;
    VISS_DEV dev = 0;
    VISS_CHN chn = 0;
    SNS_TYPE_E sns = TP9930_DVP_1920_1080_25FPS_4CH_YUV;
    int rot = 90;
    EI_BOOL flip = EI_FALSE;
    EI_BOOL mirror = EI_FALSE;
    unsigned int rect_x = 0;
    unsigned int rect_y = 0;
    unsigned int width = 720;
    unsigned int hight = 1280;
    int c;

	EI_PRINT("[%d] mdp_facerecg_nn start v1.0rc1_202204181000\n", __LINE__);
	memset(&axframe_info, 0x00, sizeof(axframe_info));
	memset(&drawframe_info, 0x00, sizeof(drawframe_info));
    while ((c = getopt(argc, argv, "d:c:r:s:a:i:n:f:h:")) != -1) {
        switch (c) {
		case 'd':
			dev = (unsigned int)strtol(argv[optind - 1], NULL, 10);
			break;
		case 'c':
   			chn = (unsigned int)strtol(argv[optind - 1], NULL, 10);
			break;
		case 'r':
			rot = (unsigned int)strtol(argv[optind - 1], NULL, 10);
			break;
		case 's':
			sns = (unsigned int)strtol(argv[optind - 1], NULL, 10);
 			break;
		case 'a':
			axframe_info.action = (unsigned int)strtol(argv[optind - 1], NULL, 10);
			if (axframe_info.action >= REG_ACTION_END) {
				preview_help();
				goto EXIT;
			}
			break;
		case 'i':
			axframe_info.input_id = (unsigned int)strtol(argv[optind - 1], NULL, 10);
			break;
		case 'n':
			sprintf(axframe_info.input_name, "n-%s", argv[optind - 1]);
			break;
		case 'f':		
			sprintf(mdp_vdec_info.vdec_full_name, "%s", argv[optind - 1]);
 			break;
        default:
            preview_help();
            goto EXIT;
        }
    }
	EI_PRINT("dev: %d c: %d r: %d s: %d a: %d i : %d n: %s f: %s\n", dev, chn, rot, sns,
		axframe_info.action, axframe_info.input_id, axframe_info.input_name, mdp_vdec_info.vdec_full_name);
    signal(SIGINT, stop_get_frame);
    signal(SIGTERM, stop_get_frame);
    if (rot == 90) {
        rot = ROTATION_90;
    } else if (rot == 180) {
        rot = ROTATION_180;
    } else if (rot == 270) {
        rot = ROTATION_270;
    }
	warning_info.mq_id = osal_mq_create("/warning_mq", MQ_MSG_NUM, MQ_MSG_LEN);
	if (warning_info.mq_id == 0) {
		EI_PRINT("[%s, %d]Create waring queue failed!\n", __func__, __LINE__);
		goto EXIT;
	}

	warning_info.bThreadStart = EI_TRUE;
    s32Ret = pthread_create(&(warning_info.g_Pid), NULL, warning_proc,
        (EI_VOID *)&warning_info);
    if (s32Ret)
        EI_PRINT("errno=%d, reason(%s)\n", errno, strerror(errno));
	if (axframe_info.action == REGISTER_FACE_IMG) {
		if (!strlen(mdp_vdec_info.vdec_full_name)) {
			osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_REG_FAIL, NULL, 0);
			EI_PRINT("reg img is NULL\n");
			usleep(5000*1000);
			goto EXIT1;
		}
		fp = fopen(mdp_vdec_info.vdec_full_name, "rb");
		if (fp == NULL) {
			osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_REG_FAIL, NULL, 0);
			EI_PRINT("reg img is not exist\n");
			usleep(5000*1000);
			goto EXIT1;
		} else{
			fclose(fp);
			fp = NULL;
			if (!strlen(axframe_info.input_name)) {
				sprintf(axframe_info.input_name, "n-%d", axframe_info.input_id);
			}
		}
	} else if (axframe_info.action == RECOGNITION_FACE) {
		fp = fopen("/data/drivers_db.db", "rb");
		if (fp == NULL) {
			EI_PRINT("no match database\n");
			osal_mq_send(warning_info.mq_id, LB_WARNING_FACERECG_MATCH_FAIL, NULL, 0);
			usleep(5000*1000);
			goto EXIT1;
		} else{
			fclose(fp);
			fp = NULL;
		}
	} else if (axframe_info.action == REGISTER_FACE_CAM) {
		if (!strlen(axframe_info.input_name)) {
			sprintf(axframe_info.input_name, "n-%d", axframe_info.input_id);
		}
	}
    s32Ret = show_1ch(dev, chn, rot, mirror, flip, rect_x, rect_y, width, hight, sns);
EXIT1:
	warning_info.bThreadStart = EI_FALSE;
	if (warning_info.g_Pid) {
           pthread_join(warning_info.g_Pid, NULL);
	}
	s32Ret = osal_mq_destroy(warning_info.mq_id, "/warning_mq");
	if (s32Ret) {
		EI_PRINT("[%s, %d]waring_mq destory failed!\n", __func__, __LINE__);
	}
	exit(EXIT_SUCCESS);
EXIT:
    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
