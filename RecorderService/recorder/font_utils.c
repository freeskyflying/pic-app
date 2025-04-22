#include <string.h>
#include <stdlib.h>
#include "font_utils.h"
#include "pnt_log.h"


// 传入OSD多字节，字体大小，颜色，返回ARGB1555字节的buffer及矩形的宽高 注意:传入的str若有中文需是GBK编码
int osd_str_to_argb(const unsigned char *str, int fontsize,unsigned short color, unsigned short backcolor, unsigned int buffwidth,
                    unsigned short *bufferaddr,  uint32_t *pwidth, uint32_t *pheight)
{   
    int ret;
    int max_line_count = 120;
    if(buffwidth == 1920){
        max_line_count = fontsize>=2?73:120;
    }else if(buffwidth == 1280){
        max_line_count = fontsize>=2?50:70;
    }else{
        max_line_count = fontsize>=2?20:60;
    }

    if (str == NULL || bufferaddr == NULL || pwidth == NULL || pheight  == NULL) {
        PNT_LOGE("param is err");
        return -1;
    }

    if(fontsize>10){
      fontsize = 2;
    }
    unsigned short *buffer = (unsigned short *)bufferaddr;
   
    int len = strlen((const char*)str);
    unsigned char *fontbuf = (unsigned char *)malloc(len * FONT_ASCI_SIZE);
    if (fontbuf == NULL) {
        PNT_LOGE("malloc err");
        return -1;
    }
    
    /************************提取字模***********************************/
    int i, j, k;
    unsigned char ch1, ch2;
    for (i = 0; i < len; ++i) {
        ch1 = *(str + i);
        ch2 = i + 1 < len ? *(str + i + 1) : 0;
        if (ch1 < 0xa1) {
            long pos = ch1 * FONT_ASCI_SIZE;
             memcpy(fontbuf + i * FONT_ASCI_SIZE,ascii_font_buffer+pos,FONT_ASCI_SIZE);
        }else if (ch2 >= 0xa1) {
            long pos = (94 * (ch1 - 0xa1) + (ch2 - 0xa1)) * FONT_ASCI_SIZE * 2;
            unsigned char tmp[FONT_ASCI_SIZE * 2];
            memcpy(tmp,chk_font_buffer+pos,FONT_ASCI_SIZE*2);
            for (j = 0; j < FONT_ASCI_SIZE; j++) {
              *(fontbuf + i * FONT_ASCI_SIZE + j) = tmp[j * 2];  
              *(fontbuf + i * FONT_ASCI_SIZE + FONT_ASCI_SIZE + j) = tmp[j * 2 + 1];
            }
            
            ++i;
        }
    }
    
    /************************缩放字库***********************************/
    if(len > max_line_count){
      *pheight = 16*(fontsize+1) * 2;
      *pwidth = max_line_count * 8 *(fontsize+1);

      /* 遍历单个字模 */
      for (i = 0; i < max_line_count; i++) {
        int offset_x = i * (8 * (fontsize+1));
        for (j = 0; j < FONT_ASCI_SIZE; j++) {
          unsigned char ch = *(fontbuf + i * FONT_ASCI_SIZE + j);
          int zoom, zoom_x=0, zoom_y=0;
          zoom = fontsize+1; //eg:zoom 3 ,col 8->24, row 16->48

          for (k = 0; k < 8; k++) {
            for (zoom_y = 0; zoom_y < zoom; zoom_y++){
              for (zoom_x  = 0; zoom_x < zoom; zoom_x++){
                *(buffer + offset_x + (j*zoom + zoom_y)*(*pwidth) + (k*zoom + zoom_x)) = BIT_I(ch, (7 - k)) ? color : backcolor;
              }
            }
          }
        }
      }
      
      //line 2
      for (i = 0; i < len - max_line_count; i++) {
        int offset_x = (i+max_line_count) * (8 * (fontsize+1));
        for (j = 0; j < FONT_ASCI_SIZE; j++) {
          unsigned char ch = *(fontbuf + (i+max_line_count) * FONT_ASCI_SIZE + j);
          int zoom, zoom_x=0, zoom_y=0;
          zoom = fontsize+1; //eg:zoom 3 ,col 8->24, row 16->48

          for (k = 0; k < 8; k++) {
            for (zoom_y = 0; zoom_y < zoom; zoom_y++){
              for (zoom_x  = 0; zoom_x < zoom; zoom_x++){
                *(buffer + offset_x +  (j*zoom + zoom_y)*(*pwidth) + ((FONT_ASCI_SIZE-1)*zoom /* + zoom_y*/)*(*pwidth) + (k*zoom + zoom_x)) = BIT_I(ch, (7 - k)) ? color : backcolor;
              }
            }
          }
        }
      }
    }else{
      *pheight = 16*(fontsize+1);
      *pwidth = len * 8 *(fontsize+1);
      for (i = 0; i < len; i++) {
        int offset_x = i * (8 * (fontsize+1));
        for (j = 0; j < FONT_ASCI_SIZE; j++) {
          unsigned char ch = *(fontbuf + i * FONT_ASCI_SIZE + j);
          int zoom, zoom_x=0, zoom_y=0;
          zoom = fontsize+1; //eg:zoom 3 ,col 8->24, row 16->48

          for (k = 0; k < 8; k++) {
            for (zoom_y = 0; zoom_y < zoom; zoom_y++){
              for (zoom_x  = 0; zoom_x < zoom; zoom_x++){
                *(buffer + offset_x + (j*zoom + zoom_y)*(*pwidth) + (k*zoom + zoom_x)) = BIT_I(ch, (7 - k)) ? color : backcolor;
              }
            }
          }
        }
      }
    }
    /***************************end*************************** */

    free(fontbuf);
    fontbuf = NULL;
    return 0;
}

