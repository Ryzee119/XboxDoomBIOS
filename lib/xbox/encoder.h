// SPDX-License-Identifier: CC0-1.0

#ifndef ENCODER_H
#define ENCODER_H

#include "video.h"
#include "xbox.h"

void xbox_encoder_configure(uint32_t mode_coding, display_information_t *display_info);
uint8_t xbox_encoder_detect(void);

#define FOCUS_SDTVI_IHO_16      0x00
#define FOCUS_SDTVI_IVO_16      0x02
#define FOCUS_SDTVI_IHW_16      0x04
#define FOCUS_SDTVI_VSC_16      0x06
#define FOCUS_SDTVI_HSCD_8      0x08
#define FOCUS_SDTVI_HSCU_8      0x09
#define FOCUS_SDTVI_FLK_16      0x26
#define FOCUS_SDTVI_FIFO_LAT_16 0x38

#define FOCUS_CTRL_BYPASS_16            0x0A
#define FOCUS_CTRL_CR_16                0x0C
#define FOCUS_CTRL_MISC_16              0x0E
#define FOCUS_CTRL_PWR_MGNT_16          0xA0
#define FOCUS_CTRL_VIDCNTRL0_16         0x92
#define FOCUS_CTRL_DACCNTL_16           0x9E
#define FOCUS_CTRL_9A_16                0x9A
#define FOCUS_CLOCK_NCON_32             0x10
#define FOCUS_CLOCK_NCOD_32             0x14
#define FOCUS_CLOCK_PLLM_PUMP_16        0x18
#define FOCUS_CLOCK_PLLN_16             0x1A
#define FOCUS_CLOCK_POST_DIVIDER_16     0x1C
#define FOCUS_SDTVO_CHR_FREQ_32         0x40
#define FOCUS_SDTVO_MISC46_8            0x46
#define FOCUS_SDTVO_MISC47_8            0x47
#define FOCUS_SDTVO_HSYNC_WID_8         0x48
#define FOCUS_SDTVO_BURST_WID_8         0x49
#define FOCUS_SDTVO_BPORCH_8            0x4A
#define FOCUS_SDTVO_CB_BURST_8          0x4B
#define FOCUS_SDTVO_CR_BURST_8          0x4C
#define FOCUS_SDTVO_BLACK_LVL_16        0x4E
#define FOCUS_SDTVO_BLANK_LVL16         0x50
#define FOCUS_SDTVO_CB_GAIN_8           0x60
#define FOCUS_SDTVO_CR_GAIN_8           0x62
#define FOCUS_SDTVO_BR_WAY_8            0x69
#define FOCUS_SDTVO_FR_PORCH_8          0x6C
#define FOCUS_SDTVO_1ST_LINE_8          0x73
#define FOCUS_SDTVO_MISC_74_8           0x74
#define FOCUS_SDTVO_VBI_BL_LVL_16       0x7C
#define FOCUS_SDTVO_WSS_CONFIG_8        0x80
#define FOCUS_SDTVO_WSS_CLK_16          0x81
#define FOCUS_SDTVO_WSS_LNF1_8          0x89
#define FOCUS_SDTVO_WSS_LNF0_8          0x8A
#define FOCUS_SDTVO_CCC_16              0xB2
#define FOCUS_SDTVO_PR_SC_8             0xC0
#define FOCUS_SDTVO_PB_SC_8             0xC1
#define FOCUS_SDTVO_CC_BLANK_SPL_16     0xB6
#define FOCUS_SDTVO_LUMU_BWIDTH_16      0xC2
#define FOCUS_HDTV_OUT_HD_FP_SYNC_16    0x94
#define FOCUS_HDTV_OUT_HD_YOFF_BP_16    0x96
#define FOCUS_HDTV_OUT_SYNC_DL_16       0x98
#define FOCUS_HDTV_OUT_HACT_ST_16       0xB8 // HDTV Horizontal Active Start
#define FOCUS_HDTV_OUT_HACT_WD_16       0xBA // HDTV Horizontal Active Width
#define FOCUS_HDTV_OUT_VACT_ST_16       0xBC // HDTV Vertical Active Start
#define FOCUS_HDTV_OUT_VACT_HT_16       0xBE // HDTV Vertical Active Height
#define FOCUS_COL_MATRIX_RED_COEFF_16   0xA2
#define FOCUS_COL_MATRIX_GREEN_COEFF_16 0xA4
#define FOCUS_COL_MATRIX_BLUE_COEFF_16  0xA6
#define FOCUS_COL_MATRIX_RED_SCALE_16   0xA8
#define FOCUS_COL_MATRIX_GREEN_SCALE_16 0xAA
#define FOCUS_COL_MATRIX_BLUE_SCALE_16  0xAC
#endif
