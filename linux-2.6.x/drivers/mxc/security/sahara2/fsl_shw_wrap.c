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
 * @file fsl_shw_wrap.c
 *
 * @brief This file implements Key-Wrap (Black Key) functions of the FSL SHW API for
 * Sahara.
 *
 * - Ownerid is an 8-byte, user-supplied, value to keep KEY confidential.
 * - KEY is a 1-32 byte value which starts in SCC RED RAM before
 *     wrapping, and ends up there on unwrap.  Length is limited because of
 *    size of SCC1 RAM.
 * - KEY' is the encrypted KEY
 * - LEN is a 1-byte (for now) byte-length of KEY
 * - ALG is a 1-byte value for the algorithm which which the key is
 *    associated.  Values are defined by the FSL SHW API
 * - Ownerid, LEN, and ALG come from the user's "key_info" object, as does the
 *    slot number where KEY already is/will be.
 * - T is a Nonce
 * - T' is the encrypted T
 * - KEK is a Key-Encryption Key for the user's Key
 * - ICV is the "Integrity Check Value" for the wrapped key
 * - Black Key is the string of bytes returned as the wrapped key
<table>
<tr><TD align="right">BLACK_KEY <TD width="3">=<TD>ICV | T' | LEN | ALG |
     KEY'</td></tr>
<tr><td>&nbsp;</td></tr>

<tr><th>To Wrap</th></tr>
<tr><TD align="right">T</td> <TD width="3">=</td> <TD>RND()<sub>16</sub>
    </td></tr>
<tr><TD align="right">KEK</td><TD width="3">=</td><TD>HASH<sub>sha1</sub>(T |
     Ownerid)<sub>16</sub></td></tr>
<tr><TD align="right">KEY'<TD width="3">=</td><TD>
     AES<sub>ctr-enc</sub>(Key=KEK, CTR=0, Data=KEY)</td></tr>
