/************************************************************************************************
* 程序版本：V3.0
* 程序日期：2022-11-3
* 程序作者：719飞行器实验室
************************************************************************************************/
#include "imu.h"
#include "myMath.h"

#define Kp_New      0.9f           				    //互补滤波当前数据的权重
#define Kp_Old      0.1f           				    //互补滤波历史数据的权重  
#define Acc_Gain  	0.0001220f		    			  //加速度变成G (初始化加速度满量程-+4g LSBa = 2*4/65535.0)
#define Gyro_Gain 	0.0609756f							  //角速度变成度 (初始化陀螺仪满量程+-2000 LSBg = 2*2000/65535.0)
#define Gyro_Gr	    0.0010641f							  //角速度变成弧度(3.1415/180 * LSBg)       

FLOAT_ANGLE Att_Angle;                        //飞机姿态数据
FLOAT_XYZ 	Acc_filt,Gyr_filt,Acc_filtold,Gyr_radold;	  //滤波后的各通道数据
float   accb[3];                              //加速度计数据存储
float   gyrb[3];                               //角速度计数据存储
float   DCMgb[3][3];                          //方向余弦阵（将 惯性坐标系 转化为 机体坐标系）
uint8_t AccbUpdate = 0;                       //加速度计数据更新标志位
uint8_t GyrbUpdate = 0;                       //角速度计数据更新标志位

long long int timeStart = 0;

float   gAccDeltForward[100];

/*********************************************************************************************************
* 函  数：void Prepare_Data(void)
* 功　能：对陀螺仪去零偏后的数据滤波及赋予物理意义，为姿态解算做准备
* 参  数：无
* 返回值：无
**********************************************************************************************************/	
void Prepare_Data(void)
{
	float accs[3], gyros[3], temprature;
	/*获取角速度和加速度 */
	imu_get_accgyro(accs, gyros, &temprature);
//	Aver_FilterXYZ(&Acc_filt,12);//此处对加速度计进行滑动窗口滤波处理 
//  Aver_FilterXYZ(&Gyr_filt,12);//此处对角速度计进行滑动窗口滤波处理
	
	//加速度AD值 转换成 米/平方秒 
	Acc_filt.X = accs[0] /* Acc_Gain*/ * G;;
	Acc_filt.Y = accs[1] /* Acc_Gain*/ * G;;
	Acc_filt.Z = accs[2] /* Acc_Gain*/ * G;;
	printf("ax=%0.2f ay=%0.2f az=%0.2f\r\n", accs[0], accs[1], accs[2]);

	//陀螺仪AD值 转换成 弧度/秒    
	Gyr_filt.X = gyros[0] / 57.3f;  
	Gyr_filt.Y = gyros[1] / 57.3f;
	Gyr_filt.Z = gyros[2] / 57.3f;
//	printf("gx=%0.2f gy=%0.2f gz=%0.2f\r\n",Gyr_filt.X,Gyr_filt.Y,Gyr_filt.Z);
}
/*********************************************************************************************************
* 函  数：void IMUupdate(FLOAT_XYZ *Gyr_filt,FLOAT_XYZ *Acc_filt,FLOAT_ANGLE *Att_Angle)
* 功　能：获取姿态角
* 参  数：Gyr_filt 	指向角速度的指针（注意单位必须是弧度）
*         Acc_filt 	指向加速度的指针
*         Att_Angle 指向姿态角的指针
* 返回值：无
* 备  注：求解四元数和欧拉角都在此函数中完成
**********************************************************************************************************/	
//kp=ki=0 就是完全相信陀螺仪
#define Kp 1.5f                          // proportional gain governs rate of convergence to accelerometer/magnetometer
                                          //比例增益控制加速度计，磁力计的收敛速率
#define Ki 0.005f                         // integral gain governs rate of convergence of gyroscope biases  
                                          //积分增益控制陀螺偏差的收敛速度
#define halfT 0.005f                      // half the sample period 采样周期的一半

float q0 = 1, q1 = 0, q2 = 0, q3 = 0;     // quaternion elements representing the estimated orientation
float exInt = 0, eyInt = 0, ezInt = 0;    // scaled integral error

