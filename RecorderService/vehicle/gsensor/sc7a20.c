#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <math.h>

#include "utils.h"
#include "sc7a20.h"
#include "pnt_log.h"

static SC7A20_t gSC7A20 = { 0 };

#define DBG_PRINT	PNT_LOGE

static int I2C_wirte(int file, unsigned char addr, unsigned char *value, int len) 
{
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];
 
    messages[0].addr  = addr;
    messages[0].flags = 0;
    messages[0].len   = len;
    messages[0].buf   = value;
 
    /* Transfer the i2c packets to the kernel and verify it worked */
    packets.msgs  = messages;
    packets.nmsgs = 1;
    if(ioctl(file, I2C_RDWR, &packets) < 0) {
    	perror("Unable to send data");
    	return 1;
    }
 
    return 0;
}

static int I2C_read(int file, unsigned char addr, unsigned char *value, int len_write, int len_read) 
{
	unsigned char *inbuf, *buf_write;
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[2];
 
	/* messages[0] : write step */
	messages[0].addr  = addr;
	messages[0].flags = 0;
	messages[0].len   = len_write;
	messages[0].buf   = value;
 
	/* messages[1] : read step */
	messages[1].addr  = addr;
	messages[1].flags = I2C_M_RD/* | I2C_M_NOSTART*/;
	messages[1].len   = len_read;
	messages[1].buf   = value;   //读到数据存放value中
 
	/* Send the request to the kernel and get the result back */
	packets.msgs      = messages;
	packets.nmsgs     = 2;
	if(ioctl(file, I2C_RDWR, &packets) < 0) {
		perror("Unable to send data");
		return 1;
	}
 
	return 0;
}

static int I2C_reads(int file, unsigned char addr, unsigned char reg, unsigned char *value, int len_read) 
{
	unsigned char *inbuf, *buf_write;
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[2];
 
	/* messages[0] : write step */
	messages[0].addr  = addr;
	messages[0].flags = 0;
	messages[0].len   = 1;
	messages[0].buf   = &reg;
 
	/* messages[1] : read step */
	messages[1].addr  = addr;
	messages[1].flags = I2C_M_RD/* | I2C_M_NOSTART*/;
	messages[1].len   = len_read;
	messages[1].buf   = value;   //读到数据存放value中
 
	/* Send the request to the kernel and get the result back */
	packets.msgs      = messages;
	packets.nmsgs     = 2;
	if(ioctl(file, I2C_RDWR, &packets) < 0) {
		perror("Unable to send data");
		return 1;
	}
 
	return 0;
}

void g_sensor_write(unsigned char reg, unsigned char value)
{
	unsigned char values[2] = {reg, value};

	if (gSC7A20.i2cFd<0)
	{
		return;
	}

	I2C_wirte(gSC7A20.i2cFd, SC7A20_IIC_ADDR, values, sizeof(values));
}

unsigned char g_sensor_read(unsigned char reg)
{
	unsigned char value = reg;

	if (gSC7A20.i2cFd<0)
	{
		return -1;
	}

	I2C_read(gSC7A20.i2cFd, SC7A20_IIC_ADDR, &value, 1, 1);

	return value;
}

int g_sensor_read_bytes(unsigned char reg, unsigned char* buf, int len)
{
	if (gSC7A20.i2cFd<0)
	{
		return -1;
	}
	I2C_reads(gSC7A20.i2cFd, SC7A20_IIC_ADDR, reg, buf, len);

	return 0;
}

unsigned char ap_gsensor_power_on_get_status(void)
{
  return gSC7A20.status;
}

void ap_gsensor_power_on_set_status(unsigned char status)
{
   if(status)
     gSC7A20.status=1;  //gsensor power on
   else
     gSC7A20.status=0;  //key power on
}

/***************BOOT 重载内部寄存器值*********************/
void SL_SC7A20_BOOT(void)
{
    unsigned char SL_Read_Reg=0xff;
    SL_Read_Reg = g_sensor_read(SL_SC7A20_CTRL_REG5);
    SL_Read_Reg = SL_SC7A20_BOOT_ENABLE | SL_Read_Reg;
    g_sensor_write(SL_SC7A20_CTRL_REG5, SL_Read_Reg);
}

/***************传感器量程设置**********************/
void SL_SC7A20_FS_Config(unsigned char Sc7a20_FS_Reg)
{
	unsigned char SL_Write_Reg;
//    unsigned char SL_Read_Reg=0xff;
//    SL_Read_Reg = g_sensor_read(SL_SC7A20_CTRL_REG4);
    SL_Write_Reg = 0x80 | Sc7a20_FS_Reg | SL_SC7A20_HR_ENABLE;
    g_sensor_write(SL_SC7A20_CTRL_REG4, SL_Write_Reg);
}

