/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
/*!
 * @file fsl_shw_sym.c
 *
 * @brief This file implements Symmetric Cipher functions of the FSL SHW API for
 * Sahara.  This does not include CCM.
 *
 * @ingroup MXCSAHARA2
 */

#include "sahara.h"
#include "sf_util.h"
#include "adaptor.h"

#ifdef LINUX_KERNEL
EXPORT_SYMBOL(fsl_shw_symmetric_encrypt);
EXPORT_SYMBOL(fsl_shw_symmetric_decrypt);
#endif

/*!
 * Block of zeroes which is maximum Symmetric block size, used for
 * initializing context register, etc.
 */
static uint8_t block_zeros[16] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};


typedef enum cipher_direction {
    SYM_DECRYPT,
    SYM_ENCRYPT
} cipher_direction_t;

/*! 
 * Create and run the chain for a symmetric-key operation.
 *
 * @param user_ctx    Who the user is
 * @param key_info    What key is to be used
 * @param sym_ctx     Info details about algorithm
 * @param encrypt     0 = decrypt, non-zero = encrypt
 * @param length      Number of octets at @a in and @a out
 * @param in          Pointer to input data
 * @param out         Location to store output data
 *
 * @return         The status of handing chain to driver,
 *                 or an earlier argument/flag or allocation
 *                 error.
 */
static fsl_shw_return_t do_symmetric(
                                fsl_shw_uco_t* user_ctx,
                                fsl_shw_sko_t* key_info,
                                fsl_shw_scco_t* sym_ctx,
                                cipher_direction_t encrypt,
                                uint32_t length,
                                const uint8_t* in,
                                uint8_t* out)
{
    fsl_shw_return_t status = FSL_RETURN_ERROR_S;
    uint32_t header;
    sah_Head_Desc* desc_chain = NULL;
    sah_Oct_Str ptr1;

    /* Two different sets of chains, depending on algorithm */
    if (key_info->algorithm == FSL_KEY_ALG_ARC4) {
        if (sym_ctx->flags & FSL_SYM_CTX_INIT) {
            /* Desc. #35 w/ARC4 - start from key*/
            header = SAH_HDR_ARC4_SET_MODE_KEY
                ^ insert_skha_algorithm_arc4;

            status = add_in_key_desc(header, NULL, 0, key_info,
                                     user_ctx->mem_util, &desc_chain);

        } else {                /* load SBox */
            /* Desc. #33 w/ARC4 and NO PERMUTE */
            header = SAH_HDR_ARC4_SET_MODE_SBOX
                ^ insert_skha_no_permute
                ^ insert_skha_algorithm_arc4;
            status = add_two_in_desc(header, sym_ctx->context, 256,
                                     sym_ctx->context+256, 3,
                                     user_ctx->mem_util, &desc_chain);

        } /* load SBox */

        /* Add in-out data descriptor */
        if ((status == FSL_RETURN_OK_S) && (length != 0)) {
            status = add_in_out_desc(SAH_HDR_SKHA_ENC_DEC,
                                     in, length, out, length,
                                     user_ctx->mem_util, &desc_chain);
        }

        /* Operation is done ... save what came out? */
        if (sym_ctx->flags & FSL_SYM_CTX_SAVE) {
            /* Desc. #34 - Read SBox, pointers */
            header = SAH_HDR_ARC4_READ_SBOX;
            if (status == FSL_RETURN_OK_S) {
                status = add_two_out_desc(header, sym_ctx->context, 256,
                                          sym_ctx->context+256, 3,
                                          user_ctx->mem_util, &desc_chain);
            }
        }
    } else { /* not ARC4 */
        /* Doing 1- or 2- descriptor chain. */
        /* Desc. #1 and algorithm and mode */
        header = SAH_HDR_SKHA_SET_MODE_IV_KEY
            ^ insert_skha_mode[sym_ctx->mode]
            ^ insert_skha_algorithm[key_info->algorithm];

        /* Honor 'no key parity checking' for DES and TDES */
        if ((key_info->flags & FSL_SKO_KEY_IGNORE_PARITY) &&
            ((key_info->algorithm == FSL_KEY_ALG_DES) ||
             (key_info->algorithm == FSL_KEY_ALG_TDES))) {
            header ^= insert_skha_no_key_parity;
        }

        /* Header by default is decrypting, so... */
        if (encrypt == SYM_ENCRYPT) {
            header ^= insert_skha_encrypt;
        }

        if (sym_ctx->mode == FSL_SYM_MODE_CTR) {
            header ^= insert_skha_modulus[sym_ctx->modulus_exp];
        }

        if ((sym_ctx->mode == FSL_SYM_MODE_ECB)
            || (sym_ctx->flags & FSL_SYM_CTX_INIT)) {
            ptr1 = block_zeros;
        } else {
            ptr1 = sym_ctx->context;
        }

        status = add_in_key_desc(header, ptr1, sym_ctx->block_size_bytes,
                                 key_info, user_ctx->mem_util, &desc_chain);

        /* Add in-out data descriptor */
        if ((length != 0) && (status == FSL_RETURN_OK_S)) {
            status = add_in_out_desc(SAH_HDR_SKHA_ENC_DEC, in, length,
                                     out, length, user_ctx->mem_util,
                                     &desc_chain);
        }

        /* Unload any desired context */
        if ((sym_ctx->flags & FSL_SYM_CTX_SAVE) &&
            (status == FSL_RETURN_OK_S)) {
            status = add_two_out_desc(SAH_HDR_SKHA_READ_CONTEXT_IV,
                                      NULL, 0, sym_ctx->context,
                                      sym_ctx->block_size_bytes,
                                      user_ctx->mem_util, &desc_chain);
        }

    } /* not ARC4 */

    if (status != FSL_RETURN_OK_S) {
        /* Delete possibly-started chain */
        sah_Descriptor_Chain_Destroy(user_ctx->mem_util, &desc_chain);
    } else {
        status = sah_Descriptor_Chain_Execute(desc_chain, user_ctx);
    }

    return status;
}