<tr><TD align="right">ICV</td><TD width="3">=</td><td>HMAC<sub>sha1</sub>
     (Key=T, Data=Ownerid | LEN | ALG | KEY')<sub>16</sub></td></tr>
<tr><TD align="right">T'</td><TD width="3">=</td><TD>TDES<sub>cbc-enc</sub>
     (Key=SLID, IV=Ownerid, Data=T)</td></tr>

<tr><td>&nbsp;</td></tr>

<tr><th>To Unwrap</th></tr>
<tr><TD align="right">T</td><TD width="3">=</td><TD>TDES<sub>ecb-dec</sub>
    (Key=SLID, IV=Ownerid, Data=T')</sub></td></tr>
<tr><TD align="right">ICV</td><TD width="3">=</td><td>HMAC<sub>sha1</sub>
    (Key=T, Data=Ownerid | LEN | ALG | KEY')<sub>16</sub></td></tr>
<tr><TD align="right">KEK</td><TD width="3">=</td><td>HASH<sub>sha1</sub>
    (T | Ownerid)<sub>16</sub></td></tr>
<tr><TD align="right">KEY<TD width="3">=</td><TD>AES<sub>ctr-dec</sub>
    (Key=KEK, CTR=0, Data=KEY')</td></tr>
</table>

 */

#include "sahara.h"
#include "sf_util.h"
#include "adaptor.h"

#if defined(DIAG_SECURITY_FUNC)
#include <diagnostic.h>
#endif

#ifdef __KERNEL__
EXPORT_SYMBOL(fsl_shw_establish_key);
EXPORT_SYMBOL(fsl_shw_extract_key);
EXPORT_SYMBOL(fsl_shw_release_key);
#endif


#define ICV_LENGTH 16
#define T_LENGTH 16
#define KEK_LENGTH 16
#define LENGTH_LENGTH 1
#define ALGORITHM_LENGTH 1

/* ICV | T' | LEN | ALG | KEY' */
#define ICV_OFFSET       0
#define T_PRIME_OFFSET   (ICV_OFFSET + ICV_LENGTH)
#define LENGTH_OFFSET    (T_PRIME_OFFSET + T_LENGTH)
#define ALGORITHM_OFFSET (LENGTH_OFFSET + LENGTH_LENGTH)
#define KEY_PRIME_OFFSET (ALGORITHM_OFFSET + ALGORITHM_LENGTH)

/*
 * For testing of the algorithm implementation,, the DO_REPEATABLE_WRAP flag
 * causes the T_block to go into the T field during a wrap operation.  This
 * will make the black key value repeatable (for a given SCC secret key, or
 * always if the default key is in use).
 *
 * Normally, a random sequence is used.
 */
#ifdef DO_REPEATABLE_WRAP
/**
 * Block of zeroes which is maximum Symmetric block size, used for
 * initializing context register, etc.
 */
static uint8_t T_block_[16] = {
    0x42, 0, 0, 0x42, 0x42, 0, 0, 0x42,
    0x42, 0, 0, 0x42, 0x42, 0, 0, 0x42
};
#endif

/*!
 * Insert descriptors to calculate ICV = HMAC(key=T, data=LEN|ALG|KEY')
 *
 * @param  user_ctx      User's context for this operation
 * @param  desc_chain    Descriptor chain to append to
 * @param  t_key_info    T's key object
 * @param  black_key     Beginning of Black Key region
 * @param  key_length    Number of bytes of key' there are in @c black_key
 * @param[out] hmac      Location to store ICV.  Will be tagged "USES" so
 *                       sf routines will not try to free it.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
static inline fsl_shw_return_t create_icv_calc(fsl_shw_uco_t* user_ctx,
                                               sah_Head_Desc** desc_chain,
                                               fsl_shw_sko_t* t_key_info,
                                               const uint8_t* black_key,
                                               uint32_t key_length,
                                               uint8_t* hmac)
{
    fsl_shw_return_t sah_code;
    uint32_t header;

    /* Load up T as key for the HMAC */
    header = (SAH_HDR_MDHA_SET_MODE_MD_KEY /* #6 */
              ^ insert_mdha_algorithm_sha1
              ^ insert_mdha_init ^ insert_mdha_hmac ^ insert_mdha_pdata
              ^ insert_mdha_mac_full);
    sah_code = add_in_key_desc(header,
                               NULL, 0,
                               t_key_info, /* Reference T in RED */
                               user_ctx->mem_util, desc_chain);

    /* Previous step loaded key; Now set up to hash the data */
    if (sah_code == FSL_RETURN_OK_S) {
        sah_Link* link1 = NULL;
        sah_Link* link2 = NULL;
        header = SAH_HDR_MDHA_HASH; /* #10 */

        /* Input - start with ownerid */
        sah_code = sah_Create_Link(user_ctx->mem_util, &link1,
                                   (void*)&t_key_info->userid,
                                   sizeof(t_key_info->userid),
                                   SAH_USES_LINK_DATA);

        if (sah_code == FSL_RETURN_OK_S) {
            /* Still input  - Append black-key fields len, alg, key' */
            sah_code = sah_Append_Link(user_ctx->mem_util, link1,
                                       (void*)black_key + LENGTH_OFFSET,
                                       (LENGTH_LENGTH
                                        + ALGORITHM_LENGTH
                                        + key_length),
                                       SAH_USES_LINK_DATA);

            if (sah_code != FSL_RETURN_OK_S) {
                link1 = NULL;
            } else {
                /* Output - computed ICV/HMAC */
                sah_code = sah_Create_Link(user_ctx->mem_util, &link2,
                                           hmac, ICV_LENGTH,
                                           SAH_USES_LINK_DATA |
                                           SAH_OUTPUT_LINK);
            }

            sah_code = sah_Append_Desc(user_ctx->mem_util, desc_chain,
                                       header, link1, link2);
        }
    }

    return sah_code;
} /* create_icv_calc */


/*!
 * Perform unwrapping of a black key into a RED slot
 *
 * @param         user_ctx      A user context from #fsl_shw_register_user().
 * @param[in,out] key_info      The information about the key to be which will
 *                              be unwrapped... key length, slot info, etc.
 * @param         black_key     Encrypted key
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
static fsl_shw_return_t unwrap(fsl_shw_uco_t* user_ctx,
                               fsl_shw_sko_t* key_info,
                               const uint8_t* black_key)
{
    fsl_shw_return_t ret = FSL_RETURN_ERROR_S;
    sah_Mem_Util *mu = user_ctx->mem_util;
    uint8_t* hmac = mu->mu_malloc(mu->mu_ref, ICV_LENGTH);

    if (hmac == NULL) {
        ret = FSL_RETURN_NO_RESOURCE_S;
    } else {
        sah_Head_Desc* desc_chain = NULL;
        fsl_shw_sko_t t_key_info;
        unsigned i;

        /* Set up key_info for "T" - use same slot as eventual key */
        fsl_shw_sko_init(&t_key_info, FSL_KEY_ALG_AES);
        t_key_info.userid  = key_info->userid;
        t_key_info.handle = key_info->handle;
        t_key_info.flags = key_info->flags;
        t_key_info.key_length = T_LENGTH;

        /* Compute T = SLID_decrypt(T'); leave in RED slot */
        ret = do_scc_slot_decrypt(user_ctx, key_info->userid,
                                  t_key_info.handle,
                                  T_LENGTH,
                                  black_key + T_PRIME_OFFSET);

        /* Compute ICV = HMAC(T, ownerid | len | alg | key' */
        if (ret == FSL_RETURN_OK_S) {
            ret = create_icv_calc(user_ctx, &desc_chain, &t_key_info,
                                  black_key, key_info->key_length, hmac);
            if (ret == FSL_RETURN_OK_S) {
                ret = sah_Descriptor_Chain_Execute(desc_chain, user_ctx);
                desc_chain = NULL;
            }
#ifdef DIAG_SECURITY_FUNC
            else {
                LOG_DIAG("Creation of sah_Key_Link failed due to bad key"
                         " flag!\n");
            }
#endif /*DIAG_SECURITY_FUNC*/

            /* Check computed ICV against value in Black Key */
            if (ret == FSL_RETURN_OK_S) {
                for (i = 0; i < ICV_LENGTH; i++) {
                    if (black_key[ICV_OFFSET+i] != hmac[i]) {
                        ret = FSL_RETURN_AUTH_FAILED_S;
                        break;
                    }
                }
            }

            /* This is no longer needed. */
            mu->mu_free(mu->mu_ref, hmac);

            /* Compute KEK = SHA1(T | ownerid).  Rewrite slot with value */
            if (ret == FSL_RETURN_OK_S) {
                sah_Link* link1 = NULL;
                sah_Link* link2 = NULL;
                uint32_t header;

                header = (SAH_HDR_MDHA_SET_MODE_HASH /* #8 */
                          ^ insert_mdha_init
                          ^ insert_mdha_algorithm_sha1
                          ^ insert_mdha_pdata);

                /* Input - Start with T */
                ret = sah_Create_Key_Link(user_ctx->mem_util, &link1,
                                               &t_key_info);

                if (ret == FSL_RETURN_OK_S) {
                    /* Still input - append ownerid */
                    ret = sah_Append_Link(user_ctx->mem_util, link1,
                                          (void*)&key_info->userid,
                                           sizeof(key_info->userid),
                                           SAH_USES_LINK_DATA);
                }

                if (ret == FSL_RETURN_OK_S) {
                    /* Output - KEK goes into RED slot */
                    ret = sah_Create_Key_Link(user_ctx->mem_util, &link2,
                                              &t_key_info);
                }

                if (ret == FSL_RETURN_OK_S) {
                    /* Put the Hash calculation into the chain. */
                    ret = sah_Append_Desc(user_ctx->mem_util, &desc_chain,
                                          header, link1, link2);
                }
            }

            /* Compute KEY = AES-decrypt(KEK, KEY') */
            if (ret == FSL_RETURN_OK_S) {
                uint32_t header;
                unsigned rounded_key_length;
                unsigned original_key_length = key_info->key_length;

                header = (SAH_HDR_SKHA_SET_MODE_IV_KEY /* #1 */
                          ^ insert_skha_mode_ctr
                          ^ insert_skha_algorithm_aes
                          ^ insert_skha_modulus_128);
                /* Load KEK in as the key to use */
                ret = add_in_key_desc(header, NULL, 0, &t_key_info,
                                      user_ctx->mem_util, &desc_chain);

                /* Make sure that KEY = AES(KEK, KEY') step writes a multiple
                   of words into the SCC to avoid 'Byte Access errors.' */
                if ((original_key_length & 3) != 0) {
                    rounded_key_length = original_key_length + 4
                        - (original_key_length & 3);
                } else {
                    rounded_key_length = original_key_length;
                }

                key_info->key_length = rounded_key_length;
                if (ret == FSL_RETURN_OK_S) {
                    /* Now set up for computation.  Result in RED */
                    header = SAH_HDR_SKHA_ENC_DEC; /* #4 */

                    ret = add_in_key_desc(header, black_key + KEY_PRIME_OFFSET,
                                          rounded_key_length, key_info,
                                          user_ctx->mem_util, &desc_chain);
                    key_info->key_length = original_key_length;
                    /* Perform the operation */
                    if (ret == FSL_RETURN_OK_S) {
                        ret =
                           sah_Descriptor_Chain_Execute(desc_chain, user_ctx);
                    }
                }
            }

            /* Erase tracks */
            t_key_info.userid = 0xdeadbeef;
            t_key_info.handle = 0xdeadbeef;
        }
    }

    return ret;
} /* unwrap */


/*!
 * Perform wrapping of a black key from a RED slot
 *
 * @param      user_ctx         A user context from #fsl_shw_register_user().
 * @param[in,out] key_info      The information about the key to be which will
 *                              be wrapped... key length, slot info, etc.
 * @param      black_key        Place to store encrypted key
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
static fsl_shw_return_t wrap(fsl_shw_uco_t* user_ctx,
                             fsl_shw_sko_t* key_info,
                             uint8_t* black_key)
{
    fsl_shw_return_t ret;
    sah_Head_Desc    *desc_chain = NULL;
    unsigned         slots_allocated = 0; /* boolean */
    fsl_shw_sko_t    T_key_info; /* for holding T */
    fsl_shw_sko_t    KEK_key_info; /* for holding KEK */


    black_key[LENGTH_OFFSET] = key_info->key_length;
    black_key[ALGORITHM_OFFSET] = key_info->algorithm;

    memcpy(&T_key_info, key_info, sizeof(T_key_info));
    fsl_shw_sko_set_key_length(&T_key_info, T_LENGTH);
    T_key_info.algorithm = FSL_KEY_ALG_HMAC;

    memcpy(&KEK_key_info, &T_key_info, sizeof(KEK_key_info));
    KEK_key_info.algorithm = FSL_KEY_ALG_AES;

    ret = do_scc_slot_alloc(user_ctx, T_LENGTH, key_info->userid,
                            &T_key_info.handle);

    if (ret == FSL_RETURN_OK_S) {
        ret = do_scc_slot_alloc(user_ctx, KEK_LENGTH, key_info->userid,
                                &KEK_key_info.handle);
        if (ret != FSL_RETURN_OK_S) {
#ifdef DIAG_SECURITY_FUNC
            LOG_DIAG("do_scc_slot_alloc() failed");
#endif
            do_scc_slot_dealloc(user_ctx, key_info->userid, T_key_info.handle);
        } else {
            slots_allocated = 1;
        }

    }

    /* Set up to compute everything except T' ... */
    if (ret == FSL_RETURN_OK_S) {
        uint32_t header;

#ifndef DO_REPEATABLE_WRAP
        /* Compute T = RND() */
        header = SAH_HDR_RNG_GENERATE; /* Desc. #18 */
        ret = add_key_out_desc(header, &T_key_info, NULL, 0,
                               user_ctx->mem_util, &desc_chain);
#else
        ret = do_scc_slot_load_slot(user_ctx, T_key_info.userid,
                                    T_key_info.handle, T_block,
                                    T_key_info.key_length);
#endif

        /* Compute KEK = SHA1(T | Ownerid) */
        if (ret == FSL_RETURN_OK_S) {
            sah_Link* link1;
            sah_Link* link2;

            header = (SAH_HDR_MDHA_SET_MODE_HASH /* #8 */
                      ^ insert_mdha_init
                      ^ insert_mdha_algorithm[FSL_HASH_ALG_SHA1]
                      ^ insert_mdha_pdata);

            /* Input - Start with T */
            ret = sah_Create_Key_Link(user_ctx->mem_util, &link1, &T_key_info);

            if (ret == FSL_RETURN_OK_S) {
                /* Still input - append ownerid */
                ret = sah_Append_Link(user_ctx->mem_util, link1,
                                      (void*)&key_info->userid,
                                      sizeof(key_info->userid),
                                      SAH_USES_LINK_DATA);
            }

            if (ret == FSL_RETURN_OK_S) {
                /* Output - KEK goes into RED slot */
                ret = sah_Create_Key_Link(user_ctx->mem_util, &link2,
                                          &KEK_key_info);
            }

            /* Put the Hash calculation into the chain. */
            if (ret == FSL_RETURN_OK_S) {
                ret = sah_Append_Desc(user_ctx->mem_util, &desc_chain,
                                      header, link1, link2);
            }
        }

        /* Compute KEY' = AES-encrypt(KEK, KEY) */
        if (ret == FSL_RETURN_OK_S) {

            header = (SAH_HDR_SKHA_SET_MODE_IV_KEY /* #1 */
                      ^ insert_skha_mode[FSL_SYM_MODE_CTR]
                  ^ insert_skha_algorithm[FSL_KEY_ALG_AES]
                  ^ insert_skha_modulus[FSL_CTR_MOD_128]);
            /* Set up KEK as key to use */
            ret = add_in_key_desc(header, NULL, 0, &KEK_key_info,
                                  user_ctx->mem_util, &desc_chain);

            /* Set up KEY as input, KEY' as output */
            if (ret == FSL_RETURN_OK_S) {
                header = SAH_HDR_SKHA_ENC_DEC; /* #4 */
                ret = add_key_out_desc(header, key_info,
                                       black_key + KEY_PRIME_OFFSET,
                                       key_info->key_length,
                                       user_ctx->mem_util, &desc_chain);
            }

#ifdef DIAG_SECURITY_FUNC
            if (ret != FSL_RETURN_OK_S) {
                LOG_DIAG("Creation of sah_Key_Link failed due to bad key"
                         " flag!\n");
            }
#endif /*DIAG_SECURITY_FUNC*/
        }

        /* Compute and store ICV into Black Key */
        if (ret == FSL_RETURN_OK_S) {
            ret = create_icv_calc(user_ctx, &desc_chain, &T_key_info,
                                  black_key, key_info->key_length,
                                  black_key + ICV_OFFSET);
#ifdef DIAG_SECURITY_FUNC
            if (ret != FSL_RETURN_OK_S) {
                LOG_DIAG("Creation of sah_Key_Link failed due to bad key"
                         " flag!\n");
            }
#endif /*DIAG_SECURITY_FUNC*/
        }
    }

    /* Now get Sahara to do the work. */
    if (ret == FSL_RETURN_OK_S) {
        ret = sah_Descriptor_Chain_Execute(desc_chain, user_ctx);
#ifdef DIAG_SECURITY_FUNC
        if (ret != FSL_RETURN_OK_S) {
            LOG_DIAG("sah_Descriptor_Chain_Execute() failed");
        }
#endif
    }

    /* Compute T' = SLID_encrypt(T); Result goes to Black Key */
    if (ret == FSL_RETURN_OK_S) {
        ret = do_scc_slot_encrypt(user_ctx, T_key_info.userid,
                                  T_key_info.handle,
                                  T_LENGTH,
                                  black_key + T_PRIME_OFFSET);
#ifdef DIAG_SECURITY_FUNC
        if (ret != FSL_RETURN_OK_S) {
            LOG_DIAG("do_scc_slot_encrypt() failed");
        }
#endif
    }

    if (slots_allocated) {
        do_scc_slot_dealloc(user_ctx, key_info->userid, T_key_info.handle);
        do_scc_slot_dealloc(user_ctx, key_info->userid, KEK_key_info.handle);
    }

    return ret;
} /* wrap */