// 传入OSD多字节，字体大小，颜色，返回ARGB1555字节的buffer及矩形的宽高 注意:传入的str若有中文需是GBK编码
int osd_str_to_double_color_argb(const unsigned char *str, int fontsize, int color2_len,unsigned short color1,  
                                 unsigned short color2,  unsigned short backcolor,unsigned short *bufferaddr, 
                                  uint32_t *pwidth, uint32_t *pheight)
{   
    int ret;
    if (str == NULL || bufferaddr == NULL || pwidth == NULL || pheight  == NULL) {
        PNT_LOGE("param is err");
        return -1;
    }
    if(fontsize>10){
      fontsize = 10;
    }
    unsigned short *buffer = (unsigned short *)bufferaddr;
   
    int len = strlen((const char*)str);
    unsigned char *fontbuf = (unsigned char *)malloc(len * FONT_ASCI_SIZE);
    if (fontbuf == NULL) {
        PNT_LOGE("malloc err");
        return -1;
    }

    /************************提取字模***********************************/
    int i, j, k;
    unsigned char ch1, ch2;
    for (i = 0; i < len; ++i) {
        ch1 = *(str + i);
        ch2 = i + 1 < len ? *(str + i + 1) : 0;
        if (ch1 < 0xa1) {
            long pos = ch1 * FONT_ASCI_SIZE;
             memcpy(fontbuf + i * FONT_ASCI_SIZE,ascii_font_buffer+pos,FONT_ASCI_SIZE);
        }else if (ch2 >= 0xa1) {
            long pos = (94 * (ch1 - 0xa1) + (ch2 - 0xa1)) * FONT_ASCI_SIZE * 2;
            unsigned char tmp[FONT_ASCI_SIZE * 2];
            memcpy(tmp,chk_font_buffer+pos,FONT_ASCI_SIZE*2);
            for (j = 0; j < FONT_ASCI_SIZE; j++) {
              *(fontbuf + i * FONT_ASCI_SIZE + j) = tmp[j * 2];  
              *(fontbuf + i * FONT_ASCI_SIZE + FONT_ASCI_SIZE + j) = tmp[j * 2 + 1];
            }
            
            ++i;
        }
    }
    
    /************************缩放字库***********************************/
    *pheight = 16*(fontsize+1);
    *pwidth = len * 8 *(fontsize+1);
    unsigned short color;
     //LOGC("------len is %d,high is %d,with %d\n",len,*pheight,*pwidth );
    /* 遍历单个字模 */
    for (i = 0; i < len; i++) {
      int offset_x = i * (8 * (fontsize+1));
      color = i<color2_len?color1:color2;
      for (j = 0; j < FONT_ASCI_SIZE; j++) {
        unsigned char ch = *(fontbuf + i * FONT_ASCI_SIZE + j);
        int zoom, zoom_x=0, zoom_y=0;
        zoom = fontsize+1; //eg:zoom 3 ,col 8->24, row 16->48

        for (k = 0; k < 8; k++) {
          for (zoom_y = 0; zoom_y < zoom; zoom_y++){
            for (zoom_x  = 0; zoom_x < zoom; zoom_x++){
              *(buffer + offset_x + (j*zoom + zoom_y)*(*pwidth) + (k*zoom + zoom_x)) = BIT_I(ch, (7 - k)) ? color : backcolor;
            }
          }
        }
      }
    }

    free(fontbuf);
    fontbuf = NULL;
    return 0;
}

