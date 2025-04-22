/************************************************************************************************
* 程序版本：V3.0
* 程序日期：2022-11-3
* 程序作者：719飞行器实验室
************************************************************************************************/
#include "filter.h"

#define N 20      														//滤波缓存数组大小
#define M_PI_F 3.1416f

/************************************************************************************************
* 函  数：void Aver_FilterXYZ(INT16_XYZ *acc,FLOAT_XYZ *Acc_filt,uint8_t n)
* 功  能：滑动窗口滤波(三组数据)
* 参  数：*acc  要滤波数据地址
*         *Acc_filt 滤波后数据地址
* 返回值：无 
************************************************************************************************/
void Aver_FilterXYZ(FLOAT_XYZ *Acc_filt,uint8_t n)
{
	static int32_t bufax[N],bufay[N],bufaz[N];
	static uint8_t cnt =0,flag = 1;
	int32_t temp1=0,temp2=0,temp3=0,i;
	bufax[cnt] = Acc_filt->X;
	bufay[cnt] = Acc_filt->Y;
	bufaz[cnt] = Acc_filt->Z;
	cnt++;     															 //这个的位置必须在赋值语句后，否则bufax[0]不会被赋值
	if(cnt<n && flag) 
		return; 														   //数组填不满不计算
	else
		flag = 0;
	for(i=0;i<n;i++)
	{
		temp1 += bufax[i];
		temp2 += bufay[i];
		temp3 += bufaz[i];
	}
	 if(cnt>=n)  cnt = 0;
	 Acc_filt->X  = temp1/n;
	 Acc_filt->Y  = temp2/n;
	 Acc_filt->Z  = temp3/n;
}
/************************************************************************************************
* 函  数：void Aver_Filter(float data,float *filt_data,uint8_t n
* 功  能：滑动窗口滤波（一组数据）
* 参  数：data  要滤波数据
*         *filt_data 滤波后数据地址
* 返回值：返回滤波后的数据
************************************************************************************************/
void Aver_Filter(float data,float *filt_data,uint8_t n)
{
  static float buf[N];
	static uint8_t cnt =0,flag = 1;
	float temp=0;
	uint8_t i;
	buf[cnt] = data;
	cnt++;
	if(cnt<n && flag) 
		return; 														  //数组填不满不计算
	else
		flag = 0;
	
	for(i=0;i<n;i++)
	{
		temp += buf[i];
	}
	if(cnt>=n) cnt = 0;
	 *filt_data = temp/n;
}
/************************************************************************************************
* 函  数：void Aver_FilterXYZ(INT16_XYZ *acc,FLOAT_XYZ *Acc_filt,uint8_t n)
* 功  能：抗干扰型滑动窗口滤波(三组数据)
* 参  数：*acc  要滤波数据地址
*          *Acc_filt 滤波后数据地址
* 返回值：无 
************************************************************************************************/

/************************************************************************************************
* 函  数：float Lowpass_Filter(float InputData)
* 功  能：一阶低通滤波
* 参  数：
* 返回值：无 
************************************************************************************************/
float Lowpass_Filter_ROLL(float InputData)
{
		static float old_data = 1500;
		float new_data = InputData;
		float OutputData;
		OutputData = 0.4*new_data + 0.6*old_data;
		old_data = OutputData;
		return OutputData;
}

float Lowpass_Filter_PITCH(float InputData)
{
		static float old_data_2 = 1500;
		float new_data = InputData;
		float OutputData;
		OutputData = 0.4*new_data + 0.6*old_data_2;
		old_data_2 = OutputData;
		return OutputData;
}
float Lowpass_Filter_YAW(float InputData)
{
		static float old_data_3 = 1500;
		float new_data = InputData;
		float OutputData;
		OutputData = 0.4*new_data + 0.6*old_data_3;
		old_data_3 = OutputData;
		return OutputData;
}
float Lowpass_Filter_THROTTLE(float InputData)
{
		static float old_data_4 = 1500;
		float new_data = InputData;
		float OutputData;
		OutputData = 0.4*new_data + 0.6*old_data_4;
		old_data_4 = OutputData;
		return OutputData;
}
