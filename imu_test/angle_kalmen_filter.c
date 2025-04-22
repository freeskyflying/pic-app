#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "utils.h"

#define AX_IDX			0
#define AY_IDX			1
#define AZ_IDX			2

#define GX_IDX			0
#define GY_IDX			1
#define GZ_IDX			2

/*计算偏移量*/
float i;                                    //计算偏移量时的循环次数
float ax_offset = 0, ay_offset = 0;         //x,y轴的加速度偏移量
float gx_offset = 0, gy_offset = 0;         //x,y轴的角速度偏移量

/*参数*/
float rad2deg = 57.29578;                   //弧度到角度的换算系数
float roll_v = 0, pitch_v = 0;              //换算到x,y轴上的角速度

/*定义微分时间*/
float now = 0, lasttime = 0, dt = 0;        //定义微分时间

/*三个状态，先验状态，观测状态，最优估计状态*/
float gyro_roll = 0, gyro_pitch = 0;        //陀螺仪积分计算出的角度，先验状态
float acc_roll = 0, acc_pitch = 0;          //加速度计观测出的角度，观测状态
float k_roll = 0, k_pitch = 0;              //卡尔曼滤波后估计出最优角度，最优估计状态

/*误差协方差矩阵P*/
float e_P[2][2] ={{1,0},{0,1}};             //误差协方差矩阵，这里的e_P既是先验估计的P，也是最后更新的P

/* 误差协方差矩阵Q */
float e_Q[2][2] ={{0.0025,0},{0,0.0025}};

/*卡尔曼增益K*/
float k_k[2][2] ={{0,0},{0,0}};             //这里的卡尔曼增益矩阵K是一个2X2的方阵
/*卡尔曼增益K offset*/
float k_koff[2][2] ={{0.3,0},{0,0.3}};             //这里的卡尔曼增益矩阵K是一个2X2的方阵

void andle_kalmen_init(void) {

	inv_imu_start();
	usleep(500*1000);
	
	float accs[3], gyros[3], temprature;
	//计算偏移量
	for (i = 1; i <= 2000; i++) {
		imu_get_accgyro(accs, gyros, &temprature);
		ax_offset = ax_offset + accs[AX_IDX];//计算x轴加速度的偏移总量
		ay_offset = ay_offset + accs[AY_IDX];//计算y轴加速度的偏移总量
		gx_offset = gx_offset + gyros[GX_IDX];
		gy_offset = gy_offset + gyros[GY_IDX];
		usleep(10);
	}
	ax_offset = ax_offset / 2000; //计算x轴加速度的偏移量
	ay_offset = ay_offset / 2000; //计算y轴加速度的偏移量
	gx_offset = gx_offset / 2000; //计算x轴角速度的偏移量
	gy_offset = gy_offset / 2000; //计算y轴角速度的偏移量
}

float sq(float n)
{
	return n*n;
}

void loop() {
	/*计算微分时间*/
	now = getUptime();                           //当前时间(ms)
	dt = (now - lasttime);           //微分时间(s)
	lasttime = now;                           //上一次采样时间(ms)

	float accs[3], gyros[3], temprature;
	/*获取角速度和加速度 */
	imu_get_accgyro(accs, gyros, &temprature);

	/*step1:计算先验状态*/
	/*计算x,y轴上的角速度*/
	roll_v = (gyros[GX_IDX]-gx_offset) + ((sin(k_pitch)*sin(k_roll))/cos(k_pitch))*(gyros[GY_IDX]-gy_offset) + ((sin(k_pitch)*cos(k_roll))/cos(k_pitch))*gyros[GZ_IDX];//roll轴的角速度
	pitch_v = cos(k_roll)*(gyros[GY_IDX]-gy_offset) - sin(k_roll)*gyros[GZ_IDX];//pitch轴的角速度
	gyro_roll = k_roll + dt*roll_v;//先验roll角度
	gyro_pitch = k_pitch + dt*pitch_v;//先验pitch角度

	/*step2:计算先验误差协方差矩阵P*/
	e_P[0][0] = e_P[0][0] + e_Q[0][0];//这里的Q矩阵是一个对角阵且对角元均为0.0025
	e_P[0][1] = e_P[0][1] + 0;
	e_P[1][0] = e_P[1][0] + 0;
	e_P[1][1] = e_P[1][1] + e_Q[1][1];

	/*step3:更新卡尔曼增益K*/
	k_k[0][0] = e_P[0][0]/(e_P[0][0]+k_koff[0][0]);
	k_k[0][1] = 0;
	k_k[1][0] = 0;
	k_k[1][1] = e_P[1][1]/(e_P[1][1]+k_koff[1][1]);

	/*step4:计算最优估计状态*/
	/*观测状态*/
	//roll角度
	acc_roll = atan((accs[AY_IDX] - ay_offset) / (accs[AZ_IDX]))*rad2deg;
	//pitch角度
	acc_pitch = -1*atan((accs[AX_IDX] - ax_offset) / sqrt(sq(accs[AY_IDX] - ay_offset) + sq(accs[AZ_IDX])))*rad2deg;
	/*最优估计状态*/
	k_roll = gyro_roll + k_k[0][0]*(acc_roll - gyro_roll);
	k_pitch = gyro_pitch + k_k[1][1]*(acc_pitch - gyro_pitch);

	/*step5:更新协方差矩阵P*/
	e_P[0][0] = (1 - k_k[0][0])*e_P[0][0];
	e_P[0][1] = 0;
	e_P[1][0] = 0;
	e_P[1][1] = (1 - k_k[1][1])*e_P[1][1];

	//printf("%.3f, %.3f, %.3f,   %.3f, %.3f, %.3f   ", accs[0], accs[1], accs[2], gyros[0], gyros[1], gyros[2]);
	printf("roll: %f", k_roll);
	printf("   pitch: %f\n", k_pitch);

	//打印角度
	//  Serial.print("roll: ");
	//  Serial.print(k_roll);
	//  Serial.print(",");
	//  Serial.print("pitch: ");
	//  Serial.println(k_pitch);

	//姿态可视化
	//  Serial.print(k_roll);
	//  Serial.print("/");
	//  Serial.println(k_pitch);
}
