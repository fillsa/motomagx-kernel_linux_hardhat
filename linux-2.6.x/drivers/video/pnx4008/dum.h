/*
 * linux/drivers/video/pnx4008/dum.h
 *
 * 2005 (c) Koninklijke Philips N.V. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __PNX008_DUM_H__
#define __PNX008_DUM_H__

#include <asm/arch/platform.h>

/* DUM CFG ADDRESSES *********************************************************/
#define DUM_CH_BASE_ADR       (PNX4008_DUMCONF_BASE + 0x0000)
#define MIN                   0x00
#define MAX                   0x04
#define CONF                  0x08
#define STAT                  0x0C
#define CTRL                  0x10
#define DUM_CONF_ADR          (PNX4008_DUMCONF_BASE + 0x4000)
#define DUM_CTRL_ADR          (PNX4008_DUMCONF_BASE + 0x4004)
#define DUM_STAT_ADR          (PNX4008_DUMCONF_BASE + 0x4008)
#define DUM_DECODE_ADR        (PNX4008_DUMCONF_BASE + 0x400C)
#define DUM_COM_BASE_ADR      (PNX4008_DUMCONF_BASE + 0x4010)
#define DUM_SYNC_C_ADR        (PNX4008_DUMCONF_BASE + 0x4014)
#define DUM_CLK_DIV_ADR       (PNX4008_DUMCONF_BASE + 0x4018)
#define DUM_DIRTY_LOW_ADR     (PNX4008_DUMCONF_BASE + 0x4020)
#define DUM_DIRTY_HIGH_ADR    (PNX4008_DUMCONF_BASE + 0x4024)
#define DUM_FORMAT_ADR        (PNX4008_DUMCONF_BASE + 0x4028)
#define DUM_WTCFG1_ADR        (PNX4008_DUMCONF_BASE + 0x4030)
#define DUM_RTCFG1_ADR        (PNX4008_DUMCONF_BASE + 0x4034)
#define DUM_WTCFG2_ADR        (PNX4008_DUMCONF_BASE + 0x4038)
#define DUM_RTCFG2_ADR        (PNX4008_DUMCONF_BASE + 0x403C)
#define DUM_TCFG_ADR          (PNX4008_DUMCONF_BASE + 0x4040)
#define DUM_OUTP_FORMAT1_ADR  (PNX4008_DUMCONF_BASE + 0x4044)
#define DUM_OUTP_FORMAT2_ADR  (PNX4008_DUMCONF_BASE + 0x4048)
#define DUM_SYNC_MODE_ADR     (PNX4008_DUMCONF_BASE + 0x404C)
#define DUM_SYNC_OUT_C_ADR    (PNX4008_DUMCONF_BASE + 0x4050)

#define DUM_CH_BASE           ((volatile u32*)(DUM_CH_BASE_ADR))
#define DUM_CONF              ((volatile u32*)(DUM_CONF_ADR))
#define DUM_CTRL              ((volatile u32*)(DUM_CTRL_ADR))
#define DUM_STAT              ((volatile u32*)(DUM_STAT_ADR))
#define DUM_DECODE            ((volatile u32*)(DUM_DECODE_ADR))
#define DUM_COM_BASE          ((volatile u32*)(DUM_COM_BASE_ADR))
#define DUM_SYNC_C            ((volatile u32*)(DUM_SYNC_C_ADR))
#define DUM_CLK_DIV           ((volatile u32*)(DUM_CLK_DIV_ADR))
#define DUM_DIRTY_LOW         ((volatile u32*)(DUM_DIRTY_LOW_ADR))
#define DUM_DIRTY_HIGH        ((volatile u32*)(DUM_DIRTY_HIGH_ADR))
#define DUM_FORMAT            ((volatile u32*)(DUM_FORMAT_ADR))
#define DUM_WTCFG1            ((volatile u32*)(DUM_WTCFG1_ADR))
#define DUM_RTCFG1            ((volatile u32*)(DUM_RTCFG1_ADR))
#define DUM_WTCFG2            ((volatile u32*)(DUM_WTCFG2_ADR))
#define DUM_RTCFG2            ((volatile u32*)(DUM_RTCFG2_ADR))
#define DUM_TCFG              ((volatile u32*)(DUM_TCFG_ADR))
#define DUM_OUTP_FORMAT1      ((volatile u32*)(DUM_OUTP_FORMAT1_ADR))
#define DUM_OUTP_FORMAT2      ((volatile u32*)(DUM_OUTP_FORMAT2_ADR))
#define DUM_SYNC_MODE         ((volatile u32*)(DUM_SYNC_MODE_ADR))
#define DUM_SYNC_OUT_C        ((volatile u32*)(DUM_SYNC_OUT_C_ADR))