void IMUupdate(FLOAT_XYZ *Gyr_filt,FLOAT_XYZ *Acc_filt,FLOAT_ANGLE *Att_Angle)
{
	float ax = Acc_filt->X,ay = Acc_filt->Y,az = Acc_filt->Z;
	float gx = Gyr_filt->X,gy = Gyr_filt->Y,gz = Gyr_filt->Z;
	float vx, vy, vz;
	float ex, ey, ez;
	float norm;

	float q0q0 = q0*q0;
	float q0q1 = q0*q1;
	float q0q2 = q0*q2;
	float q0q3 = q0*q3;
	float q1q1 = q1*q1;
	float q1q2 = q1*q2;
	float q1q3 = q1*q3;
	float q2q2 = q2*q2;
	float q2q3 = q2*q3;
	float q3q3 = q3*q3;

	if(ax*ay*az==0)
	return;

	//加速度计<测量>的重力加速度向量(机体坐标系) 
	norm = invSqrt(ax*ax + ay*ay + az*az); 
	ax = ax * norm;
	ay = ay * norm;
	az = az * norm;
//	printf("ax=%0.2f ay=%0.2f az=%0.2f\r\n",ax,ay,az);

	//陀螺仪积分<估计>重力向量(机体坐标系) 
	vx = 2*(q1q3 - q0q2);	               //矩阵(3,1)项											
	vy = 2*(q0q1 + q2q3);                //矩阵(3,2)项
	vz = q0q0 - q1q1 - q2q2 + q3q3 ;     //矩阵(3,3)项
	
	// printf("vx=%0.2f vy=%0.2f vz=%0.2f\r\n",vx,vy,vz); 

	//向量叉乘所得的值 
	ex = (ay*vz - az*vy);                     
	ey = (az*vx - ax*vz); 
	ez = (ax*vy - ay*vx);

	//用上面求出误差进行积分
	exInt = exInt + ex * Ki;								 
	eyInt = eyInt + ey * Ki;
	ezInt = ezInt + ez * Ki;

	//将误差PI后补偿到陀螺仪
	gx = gx + Kp*ex + exInt;					   		  	
	gy = gy + Kp*ey + eyInt;
	gz = gz + Kp*ez + ezInt;//这里的gz由于没有观测者进行矫正会产生漂移，表现出来的就是积分自增或自减

	//四元素的微分方程
	q0 = q0 + (-q1*gx - q2*gy - q3*gz)*halfT;
	q1 = q1 + (q0*gx + q2*gz - q3*gy)*halfT;
	q2 = q2 + (q0*gy - q1*gz + q3*gx)*halfT;
	q3 = q3 + (q0*gz + q1*gy - q2*gx)*halfT;

	//单位化四元数 
	norm = invSqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
	q0 = q0 * norm;
	q1 = q1 * norm;
	q2 = q2 * norm;  
	q3 = q3 * norm;

//  矩阵表达式
//	matrix[0] = q0q0 + q1q1 - q2q2 - q3q3;	 // 11
//	matrix[1] = 2.f * (q1q2 + q0q3);	       // 12
//	matrix[2] = 2.f * (q1q3 - q0q2);	       // 13
//	matrix[3] = 2.f * (q1q2 - q0q3);	       // 21
//	matrix[4] = q0q0 - q1q1 + q2q2 - q3q3;	 // 22
//	matrix[5] = 2.f * (q2q3 + q0q1);	       // 23
//	matrix[6] = 2.f * (q1q3 + q0q2);	       // 31
//	matrix[7] = 2.f * (q2q3 - q0q1);	       // 32
//	matrix[8] = q0q0 - q1q1 - q2q2 + q3q3;	 // 33

	//四元数转换成欧拉角(Z->Y->X) 
	
	//偏航角YAW
	if((Gyr_filt->Z *RadtoDeg > 1.0f) || (Gyr_filt->Z *RadtoDeg < -1.0f)) //数据太小可以认为是干扰，不是偏航动作
	{
		Att_Angle->yaw += Gyr_filt->Z *RadtoDeg*0.01f;
	}   
	
	//横滚角ROLL
	Att_Angle->rol = -asin(2.f * (q1q3 - q0q2))* 57.3f;

	//俯仰角PITCH
	Att_Angle->pit = -atan2(2.f * q2q3 + 2.f * q0q1, q0q0 - q1q1 - q2q2 + q3q3)* 57.3f ;
	
//    printf("%f,%f,%f\n",Att_Angle->yaw,Att_Angle->pit,Att_Angle->rol);
}

void imu_loop(void)
{
	Prepare_Data();
	IMUupdate(&Gyr_filt, &Acc_filt, &Att_Angle);
}
