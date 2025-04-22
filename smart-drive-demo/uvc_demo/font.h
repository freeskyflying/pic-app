/*
 * Coding: UTF-8
 * Copyright (c) 2017 Liming Shao <lmshao@163.com>
 *
 */

#ifndef HZK_OSD_FONT_H
#define HZK_OSD_FONT_H

#include<stdio.h>
#include <string.h>

void str_to_bmp(unsigned char *str, char *bmp_file);

void get_hzk_code(unsigned char *c, unsigned char buff[]);

void fill_pixels(unsigned char *bmp_data, unsigned char *font_data, int type, int seq, int bmp_width);

int text_to_bmp(unsigned char *str, int len, char *bmpdata);

int get_bmp_data_len(char * str);


void text_to_bmp2(unsigned char *str, int len, char *bmpdata);

#endif //HZK_OSD_FONT_H