/* DUM SLAVE ADDRESSES *********************************************************/
#define DUM_SLAVE_WRITE_ADR      (PNX4008_DUM_MAINCFG_BASE + 0x0000000)
#define DUM_SLAVE_READ1_I_ADR    (PNX4008_DUM_MAINCFG_BASE + 0x1000000)
#define DUM_SLAVE_READ1_R_ADR    (PNX4008_DUM_MAINCFG_BASE + 0x1000004)
#define DUM_SLAVE_READ2_I_ADR    (PNX4008_DUM_MAINCFG_BASE + 0x1000008)
#define DUM_SLAVE_READ2_R_ADR    (PNX4008_DUM_MAINCFG_BASE + 0x100000C)

#define DUM_SLAVE_WRITE_W  ((volatile u32*)(DUM_SLAVE_WRITE_ADR))
#define DUM_SLAVE_WRITE_HW ((volatile uint16_t*)(DUM_SLAVE_WRITE_ADR))
#define DUM_SLAVE_READ1_I  ((volatile uint8_t*)(DUM_SLAVE_READ1_I_ADR))
#define DUM_SLAVE_READ1_R  ((volatile uint16_t*)(DUM_SLAVE_READ1_R_ADR))
#define DUM_SLAVE_READ2_I  ((volatile uint8_t*)(DUM_SLAVE_READ2_I_ADR))
#define DUM_SLAVE_READ2_R  ((volatile uint16_t*)(DUM_SLAVE_READ2_R_ADR))

/* Sony display register addresses *********************************************/
#define DISP_0_REG            (0x00)
#define DISP_1_REG            (0x01)
#define DISP_CAL_REG          (0x20)
#define DISP_ID_REG           (0x2A)
#define DISP_XMIN_L_REG       (0x30)
#define DISP_XMIN_H_REG       (0x31)
#define DISP_YMIN_REG         (0x32)
#define DISP_XMAX_L_REG       (0x34)
#define DISP_XMAX_H_REG       (0x35)
#define DISP_YMAX_REG         (0x36)
#define DISP_SYNC_EN_REG      (0x38)
#define DISP_SYNC_RISE_L_REG  (0x3C)
#define DISP_SYNC_RISE_H_REG  (0x3D)
#define DISP_SYNC_FALL_L_REG  (0x3E)
#define DISP_SYNC_FALL_H_REG  (0x3F)
#define DISP_PIXEL_REG        (0x0B)
#define DISP_DUMMY1_REG       (0x28)
#define DISP_DUMMY2_REG       (0x29)
#define DISP_TIMING_REG       (0x98)
#define DISP_DUMP_REG         (0x99)

/* Sony display constants ******************************************************/
#define SONY_ID1              (0x22)
#define SONY_ID2              (0x23)

/* Philips display register addresses ******************************************/
#define PH_DISP_ORIENT_REG    (0x003)
#define PH_DISP_YPOINT_REG    (0x200)
#define PH_DISP_XPOINT_REG    (0x201)
#define PH_DISP_PIXEL_REG     (0x202)
#define PH_DISP_YMIN_REG      (0x406)
#define PH_DISP_YMAX_REG      (0x407)
#define PH_DISP_XMIN_REG      (0x408)
#define PH_DISP_XMAX_REG      (0x409)

