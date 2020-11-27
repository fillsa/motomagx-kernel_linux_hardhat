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
 * @file fsl_platform.h
 *
 * Header file to isolate code which might be platform-dependent
 *
 * @ingroup MXCSAHARA2
 */

#ifndef FSL_PLATFORM_H
#define FSL_PLATFORM_H

#ifdef __KERNEL__
#include "portable_os.h"
#endif

#if 0


#ifdef SAHARA2


#else

#error UNKNOWN_PLATFORM

#endif /* platform checks */

#endif

#endif /* FSL_PLATFORM_H */
