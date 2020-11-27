/*
 * include/asm-i386/dpm-centrino.h    DPM defines for Intel Centrino
 *
 * 2003 (c) MontaVista Software, Inc.  This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __ASM_DPM_CENTRINO_H__
#define __ASM_DPM_CENTRINO_H__

/* MD operating point parameters */
#define DPM_MD_CPU_FREQ		0  /* CPU freq */
#define DPM_MD_V		1  /* core voltage */

#define DPM_PP_NBR 2

#define DPM_PARAM_NAMES \
{ "cpu", "v" }

/* Instances of this structure define valid Innovator operating points for DPM.
   Voltages are represented in mV, and frequencies are represented in KHz. */

struct dpm_md_opt {
        unsigned int v;         /* Target voltage in mV */
	unsigned int cpu;	/* CPU frequency in KHz */
};

#endif /* __ASM_DPM_CENTRINO_H__ */