/* Misc constants **************************************************************/
#define NO_VALID_DISPLAY_FOUND      (0)
#define DISPLAY2_IS_NOT_CONNECTED   (0)

/* EXPORTED MACROS *************************************************************/

#define DUM_CH_REG_P(ch_no,offset) (volatile u32*)(DUM_CH_BASE_ADR + (ch_no)*0x100 + (offset))

#define DUM_CH_MIN_WR(ch_no, val) (*DUM_CH_REG_P((ch_no), MIN) = (val))
#define DUM_CH_MIN_RD(ch_no) ((*DUM_CH_REG_P((ch_no),MIN))&0xFFFFE)

#define DUM_CH_MAX_WR(ch_no, val) (*DUM_CH_REG_P((ch_no), MAX) = (val))
#define DUM_CH_MAX_RD(ch_no) ((*DUM_CH_REG_P((ch_no),MAX))&0xFFFFE)

#define DUM_CH_CONF_WR(ch_no, sync_val, dirty_det, rdy_int, sync_en, pad)\
   (*DUM_CH_REG_P(ch_no,CONF) = (((sync_val)<<8) + ((dirty_det)<<5) + ((rdy_int)<<4) + ((sync_en)<<2) + ((pad)<<0)))
#define CustDUM_CH_CONF_WR(ch_no, sync_val, dirty_det, rdy_int, sync_en, pad)\
   				(((sync_val)<<8) + ((dirty_det)<<5) + ((rdy_int)<<4) + ((sync_en)<<2) + ((pad)<<0))
#define DUM_CH_CONF_RD(ch_no) ((*DUM_CH_REG_P((ch_no),CONF))&0x1FF37)
#define DUM_CH_RDY_EN(ch_no) ((*DUM_CH_REG_P(ch_no,CONF)) |= BIT(4))
#define DUM_CH_RDY_DIS(ch_no) ((*DUM_CH_REG_P(ch_no,CONF)) &= ~BIT(4))
#define DUM_CH_SYNC_DIS(ch_no) ((*DUM_CH_REG_P(ch_no,CONF)) &= ~BIT(2))
#define DUM_CH_RDY_EN_GET(ch_no) (((*DUM_CH_REG_P(ch_no,CONF))&BIT(4))>>4)
#define DUM_CH_SYNCPOS_GET(ch_no) (((*DUM_CH_REG_P(ch_no,CONF))>>8) & 0x1FF)
#define DUM_CH_SYNCPOS_SET(ch_no, val) ((*DUM_CH_REG_P(ch_no,CONF)) = (((*DUM_CH_REG_P(ch_no,CONF))&0xFF) | ((val)<<8)))

#define DUM_CH_RDY_RESET(ch_no) (*DUM_CH_REG_P(ch_no, CTRL) = (BIT(2)))
#define DUM_CH_DIRTY_RESET(ch_no) (*DUM_CH_REG_P(ch_no, CTRL) = (BIT(1)))
#define DUM_CH_DIRTY_SET(ch_no) (*DUM_CH_REG_P(ch_no, CTRL) = (BIT(0)))

#define DUM_CH_STAT_RD(ch_no) ((*DUM_CH_REG_P((ch_no),STAT))&0x7)
#define DUM_CH_DIRTY_GET(ch_no) ((*DUM_CH_REG_P(ch_no,STAT))&BIT(0))
#define DUM_CH_S_DIRTY_GET(ch_no) (((*DUM_CH_REG_P(ch_no,STAT))&BIT(1))>>1)
#define DUM_CH_RDY_GET(ch_no) (((*DUM_CH_REG_P(ch_no,STAT))&BIT(2))>>2)

