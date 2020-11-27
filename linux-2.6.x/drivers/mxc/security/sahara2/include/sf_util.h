/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU Lesser General 
 * Public License.  You may obtain a copy of the GNU Lesser General 
 * Public License Version 2.1 or later at the following locations:
 *
 * http://www.opensource.org/licenses/lgpl-license.html
 * http://www.gnu.org/copyleft/lgpl.html
 */
/*! 
* @file sf_util.h
*
* @brief Header for internal Descriptor-chain building Functions
*
* @ingroup MXCSAHARA2
*/
#ifndef SF_UTIL_H
#define SF_UTIL_H

#include <sahara.h>

/*! Header value for Sahara Descriptor  1 */
#define SAH_HDR_SKHA_SET_MODE_IV_KEY  0x10880000
/*! Header value for Sahara Descriptor  2 */
#define SAH_HDR_SKHA_SET_MODE_ENC_DEC 0x108d0000
/*! Header value for Sahara Descriptor  4 */
#define SAH_HDR_SKHA_ENC_DEC          0x90850000
/*! Header value for Sahara Descriptor  5 */
#define SAH_HDR_SKHA_READ_CONTEXT_IV  0x10820000
/*! Header value for Sahara Descriptor  6 */
#define SAH_HDR_MDHA_SET_MODE_MD_KEY  0x20880000
/*! Header value for Sahara Descriptor  8 */
#define SAH_HDR_MDHA_SET_MODE_HASH    0x208d0000
/*! Header value for Sahara Descriptor  10 */
#define SAH_HDR_MDHA_HASH             0xa0850000
/*! Header value for Sahara Descriptor  11 */
#define SAH_HDR_MDHA_STORE_DIGEST     0x20820000
/*! Header value for Sahara Descriptor  18 */
#define SAH_HDR_RNG_GENERATE          0x308C0000
/*! Header value for Sahara Descriptor  33 */
#define SAH_HDR_ARC4_SET_MODE_SBOX    0x90890000
/*! Header value for Sahara Descriptor  34 */
#define SAH_HDR_ARC4_READ_SBOX        0x90860000
/*! Header value for Sahara Descriptor  33 */
#define SAH_HDR_ARC4_SET_MODE_KEY     0x90830000

/*! Header bit indicating "Link-List optimization" */
#define SAH_HDR_LLO                   0x01000000
extern const uint32_t insert_mdha_algorithm[];

/*! 
 * @name MDHA Mode Register Values
 *
 * These are bit fields and combinations of bit fields for setting the Mode
 * Register portion of a Sahara Descriptor Header field.
 *
 * The parity bit has been set to ensure that these values have even parity,
 * therefore using an Exclusive-OR operation against an existing header will
 * preserve its parity.
 *
 */
 /*! @{ */
#define insert_mdha_ssl       0x80000400
#define insert_mdha_mac_full  0x80000200
#define insert_mdha_opad      0x80000080
#define insert_mdha_ipad      0x80000040
#define insert_mdha_init      0x80000020
#define insert_mdha_hmac      0x80000008
#define insert_mdha_pdata     0x80000004
#define insert_mdha_algorithm_sha224  0x00000003
#define insert_mdha_algorithm_sha256  0x80000002
#define insert_mdha_algorithm_md5     0x80000001
#define insert_mdha_algorithm_sha1    0x00000000
/*! @} */


extern const uint32_t insert_skha_algorithm[];
extern const uint32_t insert_skha_mode[];
extern const uint32_t insert_skha_modulus[];

/*! 
 * @name SKHA Mode Register Values
 *
 * These are bit fields and combinations of bit fields for setting the Mode
 * Register portion of a Sahara Descriptor Header field.
 *
 * The parity bit has been set to ensure that these values have even parity,
 * therefore using an Exclusive-OR operation against an existing header will
 * preserve its parity.
 *
 */
/*! @{ */
#define insert_skha_modulus_128    0x00001e00
#define insert_skha_no_key_parity  0x80000100
#define insert_skha_no_permute     0x80000020
#define insert_skha_algorithm_arc4 0x00000003
#define insert_skha_algorithm_tdes 0x80000002
#define insert_skha_algorithm_des  0x80000001
#define insert_skha_algorithm_aes  0x00000000
#define insert_skha_mode_ctr       0x00000018
#define insert_skha_mode_ccm       0x80000010
#define insert_skha_mode_cbc       0x80000008
#define insert_skha_mode_ecb       0x00000000
#define insert_skha_encrypt        0x80000004
/*! @} */


fsl_shw_return_t add_two_in_desc(uint32_t header,
                                 const uint8_t* in1,
                                 uint32_t in1_length,
                                 const uint8_t* in2,
                                 uint32_t in2_length,
                                 const sah_Mem_Util* mu,
                                 sah_Head_Desc** desc_chain);

fsl_shw_return_t add_in_key_desc(uint32_t header,
                                 const uint8_t* in1,
                                 uint32_t in1_length,
                                 fsl_shw_sko_t* key_info,
                                 const sah_Mem_Util* mu,
                                 sah_Head_Desc** desc_chain);

fsl_shw_return_t add_key_key_desc(uint32_t header,
                                 fsl_shw_sko_t* key_info1,
                                 fsl_shw_sko_t* key_info2,
                                 const sah_Mem_Util* mu,
                                 sah_Head_Desc** desc_chain);

fsl_shw_return_t add_two_out_desc(uint32_t header,
                                  const uint8_t* out1,
                                  uint32_t out1_length,
                                  const uint8_t* out2,
                                  uint32_t out2_length,
                                  const sah_Mem_Util* mu,
                                  sah_Head_Desc** desc_chain);

fsl_shw_return_t add_in_out_desc(uint32_t header,
                                 const uint8_t* in, uint32_t in_length,
                                 uint8_t* out, uint32_t out_length,
                                 const sah_Mem_Util* mu,
                                 sah_Head_Desc** desc_chain);

fsl_shw_return_t add_key_out_desc(uint32_t header, fsl_shw_sko_t *key_info,
                                  uint8_t* out, uint32_t out_length,
                                  const sah_Mem_Util *mu,
                                  sah_Head_Desc **desc_chain);

fsl_shw_return_t sah_validate_uco(fsl_shw_uco_t *uco);

#endif /* SF_UTIL_H */

/* End of sf_util.h */
