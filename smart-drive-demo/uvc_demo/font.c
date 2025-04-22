/*
 * Coding: UTF-8
 * Copyright (c) 2017 Liming Shao <lmshao@163.com>
 *
 */

#include <stdlib.h>
#include "font.h"
//#include "bmp.h"
#include "mask.h"
#include "custom_mask.h"

int get_bmp_data_len(char * str)
{
	int len = strlen((const char *) str);
    int image_width = len * 8;
    int image_height = 16;
	
	int bitcount = 16;	/* ARGB1555, 16 bits */
	
	int bmp_data_len = (image_width + 7) / 8 * 8 * image_height * 2;	// width align to 8
	return bmp_data_len;
}

int text_to_bmp(unsigned char *str, int len, char *bmpdata)
{
	if (str == NULL || strlen(str) <= 0)
	{
		return -1;
	}
	
	if (bmpdata == NULL)
	{
		return -1;
	}
	//printf("text to bmp [%s] \n", str);
	
	//int len = strlen((const char *) str);
    for(int i=0; i<len; i++) {
        if(str[i] & 0x80) {     /* Chinese character */
            unsigned char character[32];
            get_hzk_code(str+i, character);
            fill_pixels(bmpdata, character, 0, i, len);
            i++;
        } else {    /* ASCII character */
            unsigned char character[16];

            if(str[i]<0x20) {
                memcpy(character, ascii_mask, 16);
            } else{
                memcpy(character, &ascii_mask[(str[i]-0x1f)*16], 16);
            }

            fill_pixels(bmpdata, character, 1, i, len);
        }
    }
	
	return 0;
}
#if 0
void str_to_bmp(unsigned char *str, char *bmp_file)
{
    int len = strlen((const char *) str);
    int image_width = len * 8;
    int image_height = 16;

    printf("String: \"%s\"\n", str);
    printf("Generate bmp file: %s\n", bmp_file);

    BITMAP_S *bmp;
    create_bmp_file(image_width, image_height, &bmp);
    printf("Size of bmp: %d Bytes\n",(*bmp).bfHeader.bfSize);
	text_to_bmp(str, bmp->bmpData);
#if 0

    for(int i=0; i<len; i++) {
        if(str[i] & 0x80) {     /* Chinese character */
            unsigned char character[32];
            get_hzk_code(str+i, character);
            fill_pixels( (*bmp).bmpData, character, 0, i, len);
            i++;
        } else {    /* ASCII character */
            unsigned char character[16];

            if(str[i]<0x20) {
                memcpy(character, ascii_mask, 16);
            } else{
                memcpy(character, &ascii_mask[(str[i]-0x1f)*16], 16);
            }

            fill_pixels( (*bmp).bmpData, character, 1, i, len);
        }
    }
#endif	
    write_bmp_file(bmp, bmp_file);
    printf("Save bmp file successfully!\n");
    free(bmp->bmpData);
    free(bmp);
}
#endif
#if 0
void fill_pixels(unsigned char *bmp_data, unsigned char *font_data,
                 int type, int seq, int bmp_width)
{
    unsigned char *pixel = bmp_data;
    if(!type) {     /* Chinese character */
        int col, i, row;
        for(row=0; row<16; row++) {
            for (i = 0; i < 2; i++) { /* 2 bytes */
                for (col = 0; col < 8; col++) { /* 8 bits */
                    //int pos = (15 - row) * 8 * bmp_width + (seq + i) * 8 + col;//»áµßµ¹£¬BMPÍ¼Æ¬ÐèÒªµßµ¹
					int pos = row * 8 * bmp_width + (seq + i) * 8 + col;
                    if (font_data[row * 2 + i] & (0x80 >> col)) {
                        pixel[pos * 2] = 0x7c;
                        pixel[pos * 2 + 1] = 0x00;
                    } else {
                        pixel[pos * 2] = 0xff;
                        pixel[pos * 2 + 1] = 0xff;
                    }
                }
            }
        }
    } else {
        int col, row;
        for(row=0; row<16; row++) {
            for (col = 0; col < 8; col++) {     /* 8 bits */
                //int pos = (15 - row) * 8 * bmp_width + seq * 8 + col;//»áµßµ¹£¬BMPÍ¼Æ¬ÐèÒªµßµ¹
				int pos = row * 8 * bmp_width + seq * 8 + col;
                if (font_data[row] & (0x80 >> col)) {
                    pixel[pos * 2] = 0x7c;
                    pixel[pos * 2 + 1] = 0x00;
                } else {
                    pixel[pos * 2] = 0xff;
                    pixel[pos * 2 + 1] = 0xff;
                }
            }
        }
    }
}
#else
void fill_pixels(unsigned char *bmp_data, unsigned char *font_data,
                 int type, int seq, int bmp_width)
{
    unsigned char *pixel = bmp_data;
    if(!type) {     /* Chinese character */
        int col, i, row;
        for(row=0; row<16; row++) {
            for (i = 0; i < 2; i++) { /* 2 bytes */
                for (col = 0; col < 8; col++) { /* 8 bits */
                    //int pos = (15 - row) * 8 * bmp_width + (seq + i) * 8 + col;//»áµßµ¹£¬BMPÍ¼Æ¬ÐèÒªµßµ¹
					int pos = row * 8 * bmp_width + (seq + i) * 8 + col;
                    if (font_data[row * 2 + i] & (0x80 >> col)) {
                        pixel[pos * 4] = 0x00;
                        pixel[pos * 4 + 1] = 0x00;
                        pixel[pos * 4 + 2] = 0xff;
                        pixel[pos * 4 + 3] = 0xff;
                    } else {
                        pixel[pos * 4] = 0xff;
                        pixel[pos * 4 + 1] = 0xff;
                        pixel[pos * 4 + 2] = 0xff;
                        pixel[pos * 4 + 3] = 0x00;
                    }
                }
            }
        }
    } else {
        int col, row;
        for(row=0; row<16; row++) {
            for (col = 0; col < 8; col++) {     /* 8 bits */
                //int pos = (15 - row) * 8 * bmp_width + seq * 8 + col;//»áµßµ¹£¬BMPÍ¼Æ¬ÐèÒªµßµ¹
				int pos = row * 8 * bmp_width + seq * 8 + col;
                if (font_data[row] & (0x80 >> col)) {
                    pixel[pos * 4] = 0x00;
                    pixel[pos * 4 + 1] = 0x00;
                    pixel[pos * 4 + 2] = 0xff;
                    pixel[pos * 4 + 3] = 0xff;
                } else {
                    pixel[pos * 4] = 0xff;
                    pixel[pos * 4 + 1] = 0xff;
                    pixel[pos * 4 + 2] = 0xff;
                    pixel[pos * 4 + 3] = 0x00;
                }
            }
        }
    }
}

