/*
 * Copyright © 2008-2009, Motorola, All Rights Reserved.
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
 * Motorola 2008-Jan-29 - Initial Creation for xPIXL
 * Motorola 2008-Mar-11 - Added MORPHING_MODE_PHONE_WITHOUT_REVIEW
 * Motorola 2009-Jan-20 - Added initial & end bounds for the enum.
 * Motorola 2009-Feb-11 - Update Copyright.
 */

#ifndef __MORPHING_MODE_H__
#define __MORPHING_MODE_H__

/******************************************************************************
* Typedefs/Enumerations
******************************************************************************/

/*
 * Morphing modes
*/
typedef enum {
    // Any new values should be added only between MORPHING_MODE_BEGIN
    // and MORPHING_MODE_END 
    MORPHING_MODE_BEGIN = -2,
    MORPHING_MODE_KEEP_CURRENT = -1,
    MORPHING_MODE_STANDBY = 0,
    MORPHING_MODE_NAVIGATION,
    MORPHING_MODE_PHONE,
    MORPHING_MODE_PHONE_WITHOUT_REVIEW,
    MORPHING_MODE_STILL_VIDEO_CAPTURE,
    MORPHING_MODE_STILL_VIDEO_PLAYBACK,
    MORPHING_MODE_STILL_VIDEO_PLAYBACK_WITHOUT_SHARE,
    MORPHING_MODE_STILL_VIDEO_PLAYBACK_WITHOUT_TRASH,
    MORPHING_MODE_STILL_VIDEO_PLAYBACK_WITHOUT_TOGGLE,
    MORPHING_MODE_SHARE,
    MORPHING_MODE_TRASH,
    MORPHING_MODE_NUM,
    MORPHING_MODE_TEST,
    MORPHING_MODE_END
}MORPHING_MODE_E;

#endif /* __MORPHING_MODE_H__ */
