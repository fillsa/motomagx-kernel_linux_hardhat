/*
 * Copyright © 2006, Motorola, All Rights Reserved.
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
 * Motorola 2006-Oct-11 - Initial Creation
 */
 
/*!
 * @file power_ic_audio.h
 *
 * @brief Global mask definitions for the audio interface.
 *
 * @ingroup poweric_audio
 */

#ifndef __POWER_IC_AUDIO_H__
#define __POWER_IC_AUDIO_H__

/*==================================================================================================
                                         INCLUDE FILES
==================================================================================================*/

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*!
 * @name Audio Rx 0 Register Masks
 *
 * @brief Used to build the mask and value parameters for the
 * power_ic_audio_set_reg_mask_audio_rx_0 function to select what bits to change and what bits to
 * enable.
 */
/*@{*/
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_VAUDIOON              0x00000001
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_BIASEN                0x00000002
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_BIASSPEED             0x00000004
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_ASPEN                 0x00000008
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_ASPSEL                0x00000010
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_ALSPEN                0x00000020
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_ALSPREF               0x00000040
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_ALSPSEL               0x00000080
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_LSPLEN                0x00000100
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_AHSREN                0x00000200
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_AHSLEN                0x00000400
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_AHSSEL                0x00000800
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_HSPGDIS               0x00001000
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_HSDETEN               0x00002000
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_HSDETAUTOB            0x00004000
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_ARXOUTREN             0x00008000
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_ARXOUTLEN             0x00010000
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_ARXOUTSEL             0x00020000
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_CDCOUTEN              0x00040000
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_HSLDETEN              0x00080000
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_BIT20_RESERVED        0x00100000
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_ADDCDC                0x00200000
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_ADDSTDC               0x00400000
#define POWER_IC_AUDIO_REG_AUDIO_RX_0_ADDRXIN               0x00800000
/*@}*/

/*!
 * @name Audio Rx 1 Register Masks
 *
 * @brief Used to build the mask and value parameters for the
 * power_ic_audio_set_reg_mask_audio_rx_1 function to select what bits to change and what bits to
 * enable.
 */
/*@{*/
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGARXEN               0x00000001
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGARX0                0x00000002
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGARX1                0x00000004
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGARX2                0x00000008
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGARX3                0x00000010
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGASTEN               0x00000020
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGAST0                0x00000040
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGAST1                0x00000080
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGAST2                0x00000100
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGAST3                0x00000200
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_ARXINEN               0x00000400
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_ARXIN                 0x00000800
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGARXIN0              0x00001000
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGARXIN1              0x00002000
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGARXIN2              0x00004000
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_PGARXIN3              0x00008000
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_MONO0                 0x00010000
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_MONO1                 0x00020000
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_BAL0                  0x00040000
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_BAL1                  0x00080000
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_BAL2                  0x00100000
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_BALLR                 0x00200000
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_BIT22_UNUSED          0x00400000
#define POWER_IC_AUDIO_REG_AUDIO_RX_1_BIT23_UNUSED          0x00800000
/*@}*/

/*!
 * @name Audio TX Register Masks
 *
 * @brief Used to build the mask and value parameters for the
 * power_ic_audio_set_reg_mask_audio_tx function to select what bits to change and what bits to
 * enable.
 */
/*@{*/
#define POWER_IC_AUDIO_REG_AUDIO_TX_MC1BEN                  0x00000001
#define POWER_IC_AUDIO_REG_AUDIO_TX_MC2BEN                  0x00000002
#define POWER_IC_AUDIO_REG_AUDIO_TX_MC2BDETDBNC             0x00000004
#define POWER_IC_AUDIO_REG_AUDIO_TX_MC2BDETEN               0x00000008
#define POWER_IC_AUDIO_REG_AUDIO_TX_BIT4_RESERVED           0x00000010
#define POWER_IC_AUDIO_REG_AUDIO_TX_AMC1REN                 0x00000020
#define POWER_IC_AUDIO_REG_AUDIO_TX_AMC1RITOV               0x00000040
#define POWER_IC_AUDIO_REG_AUDIO_TX_AMC1LEN                 0x00000080
#define POWER_IC_AUDIO_REG_AUDIO_TX_AMC1LITOV               0x00000100
#define POWER_IC_AUDIO_REG_AUDIO_TX_AMC2EN                  0x00000200
#define POWER_IC_AUDIO_REG_AUDIO_TX_AMC2ITOV                0x00000400
#define POWER_IC_AUDIO_REG_AUDIO_TX_ATXINEN                 0x00000800
#define POWER_IC_AUDIO_REG_AUDIO_TX_ATXOUTEN                0x00001000
#define POWER_IC_AUDIO_REG_AUDIO_TX_RXINREC                 0x00002000
#define POWER_IC_AUDIO_REG_AUDIO_TX_PGATXR0                 0x00004000
#define POWER_IC_AUDIO_REG_AUDIO_TX_PGATXR1                 0x00008000
#define POWER_IC_AUDIO_REG_AUDIO_TX_PGATXR2                 0x00010000
#define POWER_IC_AUDIO_REG_AUDIO_TX_PGATXR3                 0x00020000
#define POWER_IC_AUDIO_REG_AUDIO_TX_PGATXR4                 0x00040000
#define POWER_IC_AUDIO_REG_AUDIO_TX_PGATXL0                 0x00080000
#define POWER_IC_AUDIO_REG_AUDIO_TX_PGATXL1                 0x00100000
#define POWER_IC_AUDIO_REG_AUDIO_TX_PGATXL2                 0x00200000
#define POWER_IC_AUDIO_REG_AUDIO_TX_PGATXL3                 0x00400000
#define POWER_IC_AUDIO_REG_AUDIO_TX_PGATXL4                 0x00800000
/*@}*/

