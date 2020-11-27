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
 * @file fsl_shw_rand.c
 *
 * @brief This file implements Random Number Generation functions of the FSL SHW API for
 * for Sahara.
 *
 * @ingroup MXCSAHARA2
 */

#include "sahara.h"
#include "sf_util.h"

#ifdef __KERNEL__
EXPORT_SYMBOL(fsl_shw_get_random);
#endif

/* REQ-S2LRD-PINTFC-API-BASIC-RNG-002 */
/*!
 * Get a random number
 *
 *
 * @param    user_ctx
 * @param    length
 * @param    data
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t fsl_shw_get_random(
                                fsl_shw_uco_t* user_ctx,
                                uint32_t length,
                                uint8_t* data)
{
    fsl_shw_return_t ret;
    sah_Head_Desc* desc_chain = NULL;
    uint32_t header = SAH_HDR_RNG_GENERATE; /* Desc. #18 */


    /* perform a sanity check on the uco */
    ret = sah_validate_uco(user_ctx);

    if (ret == FSL_RETURN_OK_S) {
        /* create descriptor #18. Generate random data */
        ret = add_two_out_desc(header, data, length, NULL, 0,
                               user_ctx->mem_util, &desc_chain);

        if (ret == FSL_RETURN_OK_S) {
            ret = sah_Descriptor_Chain_Execute(desc_chain, user_ctx);
        }
    }

    return ret;
}
