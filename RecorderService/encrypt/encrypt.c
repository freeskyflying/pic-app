#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <math.h>

#include "md5.h"

#define EFUSE_GET_MODULE_LEN	0xefe00000U
#define EFUSE_GET_MODULE		0xefe00001U
#define EFUSE_READ_USERSPACE	0xefe00045U
#define EFUSE_WRITE_USERSPACE	0xeee00045U

typedef struct
{
	
	unsigned int id;
	unsigned int len_byte;
	unsigned char* buf;
	
} lombo_efuse_module_t;

#pragma pack(1)

typedef struct
{
	
	unsigned char sn[16];
	unsigned char license[16];
	
} lombo_efuse_user_data_t;

#pragma pack()

#define LOGE	printf

static int GetCpuUUID(unsigned char* uuid)
{
	int len = 0, ret = 0;
	lombo_efuse_module_t efuse_module = { 0 };
	
	int fd = open("/dev/lombo-misc", O_RDWR);
	if (fd < 0)
	{
		LOGE("error: open lombo-misc failed.\n");
		ret = -1;
		goto out;
	}
	
	memset(&efuse_module, 0, sizeof(lombo_efuse_module_t));
	
	efuse_module.id = 0x07;
	
	ret = ioctl(fd, EFUSE_GET_MODULE_LEN, &efuse_module);
	if (0 != ret)
	{
		LOGE("error: get uuid len failed.\n");
		ret = -1;
		goto out;
	}
	
	efuse_module.buf = (unsigned char*)malloc(efuse_module.len_byte);
	if (NULL == efuse_module.buf)
	{
		LOGE("error: malloc uuid buf failed.\n");
		ret = -1;
		goto out;
	}
	
	memset(efuse_module.buf, 0, efuse_module.len_byte);
	
	ret = ioctl(fd, EFUSE_GET_MODULE, &efuse_module);
	if (0 != ret)
	{
		LOGE("error: get uuid failed.\n");
		ret = -1;
		goto out;
	}
	
	len = efuse_module.len_byte;
	
	memcpy(uuid, efuse_module.buf, efuse_module.len_byte);
	
out:
	
	if (fd>0)
	{
		close(fd);
		fd = -1;
	}

	if (efuse_module.buf)
	{
		free(efuse_module.buf);
		efuse_module.buf = NULL;
	}
	
	return len;
}

#define MD5_SIZE 16
static int MakeLicense(unsigned char* sn, unsigned char* license)
{
    int i = 0;
	char destStr[1024] = { 0 };
    unsigned char md5Value[MD5_SIZE];
	unsigned char cpu_uuid[16] = { 0 };
	int cpuUUIDLen = 0;
	
    MD5Context md5;
	
	cpuUUIDLen = GetCpuUUID(cpu_uuid);
	if (0 > cpuUUIDLen)
	{
		return -1;
	}
	
	// desStr: sn-uuid-sn
	sprintf(destStr, "%s-", sn);
	
	for (int i=0; i<cpuUUIDLen; i++)
	{
		sprintf(destStr, "%s%02x", destStr, cpu_uuid[i]);
	}
	
	sprintf(destStr, "%s-%s", destStr, sn);
	
	LOGE("desStr: %s\n", destStr);
	
    // init md5
    MD5Init(&md5);
    MD5Update(&md5, (unsigned char*)destStr, strlen(destStr));
    MD5Final(&md5, md5Value);
	
	memcpy(license, md5Value, MD5_SIZE);
	
    // convert md5 value to md5 string
	memset(destStr, 0, sizeof(destStr));
    for(i = 0; i < MD5_SIZE; i++)
    {
        snprintf(destStr + i*2, 2+1, "%02x", md5Value[i]);
    }
	LOGE("License: %s\n", destStr);
	
	return MD5_SIZE;
}

