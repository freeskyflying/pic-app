#ifndef _FONT_UTILS_H_
#define _FONT_UTILS_H_

#include "typedef.h"
#include "font_ascii_data.h"
#include "font_hzk16_data.h"

#define HIFB_RED_1555           0xFC00//0x80ff
#define HIFB_GREEN_1555         0x83E0
#define HIFB_BLUE_1555          0x801F
#define HIFB_WHITE_1555         0xFFFF
#define HIFB_BLACK_1555         0x8000
#define HIFB_LINE_1555          0x32FF 
#define FONT_IMAGE_WIDTH        1920
#define FONT_IMAGE_HEIGHT       400 
#define FONT720P_IMAGE_WIDTH    1280
#define FONT720P_IMAGE_HEIGHT   300

#define FONT_ASCI_SIZE          16
#define BIT_I(ch, i)            ((ch)&(1<<i))

#ifdef __cplusplus
extern "C" {
#endif

int osd_str_to_argb(const unsigned char *str, int fontsize, unsigned short color, unsigned short backcolor, unsigned int buffwidth,
                    unsigned short *bufferaddr, uint32_t *pwidth, uint32_t *pheight);

int osd_str_to_double_color_argb(const unsigned char *str, int fontsize, int color2_len,unsigned short color1,  
                                 unsigned short color2,  unsigned short backcolor,unsigned short *bufferaddr, 
                                  uint32_t *pwidth, uint32_t *pheight);

#ifdef __cplusplus
}
#endif

#endif