#define DUM_CONF_WR(sync_neg_edge, round_robin, mux_int, synced_dirty_flag_int, dirty_flag_int, error_int, pf_empty_int, sf_empty_int, bac_dis_int)\
   (*DUM_CONF = (((sync_neg_edge)<<10) + ((round_robin)<<9) + ((mux_int)<<7) + ((synced_dirty_flag_int)<<6) + ((dirty_flag_int)<<5) + ((error_int)<<4) + ((pf_empty_int)<<2) + ((sf_empty_int)<<1) + ((bac_dis_int)<<0)))
#define DUM_CONF_RD ((*DUM_CONF)&0x6F7)
#define DUM_INT_EN_GET ((*DUM_CONF)&0xF7)
#define DUM_PFEMPTY_EN ((*DUM_CONF) |= BIT(2))
#define DUM_PFEMPTY_DIS ((*DUM_CONF) &=~BIT(2))

#define DUM_MUX_RESET (*DUM_CTRL = (BIT(4)))
#define DUM_RESET (*DUM_CTRL = (BIT(3)))
#define DUM_DIS_IDLE (*DUM_CTRL = (BIT(2)))
#define DUM_DIS_TRIGGERED (*DUM_CTRL = (BIT(1)))
#define DUM_BAC_ENABLE_SET (*DUM_CTRL = (BIT(0)))

#define DUM_BAC_ENABLE_GET ((*DUM_STAT)&BIT(0))
#define DUM_PF_EMPTY_GET (((*DUM_STAT)&BIT(4))>>4)
#define DUM_SF_EMPTY_GET (((*DUM_STAT)&BIT(3))>>3)
#define DUM_MUX_GET (((*DUM_STAT)&BIT(7))>>7)
#define DUM_PF_LEVEL_GET (((*DUM_STAT)>>24)&0x7F)
#define DUM_SF_LEVEL_GET (((*DUM_STAT)>>12)&0x7)
#define DUM_CURRENT_BUSY_GET ((((*DUM_STAT)&(BIT(22)+BIT(21)+BIT(20)+BIT(19)+BIT(18)+BIT(17)+BIT(16)))^(BIT(22)))>>16)
#define DUM_BUSY_GET (((*DUM_STAT)>>22)&BIT(0))
#define DUM_INT_BITS_GET ((*DUM_STAT)&(BIT(23)+BIT(7)+BIT(6)+BIT(5)+BIT(4)+BIT(3)+BIT(0)))
#define DUM_STAT_ARMULATOR (*DUM_STAT=0x00000018)

#define DUM_DECODE_WR(val) (*DUM_DECODE = (val))
#define DUM_DECODE_RD ((*DUM_DECODE)&0x1FF00000)
#define DUM_COM_BASE_WR(val) (*DUM_COM_BASE = (val))
#define DUM_COM_BASE_RD ((*DUM_COM_BASE)&0xFFFFFFFC)
#define DUM_SYNC_C_WR(val) ((*DUM_SYNC_C)=(val))
#define DUM_SYNC_C_RD ((*DUM_SYNC_C)&0x1FF)
#define DUM_CLK_DIV_WR(val) (*DUM_CLK_DIV = (val))
#define DUM_SYNC_MODE_WR(sync_output, sync_restart_val)\
   (*DUM_SYNC_MODE = (((sync_output)<<31) + ((sync_restart_val)<<0)))
#define DUM_SYNC_OUT_C_WR(set_sync_high, set_sync_low)\
   (*DUM_SYNC_OUT_C = (((set_sync_high)<<16) + ((set_sync_low)<<0)))
#define DUM_FORMAT_MODE_RD (((*DUM_FORMAT)&BIT(3))>>3)
#define DUM_OUTP_FORMAT1_WR(val) (*DUM_OUTP_FORMAT1 = (val))
#define DUM_OUTP_FORMAT2_WR(val) (*DUM_OUTP_FORMAT2 = (val))