static int GetUserData(lombo_efuse_user_data_t* user_data)
{
	int len = 0, ret = 0;
	unsigned char* buf = NULL;
	lombo_efuse_module_t efuse_module = { 0 };
	
	int fd = open("/dev/lombo-misc", O_RDWR);
	if (fd < 0)
	{
		LOGE("error: open lombo-misc failed.\n");
		ret = -1;
		goto out;
	}
	
	memset(&efuse_module, 0, sizeof(lombo_efuse_module_t));
	
	efuse_module.id = 0x45;
	
	ret = ioctl(fd, EFUSE_GET_MODULE_LEN, &efuse_module);
	if (0 != ret)
	{
		LOGE("error: get uuid len failed.\n");
		ret = -1;
		goto out;
	}
	
	buf = (unsigned char*)malloc(efuse_module.len_byte);
	if (NULL == buf)
	{
		LOGE("error: malloc uuid buf failed.\n");
		ret = -1;
		goto out;
	}
	
	memset(buf, 0, efuse_module.len_byte);
	
	ret = ioctl(fd, EFUSE_READ_USERSPACE, buf);
	if (0 != ret)
	{
		LOGE("error: get uuid failed.\n");
		ret = -1;
		goto out;
	}
	
	memcpy(user_data, buf, sizeof(lombo_efuse_user_data_t));

out:

	if (fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
	if (NULL != buf)
	{
		free(buf);
		buf = NULL;
	}

	return ret;
}

static int SetUserData(lombo_efuse_user_data_t* user_data)
{
	int len = 0, ret = 0;
	unsigned char* buf = NULL;
	lombo_efuse_module_t efuse_module = { 0 };
	
	int fd = open("/dev/lombo-misc", O_RDWR);
	if (fd < 0)
	{
		LOGE("error: open lombo-misc failed.\n");
		ret = -1;
		goto out;
	}
	
	memset(&efuse_module, 0, sizeof(lombo_efuse_module_t));
	
	efuse_module.id = 0x45;
	
	ret = ioctl(fd, EFUSE_GET_MODULE_LEN, &efuse_module);
	if (0 != ret)
	{
		LOGE("error: get uuid len failed.\n");
		ret = -1;
		goto out;
	}
	
	buf = (unsigned char*)malloc(efuse_module.len_byte);
	if (NULL == buf)
	{
		LOGE("error: malloc uuid buf failed.\n");
		ret = -1;
		goto out;
	}
	
	memset(buf, 0, efuse_module.len_byte);

	memcpy(buf, user_data, sizeof(lombo_efuse_user_data_t));

	ret = ioctl(fd, EFUSE_WRITE_USERSPACE, buf);
	if (0 != ret)
	{
		LOGE("error: get uuid failed.\n");
		ret = -1;
		goto out;
	}

out:

	if (fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
	if (buf)
	{
		free(buf);
		buf = NULL;
	}

	return ret;
}

static int GetLicense(unsigned char* license)
{
	lombo_efuse_user_data_t user_data = { 0 };
	
	if (0 == GetUserData(&user_data))
	{
		memcpy(license, user_data.license, sizeof(user_data.license));
		return 0;
	}
	
	return -1;
}

static int SetLicense(unsigned char* license)
{
	lombo_efuse_user_data_t user_data = { 0 };
	
	if (GetUserData(&user_data))
	{
		LOGE("setLicense: get userdata failed.\n");
		return -1;
	}
	
	memcpy(user_data.license, license, sizeof(user_data.license));
	
	if (SetUserData(&user_data))
	{
		LOGE("setLicense: set userdata failed.\n");
		return -1;
	}
	
	return 0;
}

int SetSerilaNum(unsigned char* sn)
{
	lombo_efuse_user_data_t user_data = { 0 };
	
	if (GetUserData(&user_data))
	{
		LOGE("setSn: get userdata failed.\n");
		return -1;
	}
	
	memcpy(user_data.sn, sn, sizeof(user_data.sn));
	
	if (SetUserData(&user_data))
	{
		LOGE("setSn: set userdata failed.\n");
		return -1;
	}
	
	return 0;
}

int GetSerilaNum(unsigned char* sn)
{
	lombo_efuse_user_data_t user_data = { 0 };
	
	if (0 == GetUserData(&user_data))
	{
		memcpy(sn, user_data.sn, sizeof(user_data.sn));
		return 0;
	}
	
	return -1;
}

int EncryptCheck(void)
{
	unsigned char licenseLocal[16] = { 0 };
	unsigned char licenseGen[16] = { 0 };
	unsigned char sn[16] = { 0 };
	
	if (GetLicense(licenseLocal))
	{
		LOGE("get local license failed.\n");
		return -1;
	}
	
	if (GetSerilaNum(sn))
	{
		LOGE("get local sn failed.\n");
		return -2;
	}
	
	if (MakeLicense(sn, licenseGen) < 0)
	{
		LOGE("make license failed.\n");
		return -3;
	}
	
	if (memcmp(licenseLocal, licenseGen, sizeof(licenseGen)) != 0)
	{
		LOGE("license match failed.\n");
		return -4;
	}
	
	return 0;
} 

int EncryptWrite(void)
{
	unsigned char sn[16] = { 0 };
	unsigned char licenseGen[16] = { 0 };
	
	if (GetSerilaNum(sn))
	{
		LOGE("Encrypt: get local sn failed.\n");
		return -1;
	}
	
	if (MakeLicense(sn, licenseGen) < 0)
	{
		LOGE("Encrypt: make license failed.\n");
		return -2;
	}
	
	if (SetLicense(licenseGen))
	{
		LOGE("Encrypt: set license failed.\n");
		return -3;
	}
	
	if (EncryptCheck())
	{
		LOGE("Encrypt: checked failed.\n");
		return -3;
	}
	
	return 0;
}
