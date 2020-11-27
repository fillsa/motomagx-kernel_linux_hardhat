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
 * @file fsl_shw_hmac.c
 *
 * @brief This file implements Hashed Message Authentication Code functions of the FSL
 * SHW API for Sahara.
 *
 * @ingroup MXCSAHARA2
 */

#include "sahara.h"
#include "sf_util.h"

#ifdef __KERNEL__
EXPORT_SYMBOL(fsl_shw_hmac_precompute);
EXPORT_SYMBOL(fsl_shw_hmac);
#endif


/* REQ-S2LRD-PINTFC-API-BASIC-HMAC-001 */
/*!
 * Get the precompute information
 *
 *
 * @param   user_ctx
 * @param   key_info
 * @param   hmac_ctx
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t fsl_shw_hmac_precompute(
                                fsl_shw_uco_t* user_ctx,
                                fsl_shw_sko_t* key_info,
                                fsl_shw_hmco_t* hmac_ctx)
{
    fsl_shw_return_t status;
    sah_Head_Desc* desc_chain = NULL;
    uint32_t header;


    /* perform sanity check on uco */
    status = sah_validate_uco(user_ctx);
    if (status != FSL_RETURN_OK_S) {
        return status;
    }

    if ((key_info->algorithm != FSL_KEY_ALG_HMAC) ||
        (key_info->key_length > 64)) {
        return FSL_RETURN_BAD_ALGORITHM_S;
    } else if (key_info->key_length == 0) {
        return FSL_RETURN_BAD_KEY_LENGTH_S;
    }

    /* Set up to start the Inner Calculation */
    /* Desc. #8 w/IPAD, & INIT */
    header = SAH_HDR_MDHA_SET_MODE_HASH
             ^ insert_mdha_ipad
             ^ insert_mdha_init
             ^ insert_mdha_algorithm[hmac_ctx->algorithm];

    status = add_key_out_desc(header, key_info,
                             (uint8_t*)hmac_ctx->inner_precompute,
                             hmac_ctx->context_register_length,
                             user_ctx->mem_util, &desc_chain);


   /* Set up for starting Outer calculation */
    if (status == FSL_RETURN_OK_S) {
        /* exchange IPAD bit for OPAD bit */
        header ^= (insert_mdha_ipad ^ insert_mdha_opad);

        /* Theoretically, we can leave this link out and use the key which is
         * already in the register... however, if we do, the resulting opad
         * hash does not have the correct value when using the model. */
        status = add_key_out_desc(header, key_info,
                                 (uint8_t*)hmac_ctx->outer_precompute,
                                 hmac_ctx->context_register_length,
                                 user_ctx->mem_util, &desc_chain);
    }

    /* Pass the chain to the driver. */
    if (status == FSL_RETURN_OK_S) {
        status = sah_Descriptor_Chain_Execute(desc_chain, user_ctx);
        if (status == FSL_RETURN_OK_S) {
            /* flag that precomputes have been entered in this hco
             * assume it'll first be used for initilizing */
            hmac_ctx->flags |= (FSL_HMAC_FLAGS_INIT |
                                FSL_HMAC_FLAGS_PRECOMPUTES_PRESENT);
        }
    } else {
        sah_Descriptor_Chain_Destroy(user_ctx->mem_util, &desc_chain);
    }

    return status;
}