/***************数据更新速率**加速度计使能**********/
void SL_SC7A20_Power_Config(unsigned char Power_Config_Reg)
{
#if  SL_SC7A20_MTP_ENABLE == 0X01
    unsigned char SL_Read_Reg = 0x00;
    g_sensor_write(SL_SC7A20_MTP_CFG, SL_SC7A20_MTP_VALUE);
    SL_Read_Reg = g_sensor_read(SL_SC7A20_SDOI2C_PU_CFG); 
    SL_Read_Reg = SL_Read_Reg | SL_SC7A20_SDO_PU_MSK | SL_SC7A20_I2C_PU_MSK;
    g_sensor_write(SL_SC7A20_SDOI2C_PU_CFG, SL_Read_Reg);
#endif
    g_sensor_write(SL_SC7A20_CTRL_REG1, Power_Config_Reg);
}

//AOI2_INT1  位置检测，检测是否离开Y+位置  1D位置识别
/***************中断设置*************/
void SL_SC7A20_INT_Config(void)
{
	unsigned char SL_Read_Reg;
	//---------- 中断1配置 ------------
	SL_Read_Reg=0x00;            //0xC8
	SL_Read_Reg=SL_Read_Reg|0x20;//Z轴高
	g_sensor_write(SL_SC7A20_AOI1_CFG, SL_Read_Reg);
	//中断阈值设置
	g_sensor_write(SL_SC7A20_AOI1_THS, SL_SC7A20_INT_THS_20PERCENT);	
	//大于阈值多少时间触发中断
	g_sensor_write(SL_SC7A20_AOI1_DURATION, SL_SC7A20_INT_DURATION_5CLK);
	//---------- 中断2配置 ------------
	SL_Read_Reg=0x00;            //0xC8
	//SL_Read_Reg=SL_Read_Reg|0x40;//方向位置识别模式
	//修改08即可切换到任意轴朝上的情况
	SL_Read_Reg = SL_Read_Reg|0x20;//Z轴高
	g_sensor_write(SL_SC7A20_AOI2_CFG, SL_Read_Reg);
	//中断阈值设置
	g_sensor_write(SL_SC7A20_AOI2_THS, SL_SC7A20_INT_THS_80PERCENT);	
	//大于阈值多少时间触发中断
	g_sensor_write(SL_SC7A20_AOI2_DURATION, SL_SC7A20_INT_DURATION_10CLK);
	
	//-----------
	SL_Read_Reg = g_sensor_read(SL_SC7A20_CTRL_REG5);
	SL_Read_Reg = SL_Read_Reg&0xFD;//AOI2 NO LATCH
	g_sensor_write(SL_SC7A20_CTRL_REG5, SL_Read_Reg);
	
	//H_LACTIVE SET	    AOI2 TO INT2
	SL_Read_Reg = g_sensor_read(SL_SC7A20_CTRL_REG6);
	SL_Read_Reg = SL_SC7A20_INT_ACTIVE_LOWER_LEVEL|SL_Read_Reg|0x20;
	//interrupt happen,int pin output lower level
	g_sensor_write(SL_SC7A20_CTRL_REG6, SL_Read_Reg);

	//HPF SET
	SL_Read_Reg = g_sensor_read(SL_SC7A20_CTRL_REG2);
	SL_Read_Reg = SL_Read_Reg&0xFD;//NO HPF TO AOI2 
	g_sensor_write(SL_SC7A20_CTRL_REG2, SL_Read_Reg);

	//AOI1 TO INT1
	SL_Read_Reg = g_sensor_read(SL_SC7A20_CTRL_REG3);
	SL_Read_Reg = SL_Read_Reg|0x40; //AOI2 TO INT1
	g_sensor_write(SL_SC7A20_CTRL_REG3, SL_Read_Reg);
}

int G_Sensor_SC7A20_Init(unsigned char level)
{
	unsigned char  temp1;

	gSC7A20.i2cFd = open("/dev/i2c-1", O_RDWR);
	if (gSC7A20.i2cFd < 0)
	{
		DBG_PRINT("G_Sensor_SC7A20_Init open i2c failed. \r\n");
		return RET_FAILED;
	}

	gSC7A20.level = level;
	
	temp1 = g_sensor_read(SC7A20_CHIP_ID_ADDRESS);
	DBG_PRINT("chip_id = %x\r\n", temp1);

	if (0x11 == temp1)
	{
		SL_SC7A20_BOOT();
		
		SL_SC7A20_FS_Config(SL_SC7A20_FS_2G);
		SL_SC7A20_Power_Config(SL_SC7A20_ODR_25HZ);
		/*SL_SC7A20_INT_Config();
		*/
		return RET_SUCCESS;
	}
	else
	{
		close(gSC7A20.i2cFd);
		gSC7A20.i2cFd = -1;
	}

	return RET_FAILED;
}

