#ifndef _UPGRADE_H_
#define _UPGRADE_H_

#define UPGRADE_TARGET_FILE "/cache/package.efw"

#define TFCARD_PATH         "/mnt/card"

typedef struct
{

    char fileName[128];
    char productModel[16];
    char fileVersion[16];
    char targetVersion[16];
    char md5[33];
    char suffix[6];

} UpgradeFileInfo_t;

void upgradeProcessStart(void);


#endif