/* REQ-S2LRD-PINTFC-API-BASIC-HMAC-002 */
/*!
 * Get the hmac
 *
 *
 * @param         user_ctx    Info for acquiring memory
 * @param         key_info
 * @param         hmac_ctx
 * @param         msg
 * @param         length
 * @param         result
 * @param         result_len
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t fsl_shw_hmac(
                                fsl_shw_uco_t* user_ctx,
                                fsl_shw_sko_t* key_info,
                                fsl_shw_hmco_t* hmac_ctx,
                                const uint8_t* msg,
                                uint32_t length,
                                uint8_t* result,
                                uint32_t result_len)
{
    fsl_shw_return_t ret;
    sah_Head_Desc* desc_chain = NULL;
    uint32_t header;


    /* perform sanity check on uco */
    ret = sah_validate_uco(user_ctx);

    /* check flag consistency */
    /* Note that Final, Init, and Save are an illegal combination when a key
     * is being used. Because of the logic flow of this routine, that is
     * taken care of without being explict */
    if (ret == FSL_RETURN_OK_S) {
        if (
            /* nothing to work on */
            (((hmac_ctx->flags & FSL_HMAC_FLAGS_INIT) == 0) &&
             ((hmac_ctx->flags & FSL_HMAC_FLAGS_LOAD) == 0))        ||
            /* can't do both */
            ((hmac_ctx->flags & FSL_HMAC_FLAGS_INIT) &&
             (hmac_ctx->flags & FSL_HMAC_FLAGS_LOAD))               ||
            /* must be some output */
            (((hmac_ctx->flags & FSL_HMAC_FLAGS_SAVE)     == 0) &&
             ((hmac_ctx->flags & FSL_HMAC_FLAGS_FINALIZE) == 0)))   {
            ret = FSL_RETURN_BAD_FLAG_S;
        }
    }

    /* build descriptor #6 */
    if (ret == FSL_RETURN_OK_S) {

        /* start building descriptor header */
        header = SAH_HDR_MDHA_SET_MODE_MD_KEY ^
                 insert_mdha_algorithm[hmac_ctx->algorithm] ^
                 insert_mdha_init;

        /* if this is to finalize the digest, mark to pad last block */
        if (hmac_ctx->flags & FSL_HMAC_FLAGS_FINALIZE) {
            header ^= insert_mdha_pdata;
        }

        /*** Check if this is a one shot ***/
        if ((hmac_ctx->flags & FSL_HMAC_FLAGS_INIT) &&
            (hmac_ctx->flags & FSL_HMAC_FLAGS_FINALIZE)) {

            header ^= insert_mdha_hmac;

            /*** See if this uses Precomputes ***/
            if (hmac_ctx->flags & FSL_HMAC_FLAGS_PRECOMPUTES_PRESENT) {
                ret = add_two_in_desc(header,
                                      (uint8_t *)hmac_ctx->inner_precompute,
                                      hmac_ctx->context_register_length,
                                      (uint8_t *)hmac_ctx->outer_precompute,
                                      hmac_ctx->context_length,
                                      user_ctx->mem_util,
                                      &desc_chain);
            } else { /*** Precomputes not requested, try using Key ***/
                if (key_info->key != NULL) {
                    /* first, validate the key fields and related flag */
                    if (key_info->key_length == 0) {
                        ret = FSL_RETURN_BAD_KEY_LENGTH_S;
                    } else {
                        if ((key_info->algorithm != FSL_KEY_ALG_HMAC) ||
                            (key_info->key_length > 64)) {
                            ret = FSL_RETURN_BAD_ALGORITHM_S;
                        }
                    }

                    if (ret == FSL_RETURN_OK_S) {
                        /* finish building descriptor header (Key specific) */
                        header ^= insert_mdha_mac_full;
                        ret = add_in_key_desc(header, NULL, 0, key_info,
                                              user_ctx->mem_util, &desc_chain);
                    }
                } else { /*** not using Key or Precomputes, so die ***/
                    ret = FSL_RETURN_BAD_FLAG_S;
                }
            }
        } else {  /*** it's not a one shot, must be multi-step ***/
            /* this the last chunk? */
            if (hmac_ctx->flags & FSL_HMAC_FLAGS_FINALIZE) {
                header ^= insert_mdha_hmac;
                ret = add_two_in_desc(header,
                                      (uint8_t *)hmac_ctx->ongoing_context,
                                      hmac_ctx->context_register_length,
                                      (uint8_t *)hmac_ctx->outer_precompute,
                                      hmac_ctx->context_length,
                                      user_ctx->mem_util, &desc_chain);
            } else { /* not last chunk */
                uint8_t *ptr1;

                if (hmac_ctx->flags & FSL_HMAC_FLAGS_INIT) {
                    /* must be using precomputes, cannot 'chunk' message
                     * starting with a key */
                    if (hmac_ctx->flags & FSL_HMAC_FLAGS_PRECOMPUTES_PRESENT) {
                        ptr1 = (uint8_t *)hmac_ctx->inner_precompute;
                    } else {
                        ret = FSL_RETURN_NO_RESOURCE_S;
                    }
                } else {
                    ptr1 = (uint8_t *)hmac_ctx->ongoing_context;
                }

                if (ret == FSL_RETURN_OK_S) {
                    ret = add_two_in_desc(header, ptr1,
                                        hmac_ctx->context_register_length,
                                        NULL, 0, user_ctx->mem_util,
                                        &desc_chain);
                }
            }
        }
    }  /* end of building descriptor #6 */

    /*** build descriptor #10 & maybe 11 ***/
    if (ret == FSL_RETURN_OK_S) {
        header = SAH_HDR_MDHA_HASH;

        if (hmac_ctx->flags & FSL_HMAC_FLAGS_FINALIZE) {
            /* check that the results parameters seem reasonable */
            if ((result_len != 0) && (result != NULL)) {
                if (result_len > hmac_ctx->context_register_length) {
                    result_len = hmac_ctx->context_register_length;
                }

                /* message in / digest out (descriptor #10) */
                ret = add_in_out_desc(header, msg, length, result, result_len,
                                      user_ctx->mem_util, &desc_chain);

                /*** see if descriptor #11 needs to be built ***/
                if ((hmac_ctx->flags & FSL_HMAC_FLAGS_SAVE) &&
                    (ret == FSL_RETURN_OK_S)) {
                    header = SAH_HDR_MDHA_STORE_DIGEST;
                    /* nothing in / context out */
                    ret = add_two_in_desc(header, NULL, 0,
                                    (uint8_t *)hmac_ctx->ongoing_context,
                                    hmac_ctx->context_register_length,
                                    user_ctx->mem_util, &desc_chain);
                }
            } else {
                /* something wrong with result or its length */
                ret = FSL_RETURN_BAD_DATA_LENGTH_S;
            }
        } else {  /* finalize not set, so store in ongoing context field */
            if ((length % 64) == 0) {
                /* message in / context out */
                ret = add_in_out_desc(header, msg, length,
                                (uint8_t *)hmac_ctx->ongoing_context,
                                hmac_ctx->context_register_length,
                                user_ctx->mem_util, &desc_chain);
            } else {
                ret = FSL_RETURN_BAD_DATA_LENGTH_S;
            }
        }
    }

    /* Pass the chain to the driver. */
    if (ret == FSL_RETURN_OK_S) {
        ret = sah_Descriptor_Chain_Execute(desc_chain, user_ctx);
    } else {
        if (desc_chain != NULL) {
            /* ignore the destroy function's return value, the destroy
             * function's return value is not what went wroung with this
             * routine */
            sah_Descriptor_Chain_Destroy(user_ctx->mem_util, &desc_chain);
        }
    }

    return ret;
}