#define DUM_WTCFG1_WR(val) (*DUM_WTCFG1 = (val))
#define DUM_WTCFG2_WR(val) (*DUM_WTCFG2 = (val))
#define DUM_RTCFG1_WR(val) (*DUM_RTCFG1 = (val))
#define DUM_RTCFG2_WR(val) (*DUM_RTCFG2 = (val))
#define DUM_TCFG_WR(val)   (*DUM_TCFG   = (val))
#define DUM_WTCFG1_WR_S(wa, whl, wh, wsf, ws, we)\
   (*DUM_WTCFG1 = (((wa)<<20) + ((whl)<<16) + ((wh)<<12) + ((wsf)<<8) + ((ws)<<4) + ((we)<<0)))
#define DUM_WTCFG2_WR_S(wa, whl, wh, wsf, ws, we)\
   (*DUM_WTCFG2 = (((wa)<<20) + ((whl)<<16) + ((wh)<<12) + ((wsf)<<8) + ((ws)<<4) + ((we)<<0)))
#define DUM_RTCFG1_WR_S(ra, rh, rs) (*DUM_RTCFG1 = (((ra)<<8) + ((rh)<<4) + ((rs)<<0)))
#define DUM_RTCFG2_WR_S(ra, rh, rs) (*DUM_RTCFG2 =(((ra)<<8) + ((rh)<<4) + ((rs)<<0)))
#define DUM_TCFG_WR(val) (*DUM_TCFG = (val))
#define DUM_SLAVE_WRITE_WRHW(val) (*DUM_SLAVE_WRITE_HW = ((uint16_t)(val)))
#define DUM_SLAVE_WRITE_WRW(val) (*DUM_SLAVE_WRITE_W = (val))
#define DUM_DISP_WR(disp_no, a0, val)\
   (DUM_SLAVE_WRITE_WRHW((((disp_no)&BIT(0))<<10) | (((a0)&BIT(0))<<9) | (((val)&0x1FF)<<0)))
#define DUM_DISP_I_WR(disp_no, val)\
   (DUM_SLAVE_WRITE_WRHW((((disp_no)&BIT(0))<<10) | 0 | (((val)&0x1FF)<<0)))
#define DUM_DISP_R_WR(disp_no, val)\
   (DUM_SLAVE_WRITE_WRHW((((disp_no)&BIT(0))<<10) | BIT(9) | (((val)&0x1FF)<<0)))
#define DUM_DISP_REG_WR(disp_no, reg, val)\
   (DUM_SLAVE_WRITE_WRW((((disp_no)&BIT(0))<<26) | BIT(25) | (((val)&0x1FF)<<16) | (((disp_no)&BIT(0))<<10) | (((reg)&0x1FF)<<0)))
#define DUM_DISP1I_RD (*DUM_SLAVE_READ1_I)
#define DUM_DISP1R_RD ((*DUM_SLAVE_READ1_R)&0xFF)
#define DUM_DISP1P_RD ((*DUM_SLAVE_READ1_R)&0x1FF)
#define DUM_DISP2I_RD (*DUM_SLAVE_READ2_I)
#define DUM_DISP2R_RD ((*DUM_SLAVE_READ2_R)&0xFF)
#define DUM_DISP2P_RD ((*DUM_SLAVE_READ2_R)&0x1FF)

/* register values */
#define V_BAC_ENABLE		(BIT(0))
#define V_BAC_DISABLE_IDLE	(BIT(1))
#define V_BAC_DISABLE_TRIG	(BIT(2))
#define V_DUM_RESET		(BIT(3))
#define V_MUX_RESET		(BIT(4))
#define BAC_ENABLED		(BIT(0))
#define BAC_DISABLED		0

/* Sony LCD commands */
#define V_LCD_STANDBY_OFF	((BIT(25)) | (0 << 16) | DISP_0_REG)
#define V_LCD_USE_9BIT_BUS	((BIT(25)) | (2 << 16) | DISP_1_REG)
#define V_LCD_SYNC_RISE_L	((BIT(25)) | (0 << 16) | DISP_SYNC_RISE_L_REG)
#define V_LCD_SYNC_RISE_H	((BIT(25)) | (0 << 16) | DISP_SYNC_RISE_H_REG)
#define V_LCD_SYNC_FALL_L	((BIT(25)) | (160 << 16) | DISP_SYNC_FALL_L_REG)
#define V_LCD_SYNC_FALL_H	((BIT(25)) | (0 << 16) | DISP_SYNC_FALL_H_REG)
#define V_LCD_SYNC_ENABLE	((BIT(25)) | (128 << 16) | DISP_SYNC_EN_REG)
#define V_LCD_DISPLAY_ON	((BIT(25)) | (64 << 16) | DISP_0_REG)

