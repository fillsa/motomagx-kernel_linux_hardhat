/*
 * Copyright © 2004,2006-2008 Motorola, All Rights Reserved.
 *
 * This program is licensed under a BSD license with the following terms:
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this list of conditions
 *   and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice, this list of
 *   conditions and the following disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 * - Neither the name of Motorola nor the names of its contributors may be used to endorse or
 *   promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.  
 *
 * Date         Author    Comment
 * ----------   --------  ---------------------------
 * 06/28/2004   Motorola  File creation 
 * 12/11/2006   Motorola  Fixed bug, audio loss after wakeup from DSM
 * 05/14/2007   Motorola  Support querying data left(not played) in apal driver from user space
 * 02/28/2008   Motorola  Supporting FM radio
 */

/*!
 * @file apal_driver.h
 *
 * @ingroup audio
 *
 * @brief This is the header of the apal driver. 
 *
 */
  
#ifdef __cplusplus
extern "C" {
#endif

#ifndef APAL_DRIVER_H
#define APAL_DRIVER_H

/*==================================================================================================
                                               
GENERAL DESCRIPTION: This file contains the APAL (Audio Platform Abstraction Layer) driver defs

===================================================================================================
                                           INCLUDE FILES
==================================================================================================*/

/*==================================================================================================
                                             CONSTANTS
==================================================================================================*/
#ifndef BOOL
#define BOOL unsigned char
#endif

#ifndef TRUE
#define TRUE (BOOL)1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/****start IOCTL list ****/

#define  APAL_IOCTL_SET_AUDIO_ROUTE            1
#define  APAL_IOCTL_CLEAR_AUDIO_ROUTE          2
#define  APAL_IOCTL_AUDIO_POWER_OFF            3
#define  APAL_IOCTL_SET_DAI                    4
#define  APAL_IOCTL_STOP                       5
#define  APAL_IOCTL_PAUSE                      6
#define  APAL_IOCTL_STOP_SDMA                  7
#define  APAL_GET_LEFT_WRITTEN_BYTES_IN_KERNEL 8

/****end IOCTL list ****/

#if defined(CONFIG_ARCH_MXC91231)
#define ACR  0x50048044  /* AP CKO Register */
#define CSCR 0x5004400C  /* CRM (Clock and Reset Module) System Control Register */
#elif defined(CONFIG_ARCH_MXC91331)
#define COSR 0x53F80018  /* Argon+ COSR register for setting clock */
#endif

/* Register 36 AUDIOIC_REG_AUD_RX_0 */
#define AUDIOIC_NO_WRITE_MASK_36   0x00000000
#define AUDIOIC_SET_VAUD_ON        0x00000001
#define AUDIOIC_SET_AUD_BIAS_EN    0x00000002
#define AUDIOIC_SET_AUD_BIAS_SPD   0x00000004
#define AUDIOIC_SET_ASP_EN         0x00000008
#define AUDIOIC_SET_ASP_SEL        0x00000010
#define AUDIOIC_SET_ALSP_EN        0x00000020
#define AUDIOIC_SET_ALSP_REF       0x00000040
#define AUDIOIC_SET_ALSP_SEL       0x00000080
#define AUDIOIC_SET_LSPL_EN        0x00000100
#define AUDIOIC_SET_AHSR_EN        0x00000200
#define AUDIOIC_SET_AHSL_EN        0x00000400
#define AUDIOIC_SET_AHS_SEL        0x00000800
#define AUDIOIC_SET_HS_PG_DSBL     0x00001000
#define AUDIOIC_SET_HS_DET_EN      0x00002000
#define AUDIOIC_SET_HS_DET_AUTO_B  0x00004000
#define AUDIOIC_SET_OUT_RXR_EN     0x00008000
#define AUDIOIC_SET_OUT_RXL_EN     0x00010000
#define AUDIOIC_SET_OUT_RX_SEL     0x00020000
#define AUDIOIC_SET_OUT_CDC_EN     0x00040000
#define AUDIOIC_SET_HSL_DET_EN     0x00080000
#define AUDIOIC_SET_REG36_RSVD_0   0x00100000
#define AUDIOIC_SET_ADD_CDC        0x00200000
#define AUDIOIC_SET_ADD_ST_DAC     0x00400000
#define AUDIOIC_SET_ADD_RX_IN      0x00800000

/* Register 37 AUDIOIC_REG_AUD_RX_1 */
#define AUDIOIC_NO_WRITE_MASK_37   0x00C00000
#define AUDIOIC_SET_PGA_RX_EN      0x00000001
#define AUDIOIC_SET_PGA_RX_0       0x00000002
#define AUDIOIC_SET_PGA_RX_1       0x00000004
#define AUDIOIC_SET_PGA_RX_2       0x00000008
#define AUDIOIC_SET_PGA_RX_3       0x00000010
#define AUDIOIC_SET_PGA_ST_EN      0x00000020
#define AUDIOIC_SET_PGA_ST_0       0x00000040
#define AUDIOIC_SET_PGA_ST_1       0x00000080
#define AUDIOIC_SET_PGA_ST_2       0x00000100
#define AUDIOIC_SET_PGA_ST_3       0x00000200
#define AUDIOIC_SET_ARX_IN_EN      0x00000400
#define AUDIOIC_SET_ARX_IN         0x00000800
#define AUDIOIC_SET_PGA_RX_IN_0    0x00001000
#define AUDIOIC_SET_PGA_RX_IN_1    0x00002000
#define AUDIOIC_SET_PGA_RX_IN_2    0x00004000
#define AUDIOIC_SET_PGA_RX_IN_3    0x00008000
#define AUDIOIC_SET_MONO_0         0x00010000
#define AUDIOIC_SET_MONO_1         0x00020000
#define AUDIOIC_SET_BAL_CNTRL_0    0x00040000
#define AUDIOIC_SET_BAL_CNTRL_1    0x00080000
#define AUDIOIC_SET_BAL_CNTRL_2    0x00100000
#define AUDIOIC_SET_BAL_LFT_RGT    0x00200000

/* Register 38 AUDIOIC_REG_AUD_TX */
#define AUDIOIC_NO_WRITE_MASK_38   0x00000000
#define AUDIOIC_SET_MC1_BS_EN      0x00000001
#define AUDIOIC_SET_MC2_BS_EN      0x00000002
#define AUDIOIC_SET_MC2_BS_DBNC    0x00000004
#define AUDIOIC_SET_MC2_BS_DET_EN  0x00000008
#define AUDIOIC_SET_REG38_RESV_0   0x00000010
#define AUDIOIC_SET_AMC1R_EN       0x00000020
#define AUDIOIC_SET_AMC1R_I_TO_V   0x00000040
#define AUDIOIC_SET_AMC1L_EN       0x00000080
#define AUDIOIC_SET_AMC1L_I_TO_V   0x00000100
#define AUDIOIC_SET_AMC2_EN        0x00000200
#define AUDIOIC_SET_AMC2_I_TO_V    0x00000400
#define AUDIOIC_SET_ATXIN_EN       0x00000800
#define AUDIOIC_SET_ATXOUT_EN      0x00001000
#define AUDIOIC_SET_REG38_RSVD_1   0x00002000
#define AUDIOIC_SET_PGA_TX_R0      0x00004000
#define AUDIOIC_SET_PGA_TX_R1      0x00008000
#define AUDIOIC_SET_PGA_TX_R2      0x00010000
#define AUDIOIC_SET_PGA_TX_R3      0x00020000
#define AUDIOIC_SET_PGA_TX_R4      0x00040000
#define AUDIOIC_SET_PGA_TX_L0      0x00080000
#define AUDIOIC_SET_PGA_TX_L1      0x00100000
#define AUDIOIC_SET_PGA_TX_L2      0x00200000
#define AUDIOIC_SET_PGA_TX_L3      0x00400000
#define AUDIOIC_SET_PGA_TX_L4      0x00800000

/* Register 39 AUDIOIC_REG_SSI_NTWRK */
#define AUDIOIC_NO_WRITE_MASK_39       0x00C00000
#define AUDIOIC_SET_REG39_RSVD_0       0x00000001
#define AUDIOIC_SET_REG39_RSVD_1       0x00000002
#define AUDIOIC_SET_CDC_TXRX_SLT_0     0x00000004
#define AUDIOIC_SET_CDC_TXRX_SLT_1     0x00000008
#define AUDIOIC_SET_CDC_TX_SEC_SLT_0   0x00000010
#define AUDIOIC_SET_CDC_TX_SEC_SLT_1   0x00000020
#define AUDIOIC_SET_CDC_RX_SEC_SLT_0   0x00000040
#define AUDIOIC_SET_CDC_RX_SEC_SLT_1   0x00000080
#define AUDIOIC_SET_CDC_RX_SEC_GAN_0   0x00000100
#define AUDIOIC_SET_CDC_RX_SEC_GAN_1   0x00000200
#define AUDIOIC_SET_CDC_SUM_GAN        0x00000400
#define AUDIOIC_SET_REG_39_RSVD        0x00000800
#define AUDIOIC_SET_ST_DAC_SLT_0       0x00001000
#define AUDIOIC_SET_ST_DAC_SLT_1       0x00002000
#define AUDIOIC_SET_ST_DAC_RX_SLT_0    0x00004000
#define AUDIOIC_SET_ST_DAC_RX_SLT_1    0x00008000
#define AUDIOIC_SET_ST_DAC_RX_SSLT_0   0x00010000
#define AUDIOIC_SET_ST_DAC_RX_SSLT_1   0x00020000
#define AUDIOIC_SET_ST_DAC_RX_SGAN_0   0x00040000
#define AUDIOIC_SET_ST_DAC_RX_SGAN_1   0x00080000
#define AUDIOIC_SET_ST_DAC_SUM_GAN     0x00100000

/* Register 40 AUDIOIC_REG_AUD_CODEC */
#define AUDIOIC_NO_WRITE_MASK_40     0x00008000
#define AUDIOIC_SET_CDC_SSI_SEL      0x00000001
#define AUDIOIC_SET_CDC_CLK_SEL      0x00000002
#define AUDIOIC_SET_CDC_SM           0x00000004
#define AUDIOIC_SET_CDC_BCLK_INV     0x00000008
#define AUDIOIC_SET_CDC_FS_INV       0x00000010
#define AUDIOIC_SET_CDC_FS0          0x00000020
#define AUDIOIC_SET_CDC_FS1          0x00000040
#define AUDIOIC_SET_CDC_CLK0         0x00000080
#define AUDIOIC_SET_CDC_CLK1         0x00000100
#define AUDIOIC_SET_CDC_CLK2         0x00000200
#define AUDIOIC_SET_CDC_FS_8K16K     0x00000400
#define AUDIOIC_SET_CDC_EN           0x00000800
#define AUDIOIC_SET_CDC_CLK_EN       0x00001000
#define AUDIOIC_SET_CDC_TS           0x00002000
#define AUDIOIC_SET_CDC_DITH         0x00004000
#define AUDIOIC_SET_CDC_RESET        0x00008000
#define AUDIOIC_SET_CDC_BYP          0x00010000
#define AUDIOIC_SET_CDC_ALM          0x00020000
#define AUDIOIC_SET_CDC_DLM          0x00040000
#define AUDIOIC_SET_AUD_TX_HP_FLTR   0x00080000
#define AUDIOIC_SET_AUD_RX_HP_FLTR   0x00100000

/* Register 41 AUDIOIC_REG_AUD_ST_DAC */
#define AUDIOIC_NO_WRITE_MASK_41     0x00008000
#define AUDIOIC_SET_STDAC_SSI_SEL    0x00000001
#define AUDIOIC_SET_STDAC_CLK_SEL    0x00000002
#define AUDIOIC_SET_STDAC_SM         0x00000004
#define AUDIOIC_SET_STDAC_BCLK_INV   0x00000008
#define AUDIOIC_SET_STDAC_FS_INV     0x00000010
#define AUDIOIC_SET_STDAC_FS0        0x00000020
#define AUDIOIC_SET_STDAC_FS1        0x00000040
#define AUDIOIC_SET_STDAC_CLK0       0x00000080
#define AUDIOIC_SET_STDAC_CLK1       0x00000100
#define AUDIOIC_SET_STDAC_CLK2       0x00000200
#define AUDIOIC_SET_REG41_RSVD_0     0x00000400
#define AUDIOIC_SET_STDAC_EN         0x00000800
#define AUDIOIC_SET_STDAC_CLK_EN     0x00001000
#define AUDIOIC_SET_REG41_RSVD_1     0x00002000
#define AUDIOIC_SET_REG41_RSVD_2     0x00004000
#define AUDIOIC_SET_STDAC_RESET      0x00008000
#define AUDIOIC_SET_SP_DIF           0x00010000
#define AUDIOIC_SET_SR0              0x00020000
#define AUDIOIC_SET_SR1              0x00040000
#define AUDIOIC_SET_SR2              0x00080000
#define AUDIOIC_SET_SR3              0x00100000

/*==================================================================================================
                                               ENUMS
==================================================================================================*/
typedef enum
{
    APAL_AUDIO_ROUTE_NONE            = 0,
    APAL_AUDIO_ROUTE_PHONE           = 1,
    APAL_AUDIO_ROUTE_CODEC_AP        = 2,
    APAL_AUDIO_ROUTE_STDAC           = 3,
    APAL_AUDIO_ROUTE_FMRADIO         = 4,
}APAL_AUDIO_ROUTE_ENUM_T;

/*==================================================================================================
                                     STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct
{
    unsigned char *pbuf;
    int buf_size;
} APAL_BUFFER_STRUCT_T;

/*==================================================================================================
                                               MACROS
==================================================================================================*/

/*==================================================================================================
                                    GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                        FUNCTION PROTOTYPES
==================================================================================================*/
  
#endif /* APAL_DRIVER_H */

#ifdef __cplusplus                             
}
#endif
