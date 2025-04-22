#ifndef _FACERECG_NN_H_
#define _FACERECG_NN_H_

#include <pthread.h>
#include "utils.h"
#include "nn_facerecg_api.h"

#define compare_distance(a0, a1, a2, a3) (((a3-a2)/(a1-a0))> 0.9 && ((a3-a2)/(a1-a0))< 1.1)?1:0


#define FACE_DATABASE			"/data/etc/drivers.db"

#define FACEDB_CREATE			"CREATE TABLE IF NOT EXISTS drives(_id INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT NOT NULL,norm FLOAT NOT NULL,feature_len INT,feature BLOB);"
#define FACEDB_INSERT			"INSERT OR REPLACE INTO drives values(NULL,'%s',%f,%d,?);"
#define FACEDB_COUNT 			"SELECT COUNT(*) FROM drives;"
#define FACEDB_DELETE 			"DELETE FROM drives WHERE _id=(SELECT MIN(_id) FROM drives);"
#define FACEDB_DELALL 			"DELETE FROM drives"
#define FACEDB_DELBYNAME		"DELETE FROM drives WHERE name = '%s';"

#define NN_FACERECG_AX_PATH	 	"/usr/share/ax/dms"

#define COUNT_REGISTER_FACE_FRAME 20
#define FACE_REGISTER_MIN_PIX  64
#define FACE_REGISTER_MAX_PIX  512

#define REGISTER_FACE_MAX		20

#define FACE_RECG_FRAMES		20

typedef enum 
{

	ACTION_IDEL,
	RECOGNITION_FACE,
	REGISTER_FACE_CAM,
	REGISTER_FACE_IMG,
	REGISTER_END,
	
} RegFaceAction_e;
	
typedef struct
{

	char name[128];
	
}driver_info_t;

typedef struct
{

	driver_info_t info[REGISTER_FACE_MAX];
	nn_facerecg_features_t m_face_features;
	int size;
	
} driver_all_info_t;

typedef struct
{

	uint8_t index;
	float score;

} driver_recg_result_t;

typedef struct
{

	unsigned char initFlag;

	RegFaceAction_e action;
	nn_facerecg_cfg_t facerecg_cfg;

	unsigned short u32Width;
	unsigned short u32Height;
	nn_facerecg_in_t recg_in;
	nn_facerecg_faces_t faces;

	int nn_facerecg_det_count;
	int nn_facerecg_det_offhead_count;
	int nn_facerecg_det_too_big_count;
	int nn_facerecg_det_too_small_count;
	
	int nn_facerecg_offhead_times;

	nn_facerecg_features_t* match_features;
	
	driver_all_info_t m_drivers;

	char regDriverName[128];
	int regDriverID;

	void *nn_hdl;

	pthread_t pid;

	uint32_t register_start_time;

	driver_recg_result_t recgResult[FACE_RECG_FRAMES];

	sqlite3 *drivers_db;

} Facerecg_nn_t;

int facerecg_nn_start(VIDEO_FRAME_INFO_S *frame);

void facerecg_driver_recgonize(char* driver_name);

void facerecg_driver_register(char* driver_name);

void facerecg_nn_init(unsigned short frmWidth, unsigned short frmHeight);

int facerecg_register_by_images(char* imagesPath);

int facerecg_nn_get_drvier(int idx, char* name);

int facerecg_nn_del_driver(int idx, char* name);


#endif

