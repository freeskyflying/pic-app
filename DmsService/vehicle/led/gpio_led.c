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

    int fd = open(cmd, O_RDWR);
    if (fd < 0)
    {
        return;
    }
    brightness = brightness + '0';
    write(fd, &brightness, 1);
    close(fd);
    //sprintf(cmd, "echo %d > %s/brightness", brightness, path);

    //system(cmd);
}