/*!
 * Place a key into a protected location for use only by cryptographic
 * algorithms.
 *
 * This only needs to be used to a) unwrap a key, or b) set up a key which
 * could be wrapped with a later call to #fsl_shw_extract_key().  Normal
 * cleartext keys can simply be placed into #fsl_shw_sko_t key objects with
 * #fsl_shw_sko_set_key() and used directly.
 *
 * The maximum key size supported for wrapped/unwrapped keys is 32 octets.
 * (This is the maximum reasonable key length on Sahara - 32 octets for an HMAC
 * key based on SHA-256.)  The key size is determined by the @a key_info.  The
 * expected length of @a key can be determined by
 * #fsl_shw_sko_calculate_wrapped_size()
 *
 * The protected key will not be available for use until this operation
 * successfully completes.
 *
 * This feature is not available for all platforms, nor for all algorithms and
 * modes.
 *
 * @param      user_ctx         A user context from #fsl_shw_register_user().
 * @param[in,out] key_info      The information about the key to be which will
 *                              be established.  In the create case, the key
 *                              length must be set.
 * @param      establish_type   How @a key will be interpreted to establish a
 *                              key for use.
 * @param key                   If @a establish_type is #FSL_KEY_WRAP_UNWRAP,
 *                              this is the location of a wrapped key.  If
 *                              @a establish_type is #FSL_KEY_WRAP_CREATE, this
 *                              parameter can be @a NULL.  If @a establish_type
 *                              is #FSL_KEY_WRAP_ACCEPT, this is the location
 *                              of a plaintext key.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t fsl_shw_establish_key(
                                       fsl_shw_uco_t* user_ctx,
                                       fsl_shw_sko_t* key_info,
                                       fsl_shw_key_wrap_t establish_type,
                                       const uint8_t* key)
{
    fsl_shw_return_t ret;
    sah_Head_Desc* desc_chain = NULL;
    uint32_t header = SAH_HDR_RNG_GENERATE; /* Desc. #18 for rand */
    unsigned original_key_length = key_info->key_length;
    unsigned rounded_key_length;
 
    /* Write operations into SCC memory require word-multiple number of
     * bytes.  For ACCEPT and CREATE functions, the key length may need
     * to be rounded up.  Calculate. */
    if ((original_key_length & 3) != 0) {
        rounded_key_length = original_key_length + 4
            - (original_key_length & 3);
    } else {
        rounded_key_length = original_key_length;
    }

    /* perform sanity check on the uco */
    ret = sah_validate_uco(user_ctx);
    if (ret != FSL_RETURN_OK_S) {
        return ret;
    }

    ret = do_scc_slot_alloc(user_ctx, key_info->key_length, key_info->userid,
                            &key_info->handle);

    if (ret == FSL_RETURN_OK_S) {
        key_info->flags |= FSL_SKO_KEY_ESTABLISHED;
        switch (establish_type) {
        case FSL_KEY_WRAP_CREATE:
            /* Use safe version of key length */
            key_info->key_length = rounded_key_length;
            /* Generate descriptor to put random value into */
            ret = add_key_out_desc(header, key_info, NULL, 0,
                                   user_ctx->mem_util, &desc_chain);
            /* Restore actual, desired key length */
            key_info->key_length = original_key_length;

            if (ret == FSL_RETURN_OK_S) {
                uint32_t old_flags = user_ctx->flags;

                /* Now put random value into key */
                ret = sah_Descriptor_Chain_Execute(desc_chain, user_ctx);
                /* Restore user's old flag value */
                user_ctx->flags = old_flags;
            }
#ifdef DIAG_SECURITY_FUNC
            else {
                LOG_DIAG("Creation of sah_Key_Link failed due to bad"
                         " key flag!\n");
            }
#endif /*DIAG_SECURITY_FUNC*/
            break;

        case FSL_KEY_WRAP_ACCEPT:
            if (key == NULL) {
#ifdef DIAG_SECURITY_FUNC
                LOG_DIAG("ACCEPT:  Red Key is NULL");
#endif
                ret = FSL_RETURN_ERROR_S;
            } else {
                /* Copy in safe number of bytes of Red key */
                ret = do_scc_slot_load_slot(user_ctx, key_info->userid,
                                            key_info->handle, key,
                                            rounded_key_length);
            }
            break;

        case FSL_KEY_WRAP_UNWRAP:
            /* For now, disallow non-blocking calls. */
            if (!(user_ctx->flags & FSL_UCO_BLOCKING_MODE)) {
                ret = FSL_RETURN_BAD_FLAG_S;
            }
            else if (key == NULL) {
                ret = FSL_RETURN_ERROR_S;
            } else {
                ret = unwrap(user_ctx, key_info, key);
                if (ret != FSL_RETURN_OK_S) {
                }
            }
            break;

        default:
            ret = FSL_RETURN_BAD_FLAG_S;
            break;
        } /* switch */

        if (ret != FSL_RETURN_OK_S) {
            fsl_shw_return_t scc_err;
            scc_err = do_scc_slot_dealloc(user_ctx, key_info->userid,
                                          key_info->handle);
            key_info->flags &= ~FSL_SKO_KEY_ESTABLISHED;
        }
    }

    return ret;
}