/* REQ-S2LRD-PINTFC-API-BASIC-SYM-002 */
/* PINTFC-API-BASIC-SYM-ARC4-001 */
/* PINTFC-API-BASIC-SYM-ARC4-002 */

/*!
 * Compute symmetric encryption
 *
 *
 * @param    user_ctx
 * @param    key_info
 * @param    sym_ctx
 * @param    length
 * @param    pt
 * @param    ct
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t fsl_shw_symmetric_encrypt(
                                fsl_shw_uco_t* user_ctx,
                                fsl_shw_sko_t* key_info,
                                fsl_shw_scco_t* sym_ctx,
                                uint32_t length,
                                const uint8_t* pt,
                                uint8_t* ct)
{
    fsl_shw_return_t ret;


    /* perform sanity check on uco */
    ret = sah_validate_uco(user_ctx);

    if (ret == FSL_RETURN_OK_S) {
        ret = do_symmetric(user_ctx, key_info, sym_ctx, SYM_ENCRYPT,
                           length, pt, ct);
    }

    return ret;
}


/* PINTFC-API-BASIC-SYM-002 */
/* PINTFC-API-BASIC-SYM-ARC4-001 */
/* PINTFC-API-BASIC-SYM-ARC4-002 */

/*!
 * Compute symmetric decryption
 *
 *
 * @param    user_ctx
 * @param    key_info
 * @param    sym_ctx
 * @param    length
 * @param    pt
 * @param    ct
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t fsl_shw_symmetric_decrypt(
                                fsl_shw_uco_t* user_ctx,
                                fsl_shw_sko_t* key_info,
                                fsl_shw_scco_t* sym_ctx,
                                uint32_t length,
                                const uint8_t* ct,
                                uint8_t* pt)
{
    fsl_shw_return_t ret;


    /* perform sanity check on uco */
    ret = sah_validate_uco(user_ctx);

    if (ret == FSL_RETURN_OK_S) {
        ret = do_symmetric(user_ctx, key_info, sym_ctx, SYM_DECRYPT,
                           length, ct, pt);
    }

    return ret;
}