/*!
 * @name SSI Network Register Masks
 *
 * @brief Used to build the mask and value parameters for the
 * power_ic_audio_set_reg_mask_ssi_network function to select what bits to change and what bits to
 * enable.
 */
/*@{*/
#define POWER_IC_AUDIO_REG_SSI_NETWORK_BIT0_RESERVED        0x00000001
#define POWER_IC_AUDIO_REG_SSI_NETWORK_BIT1_RESERVED        0x00000002
#define POWER_IC_AUDIO_REG_SSI_NETWORK_CDCTXRXSLOT0         0x00000004
#define POWER_IC_AUDIO_REG_SSI_NETWORK_CDCTXRXSLOT1         0x00000008
#define POWER_IC_AUDIO_REG_SSI_NETWORK_CDCTXSECSLOT0        0x00000010
#define POWER_IC_AUDIO_REG_SSI_NETWORK_CDCTXSECSLOT1        0x00000020
#define POWER_IC_AUDIO_REG_SSI_NETWORK_CDCRXSECSLOT0        0x00000040
#define POWER_IC_AUDIO_REG_SSI_NETWORK_CDCRXSECSLOT1        0x00000080
#define POWER_IC_AUDIO_REG_SSI_NETWORK_CDCRXSECGAIN0        0x00000100
#define POWER_IC_AUDIO_REG_SSI_NETWORK_CDCRXSECGAIN1        0x00000200
#define POWER_IC_AUDIO_REG_SSI_NETWORK_CDCSUMGAIN           0x00000400
#define POWER_IC_AUDIO_REG_SSI_NETWORK_CDCFSDLY             0x00000800
#define POWER_IC_AUDIO_REG_SSI_NETWORK_STDCSLOTS0           0x00001000
#define POWER_IC_AUDIO_REG_SSI_NETWORK_STDCSLOTS1           0x00002000
#define POWER_IC_AUDIO_REG_SSI_NETWORK_STDCRXSLOT0          0x00004000
#define POWER_IC_AUDIO_REG_SSI_NETWORK_STDCRXSLOT1          0x00008000
#define POWER_IC_AUDIO_REG_SSI_NETWORK_STDCRXSECSLOT0       0x00010000
#define POWER_IC_AUDIO_REG_SSI_NETWORK_STDCRXSECSLOT1       0x00020000
#define POWER_IC_AUDIO_REG_SSI_NETWORK_STDCRXSECGAIN0       0x00040000
#define POWER_IC_AUDIO_REG_SSI_NETWORK_STDCRXSECGAIN1       0x00080000
#define POWER_IC_AUDIO_REG_SSI_NETWORK_STDCSUMGAIN          0x00100000
#define POWER_IC_AUDIO_REG_SSI_NETWORK_BIT21_RESERVED       0x00200000
#define POWER_IC_AUDIO_REG_SSI_NETWORK_BIT22_UNUSED         0x00400000
#define POWER_IC_AUDIO_REG_SSI_NETWORK_BIT23_UNUSED         0x00800000
/*@}*/

/*!
 * @name Audio Codec Register Masks
 *
 * @brief Used to build the mask and value parameters for the
 * power_ic_audio_set_reg_mask_audio_codec function to select what bits to change and what bits to
 * enable.
 */
/*@{*/
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCSSISEL            0x00000001
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCCLKSEL            0x00000002
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCSM                0x00000004
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCBCLINV            0x00000008
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCFSINV             0x00000010
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCFS0               0x00000020
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCFS1               0x00000040
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCCLK0              0x00000080
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCCLK1              0x00000100
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCCLK2              0x00000200
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCFS8K16K           0x00000400
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCEN                0x00000800
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCCLKEN             0x00001000
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCTS                0x00002000
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCDITH              0x00004000
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCRESET             0x00008000
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCBYP               0x00010000
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCALM               0x00020000
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_CDCDLM               0x00040000
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_AUDIHPF              0x00080000
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_AUDOHPF              0x00100000
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_BIT21_UNUSED         0x00200000
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_BIT22_UNUSED         0x00400000
#define POWER_IC_AUDIO_REG_AUDIO_CODEC_BIT23_UNUSED         0x00800000
/*@}*/

/*!
 * @name Audio Stereo DAC Register Masks
 *
 * @brief Used to build the mask and value parameters for the
 * power_ic_audio_set_reg_mask_audio_stereo_dac function to select what bits to change and what bits
 * to enable.
 */
/*@{*/
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCSSISEL      0x00000001
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCCLKSEL      0x00000002
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCSM          0x00000004
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCBCLINV      0x00000008
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCFSINV       0x00000010
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCFS0         0x00000020
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCFS1         0x00000040
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCCLK0        0x00000080
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCCLK1        0x00000100
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCCLK2        0x00000200
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCFSDLYB      0x00000400
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCEN          0x00000800
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCCLKEN       0x00001000
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_BIT13_RESERVED  0x00002000
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_BIT14_RESERVED  0x00004000
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_STDCRESET       0x00008000
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_SPDIF           0x00010000
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_SR0             0x00020000
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_SR1             0x00040000
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_SR2             0x00080000
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_SR3             0x00100000
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_BIT21_UNUSED    0x00200000
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_BIT22_UNUSED    0x00400000
#define POWER_IC_AUDIO_REG_AUDIO_STEREO_DAC_BIT23_UNUSED    0x00800000
/*@}*/

/*==================================================================================================
                                            MACROS
==================================================================================================*/

/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATION
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

#endif /* __POWER_IC_AUDIO_H__ */
