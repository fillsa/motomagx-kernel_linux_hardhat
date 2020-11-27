/** ********************************************************************************
 ** ********************************************************************************
 ** fx2lp-tcd.c -  TCD APIs for the OTG stack
 **
 ** Portability:
 **     This file contains H/W specific code. Partially portable
 **
 ** ********************************************************************************
 ** ********************************************************************************/

/*
 *  Copyright (c) 2006-2007, Motorola, All Rights Reserved.
 *
 *  This program is licensed under a BSD license with the following terms:
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are 
 *  met:
 *
 *  Redistributions of source code must retain the above copyright notice, 
 *  this list of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright notice, 
 *  this list of conditions and the following disclaimer in the documentation 
 *  and/or other materials provided with the distribution.
 *
 *  Neither the name of Motorola nor the names of its contributors may be 
 *  used to endorse or promote products derived from this software without 
 *  specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE 
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * Dec  29, 2006       Motorola, Inc.   Initial create
 * Jul  26, 2007       Motorola, Inc.   Changes for open source compliance.
 * Aug  24, 2007       Motorola, Inc.   Changes for open source compliance.
 */

#include <otg/pcd-include.h>

void fx2lp_tcd_en(struct otg_instance *otg, u8 flag)
{
       u64 inputs = VBUS_VLD | B_SESS_VLD | B_SESS_END_ | ID_GND_ | ID_FLOAT | SE1_DET_ | DP_HIGH_ | DM_HIGH_ ;
       switch (flag) {
        case SET:
        case PULSE:
                TRACE_MSG0(TCD, "SET/PULSE");
                otg_event(tcd_instance->otg, inputs, TCD, "FX2 OTG EVENT");
                break;
        case RESET:
                TRACE_MSG0(TCD, "RESET");
                break;
        }
}

struct tcd_ops tcd_ops = {
        .tcd_en_func = fx2lp_tcd_en,
        .chrg_vbus_func = NULL,
        .drv_vbus_func = NULL,
        .dischrg_vbus_func = NULL,
        .dp_pullup_func = NULL,
        .dm_pullup_func = NULL,
        .dp_pulldown_func = NULL,
        .dm_pulldown_func = NULL,
        .overcurrent_func = NULL,
        .dm_det_func = NULL,
        .dp_det_func = NULL,
        .cr_det_func = NULL,
        .peripheral_host_func = NULL,
        .mx21_vbus_drain_func = NULL,
        .id_pulldown_func = NULL,
        .audio_func = NULL,
        .uart_func = NULL,
        .mono_func = NULL,
        .mod_init = NULL,
        .mod_exit =NULL,
};
