/************************************************************************************************
* 程序版本：V3.0
* 程序日期：2022-11-3
* 程序作者：719飞行器实验室
************************************************************************************************/
#ifndef FILTER_H
#define FILTER_H

#include "filter.h"
#include "math.h"
#include "kalman.h"
#include "imu.h"
#include "structconfig.h"
#include "myMath.h"


float Low_Filter(float value);
void Aver_FilterXYZ(FLOAT_XYZ *Acc_filt,uint8_t n);
void Aver_Filter(float data,float *filt_data,uint8_t n);
void presssureFilter(float* in, float* out);
float Lowpass_Filter_ROLL(float InputData);
float Lowpass_Filter_THROTTLE(float InputData);
float Lowpass_Filter_YAW(float InputData);
float Lowpass_Filter_PITCH(float InputData);


void LPF2pSetCutoffFreq_1(float sample_freq, float cutoff_freq);
float LPF2pApply_1(float sample);
#endif