enum {
	PAD_NONE,
	PAD_512,
	PAD_1024
};

enum {
	RGB888,
	RGB666,
	RGB565,
	BGR565,
	ARGB1555,
	ABGR1555,
	ARGB4444,
	ABGR4444
};

typedef struct {
	int sync_neg_edge;
	int round_robin;
	int mux_int;
	int synced_dirty_flag_int;
	int dirty_flag_int;
	int error_int;
	int pf_empty_int;
	int sf_empty_int;
	int bac_dis_int;
	u32 dirty_base_adr;
	u32 command_base_adr;
	u32 sync_clk_div;
	int sync_output;
	u32 sync_restart_val;
	u32 set_sync_high;
	u32 set_sync_low;
} dum_setup_t;

typedef struct {
	int disp_no;
	u32 xmin;
	u32 ymin;
	u32 xmax;
	u32 ymax;
	int xmirror;
	int ymirror;
	int rotate;
	u32 minadr;
	u32 maxadr;
	u32 dirtybuffer;
	int pad;
	int format;
	int hwdirty;
	int slave_trans;
} dum_ch_setup_t;

typedef struct {
	u32 xmin_l;
	u32 xmin_h;
	u32 ymin;
	u32 xmax_l;
	u32 xmax_h;
	u32 ymax;
} disp_window_t;

/* Configures the dum */
/* Called from dum_init() */
void dum_setup(dum_setup_t * setup);

/* Change window sizes*/
u32 dum_change_window_setup(u32 ch_no, dum_ch_setup_t * ch_setup);

/* Switches off sync for a dum channel */
void dum_sync_off(u32 ch_no);

/* Sets a channel dirty */
void dum_set_dirty(u32 ch_no);

/* Sets dirty encode address */
void dum_set_dirty_addr(u32 ch_no, u32 addr);

/* Returns 1 if there are pending memory accesses */
int dum_ch_working(u32 ch_no);

/* Deallocates a dum channel */
void dum_ch_free(u32 ch_no);

/* Returns 1 if BAC is enabled or there are data in pixel fifo */
int dum_working(void);

/* Draws a square of size <size> and colour<XXRRGGBB> on display <disp_no>
   Upper left corner is put at <x, y>
   Pixels outside the display to the right and bottom are cut. */
void dum_draw_square(int disp_no, u32 x, u32 y, u32 size, u32 XXRRGGBB);

/* Writes to a display register */
void dum_disp_regwrite(int disp_no, u32 reg, u32 val);

/* Reads a display register */
u32 dum_disp_regread(int disp_no, u32 reg);

/* Polls until pixel fifo is empty */
void dum_pfempty_poll(void);

/* Polls until slave fifo is empty */
void dum_sfempty_poll(void);

/* Sets the update window in the display */
void dum_set_uw(int disp_no, u32 xmin, u32 ymin, u32 xmax, u32 ymax);

/* Resets display pointer to xmin, ymin */
void dum_reset_uw(int disp_no);

void dum_setup_chk_input(dum_setup_t * setup);
void dum_ch_setup_chk_input(dum_ch_setup_t * ch_setup);
u32 nof_bytes(dum_ch_setup_t * ch_setup);
void build_disp_window(dum_ch_setup_t * ch_setup, disp_window_t * dw);
u32 build_command(int disp_no, u32 reg, u32 val);
u32 build_double_index(int disp_no, u32 val);
u32 sync_place(dum_ch_setup_t * ch_setup);

#endif				/* #ifndef __PNX008_DUM_H__ */