/*!
 * Wrap a key and retrieve the wrapped value.
 *
 * A wrapped key is a key that has been cryptographically obscured.  It is
 * only able to be used with #fsl_shw_establish_key().
 *
 * This function will also release the key (see #fsl_shw_release_key()) so
 * that it must be re-established before reuse.
 *
 * This feature is not available for all platforms, nor for all algorithms and
 * modes.
 *
 * @param      user_ctx         A user context from #fsl_shw_register_user().
 * @param      key_info         The information about the key to be deleted.
 * @param[out] covered_key      The location to store the 48-octet wrapped key.
 *                              (This size is based upon the maximum key size
 *                              of 32 octets).
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t fsl_shw_extract_key(fsl_shw_uco_t* user_ctx,
                                     fsl_shw_sko_t* key_info,
                                     uint8_t* covered_key)
{
    fsl_shw_return_t ret;

    /* perform sanity check on the uco */
    ret = sah_validate_uco(user_ctx);
    if (ret == FSL_RETURN_OK_S) {
        /* For now, only blocking mode calls are supported */
        if (user_ctx->flags & FSL_UCO_BLOCKING_MODE) {
            if (key_info->flags & FSL_SKO_KEY_ESTABLISHED) {
                ret = wrap(user_ctx, key_info, covered_key);

                /* Need to deallocate on successful extraction */
                if (ret == FSL_RETURN_OK_S) {
                    do_scc_slot_dealloc(user_ctx, key_info->userid,
                                        key_info->handle);
                    /* Mark key not available in the flags */
                    key_info->flags &=
                        ~(FSL_SKO_KEY_ESTABLISHED | FSL_SKO_KEY_PRESENT);
                }
            }
        }
    }

    return ret;
}


/*!
 * De-establish a key so that it can no longer be accessed.
 *
 * The key will need to be re-established before it can again be used.
 *
 * This feature is not available for all platforms, nor for all algorithms and
 * modes.
 *
 * @param      user_ctx         A user context from #fsl_shw_register_user().
 * @param      key_info         The information about the key to be deleted.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t fsl_shw_release_key(
                             fsl_shw_uco_t* user_ctx,
                             fsl_shw_sko_t* key_info)
{
    fsl_shw_return_t ret;

    /* perform sanity check on the uco */
    ret = sah_validate_uco(user_ctx);

    if (ret == FSL_RETURN_OK_S) {
        if (key_info->flags & FSL_SKO_KEY_ESTABLISHED) {
            ret = do_scc_slot_dealloc(user_ctx, key_info->userid,
                                      key_info->handle);
            key_info->flags &= ~(FSL_SKO_KEY_ESTABLISHED |
                                 FSL_SKO_KEY_PRESENT);
        }
    }

    return ret;
}
