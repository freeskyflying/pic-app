#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gpio_led.h"

void setLedStatus(char* path, char brightness)
{
    char cmd[64] = { 0 };
    
    sprintf(cmd, "%s/brightness", path);

    FILE* pf = fopen(cmd, "w");
    if (pf == NULL)
    {
        return;
    }
    brightness = brightness + '0';
    fwrite(&brightness, 1, 1, pf);
    fclose(pf);
}

