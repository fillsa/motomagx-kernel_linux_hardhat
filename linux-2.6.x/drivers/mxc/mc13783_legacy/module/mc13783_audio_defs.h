/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 */

#ifndef         __MC13783_AUDIO_DEFS_H__
#define         __MC13783_AUDIO_DEFS_H__

#define         DEF_AUDIO_RX_0  0x001000
#define         DEF_AUDIO_RX_1  0x00D35A
#define         DEF_AUDIO_TX    0x420000
#define         DEF_AUDIO_SSI   0x013060
#define         DEF_AUDIO_CODEC 0x180027
#define         DEF_AUDIO_STDAC 0x0E0004

/*
 * Audio Rx0
 */

#define         BIT_VAUDIOFORCE  0
#define         BIT_BIASED       1
#define         BIT_BIASSPEED    2
#define         BIT_ASPEN        3
#define         BIT_ASPSEL       4
#define         BIT_ALSPEN       5
#define         BIT_ALSPREF      6
#define         BIT_ALSPSEL      7
#define         BIT_AHSREN       9
#define         BIT_AHSLEN      10
#define         BIT_AHSSEL      11
#define         BIT_HSPGDIS     12
#define         BIT_HSDETEN     13
#define         BIT_HSDETAUTOB  14
#define         BIT_ARXREN      15
#define         BIT_ARXLEN      16
#define         BIT_ARXSEL      17
#define         BIT_CDCOUTEN    18
#define         BIT_HSLDETEN    19

#define         BIT_ADDCDC      21
#define         BIT_ADDSTDC     22
#define         BIT_ADDRXIN     23

/*
 * Audio Rx1
 */

#define         BIT_AMPCDC       0
#define         BIT_CODEC_VOLUME 1
#define         BIT_AMPSTDC      5
#define         BIT_STDAC_VOLUME 6
#define         BIT_AMPRXIN     10
#define         BIT_LINE_VOLUME 12
#define         BITS_ADDER_CONF 16
#define         BITS_BALANCE    18
#define         BIT_L_R_BALANCE 21

/*
 * Audio Tx1
 */

#define         BIT_TX_MC1BEN    0
#define         BIT_TX_MC2BEN    1
#define         BIT_TX_HANDSET1  5
#define         BIT_TX_HANDSET2  7
#define         BIT_TX_HEADSET   9
#define         BIT_TX_LINE_IN  11
#define         BITS_TX_GAIN_R  14
#define         BITS_TX_GAIN_L  19

/*
 * ST DAC and CODEC bits
 */
#define         BIT_SSISEL       0
#define         BIT_CLKSEL       1
#define         BIT_SM           2
#define         BIT_BCLINV       3
#define         BIT_FSINV        4
#define         BIT_BUSPROTOCOL  5
#define         BITS_CONF_CLK    7
#define         BIT_EN          11
#define         BIT_CLK_EN      12
#define         BIT_RESET       15

/*
 * ST DAC
 */
#define         BITS_CONF_SR    17
#define         BIT_SPDIF       16

/*
 * CODEC
 */
#define		BIT_FS8K16K	10
#define         BIT_CODEC_DITH  14
#define         BIT_CODEC_THPF  19
#define         BIT_CODEC_RHPF  20

/*
 * SSI Network
 */
#define         BITS_CODEC_TXRXSLOTS0   2
#define         BITS_CODEC_TXSECSLOTS0  4
#define         BITS_CODEC_RXSECSLOTS0  6
#define         BITS_CODEC_RXSECGAIN    8
#define         BITS_CODEC_SUM          10
#define         BIT_LATCHING_EDGE       11

#define         BITS_SLOTS0             12
#define         BITS_RXSLOTS0           14
#define         BITS_RXSECSLOTS0        16
#define         BITS_RXSECGAIN          18
#define         BITS_STDAC_SUM          20

#define         NB_AUDIO_REG    6

#endif				/*  __MC13783_AUDIO_DEFS_H__ */