#endif

void get_hzk_code(unsigned char *c, unsigned char buff[])
{
    FILE *HZK;
    if((HZK = fopen("/usr/share/hzk16", "rb")) == NULL) {
        printf("Can't open font file!");
        return;
    }

    int hv = *c - 0xa1;
    int lv = *(c+1) - 0xa1;
    long offset = (long) (94 * hv + lv) * 32;

    fseek(HZK, offset, SEEK_SET);
    fread(buff, 32, 1, HZK);
    fclose(HZK);
}

void fill_pixels2(unsigned char *bmp_data, unsigned char *font_data,
                 int type, int seq, int bmp_width)
{
    unsigned char *pixel = bmp_data;
    if(!type) {     /* Chinese character */
        int col, i, row;
        for(row=0; row<32; row++) {
            for (i = 0; i < 2; i++) { /* 2 bytes */
                for (col = 0; col < 8; col++) { /* 8 bits */
                    //int pos = (15 - row) * 8 * bmp_width + (seq + i) * 8 + col;//»áµßµ¹£¬BMPÍ¼Æ¬ÐèÒªµßµ¹
					int pos = row * 16 * bmp_width + seq * 16 + i * 8 + col;
                    if (font_data[row * 2 + i] & (0x80 >> col)) {
                        pixel[pos * 4] = 0x00;
                        pixel[pos * 4 + 1] = 0x00;
                        pixel[pos * 4 + 2] = 0xff;
                        pixel[pos * 4 + 3] = 0xff;
                    } else {
                        pixel[pos * 4] = 0xff;
                        pixel[pos * 4 + 1] = 0xff;
                        pixel[pos * 4 + 2] = 0xff;
                        pixel[pos * 4 + 3] = 0x00;
                    }
                }
            }
        }
    } 
}

void text_to_bmp2(unsigned char *str, int len, char *bmpdata)
{
    unsigned char *find = NULL;
    unsigned int i = 0, index = 0;
    char character[80] = {0};
    
    for (i = 0; i < len; i++) {
        if(str[i] & 0x80) {
            i++;
            continue;
        } else {
            find = strchr(char_table, str[i]);
            if (find) {
                index = find - char_table;
                //printf("chart:%c(%#x) find:%p index:%d\n", str[i], str[i], find, index);
                if(str[i] < 0x20) {
                    memcpy(character, char_bits[0], 64);
                } else{
                    if (index >= char_count) {
                        printf("find %c fail. index:%d\n", str[i], index);
                        continue;
                    }
                    memcpy(character, char_bits[index], 64);
                }
                //printf("1\n");
                fill_pixels2(bmpdata, character, 0, i, len);
               // printf("2\n");
            }
        }
    } 
}