void G_Sensor_SC7A20_DeInit(void)
{
	if (gSC7A20.i2cFd >= 0)
	{
		close(gSC7A20.i2cFd);
	}
}

int SC7A20_ReadAcceleration(short* pXa, short* pYa, short* pZa)
{
	unsigned char buff[6];
	unsigned char i;
	short temp;
 
	memset(buff, 0, 6);
	
	g_sensor_read_bytes(SL_SC7A20_OUT_X_L|0x80, buff, 6);
	
	//X轴
	*pXa = buff[1];
	*pXa <<= 8;
	*pXa |= buff[0];
	*pXa >>= 4;	//取12bit精度

	//Y轴
	*pYa = buff[3];
	*pYa <<= 8;
	*pYa |= buff[2];
	*pYa >>= 4;	//取12bit精度
	
	//Z轴
	*pZa = buff[5];
	*pZa <<= 8;
	*pZa |= buff[4];
	*pZa >>= 4;	//取12bit精度
 
	return 0;
}
 
#define PI 3.1415926535898

int SC7A20_GetZAxisAngle(short AcceBuff[3], float* pAngleZ)
{
	double fx, fy, fz;
	double A;
	short Xa, Ya, Za;

	if (SC7A20_ReadAcceleration(&Xa, &Ya, &Za)) 
	{
		return -1;		//ADXL362 读取加速度数据
	}
	
	DBG_PRINT("Xa:%d \tYa:%d \tZa:%d \r\n",Xa,Ya,Za);

	AcceBuff[0] = Xa;	//x轴加速度
	AcceBuff[1] = Ya;	//y轴加速度
	AcceBuff[2] = Za;	//z轴加速度
 
	fx = Xa;
	fx *= 2.0 / 4096;
	fy = Ya;
	fy *= 2.0 / 4096;
	fz = Za;
	fz *= 2.0 / 4096;

	//Z方向
	A = fx * fx + fy * fy;
	A = sqrt(A);
	A = (double)A / fz;
	A = atan(A);
	A = A * 180 / PI;
 
	*pAngleZ = A;
	
//	DBG_PRINT("=======angleZ：%.04f\r\n",*pAngleZ);

	return 0;
}

int SC7A20_GetXYZAngle(double* pAngleX, double* pAngleY, double* pAngleZ)
{
	double fx, fy, fz;
	double A;
	short Xa, Ya, Za;

	if (SC7A20_ReadAcceleration(&Xa, &Ya, &Za)) 
	{
		return -1;
	}
	
//	DBG_PRINT("Xa:%d \tYa:%d \tZa:%d \r\n",Xa,Ya,Za); 

	fx = Xa;
	fx *= 2.0 / 4096;
	fy = Ya;
	fy *= 2.0 / 4096;
	fz = Za;
	fz *= 2.0 / 4096;

	if (0 == fz)
	{
		A = 0.0;
	}
	else
	{
		A = fx * fx + fy * fy;
		A = sqrt(A);
		A = (double)A / fz;
		A = atan(A);
		A = A * 180 / PI;
	}
	*pAngleZ = A;
	
	if (0 == fx)
	{
		A = 0.0;
	}
	else
	{
		A = fz * fz + fy * fy;
		A = sqrt(A);
		A = (double)A / fx;
		A = atan(A);
		A = A * 180 / PI;
	}
	*pAngleX = A;

	if (0 == fy)
	{
		A = 0.0;
	}
	else
	{
		A = fx * fx + fz * fz;
		A = sqrt(A);
		A = (double)A / fy;
		A = atan(A);
		A = A * 180 / PI;
	}
	*pAngleY = A;
	
	return 0;
}

void SC7A20_EnableWakeUp(int level)
{
	g_sensor_write(0x20,0x27);//10HZ  XYZ轴使能
	g_sensor_write(0x21,0x04);//单击功能 高通滤波使能
	g_sensor_write(0x23,0x80);//BDU保持
	
	g_sensor_write(0x3A,10);//单击 阈值设置
	g_sensor_write(0x3B,0x7f);//阈值判断时间
	g_sensor_write(0x3C,0x6a);//持续时间
	g_sensor_write(0x38,0x15);//CLICK中断配置
	
	g_sensor_write(0x22,0x80); //CLICK中断 INT1
	//g_sensor_write(0x25,0x02);//CLICK中断 INT2
}

