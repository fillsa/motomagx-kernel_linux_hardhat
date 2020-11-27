/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file mxc_rnga.c
 *
 * @brief This file allows the user to use the API's for configuring
 * seeding and generation of random numbers.
 *
 * @ingroup MXC_Security
 */

/*
 * Module header file
 */
#include "mxc_rnga.h"
#include <linux/module.h>
#include <linux/device.h>
#include <asm/io.h>

/*!
 * This variable indicates whether RNGA driver is in suspend or in resume
 * mode.
 */
static unsigned short rnga_suspend_state = 0;

/*!
 * This API configures the RNGA module for interrupt enable or disable. It
 * configures the module to be configured for High assurance boot (HAB).
 * Enabling HAB will inform the SCC in case of any Security violation.
 *
 * @param   rnga_cfg   configuration value for RNGA.
 *
 * @return  RNGA_SUCCESS  Successfully configured the RNGA module.\n
 *          RNGA_FAILURE  Invalid parameter passed.
 */
rnga_ret rnga_configure(rnga_config rnga_cfg)
{
	int config_chk;
	ulong ctrl_reg;
	ctrl_reg = __raw_readl(RNGA_CONTROL);

	if (rnga_suspend_state == 1) {
		RNGA_DEBUG("RNGA Module: RNGA Module is in suspend mode.\n");
		return RNGA_FAILURE;
	}
	RNGA_DEBUG("In function %s. RNGA CFG=0x%02X\n", __FUNCTION__, rnga_cfg);
	config_chk = RNGA_HIGH_ASSUR | RNGA_INTR_MSK;
	if ((config_chk & rnga_cfg) == 0) {
		return RNGA_FAILURE;
	}

	ctrl_reg |= rnga_cfg;
	__raw_writel(ctrl_reg, RNGA_CONTROL);
	return RNGA_SUCCESS;
}

/*!
 * This API returns the status of the RNGA module.
 *
 * @return   Status of the RNGA module.
 */
ulong rnga_get_status(void)
{
	ulong sts_reg;
	sts_reg = __raw_readl(RNGA_STATUS);
	RNGA_DEBUG("In function %s. Status=0x%08lX\n", __FUNCTION__, sts_reg);

	return sts_reg;
}

/*!
 * This API returns the RNGA Mode register.
 *
 * @return   Mode register of the RNGA module.
 */
ulong rnga_get_mode(void)
{
        ulong mode_reg;
        mode_reg = __raw_readl(RNGA_MODE);
        RNGA_DEBUG("In function %s. Mode=0x%08lX\n", __FUNCTION__, mode_reg);
        return mode_reg;
}

/*!
 * This API returns the Control register of the RNGA module.
 *
 * @return   Control register of the RNGA module.
 */
ulong rnga_get_control(void)
{
        ulong ctrl_reg;
        ctrl_reg = __raw_readl(RNGA_CONTROL);
        RNGA_DEBUG("In function %s. Control=0x%08lX\n", __FUNCTION__, ctrl_reg);
        return ctrl_reg;
}

/*!
 * This API returns the total count value of the random stored in the RNGA FIFO
 *
 * @return  Total count value of random numbers stored in the RNGA FIFO.
 */
unchar rnga_get_fifo_level(void)
{
	unchar rnga_fifo_lvl;
	ulong temp_rnga_sts;

	temp_rnga_sts = rnga_get_status();
	RNGA_DEBUG("In function %s. Status=0x%08lX\n",
		   __FUNCTION__, temp_rnga_sts);
        rnga_fifo_lvl = ((temp_rnga_sts >> RNGA_STATUS_FIFO_LVL_OFFSET)
                        & RNGA_FIFO_LVL_MASK);
	RNGA_DEBUG("In function %s. Fifo level=0x%02X\n",
		   __FUNCTION__, rnga_fifo_lvl);

	return rnga_fifo_lvl;
}

/*!
 * Seeds the Random number generator with the new seed value.
 *
 * @param  entropy_val seed for the random number generator.
 */
rnga_ret rnga_seed_rand(ulong entropy_val)
{
	ulong entropy;
	entropy = __raw_readl(RNGA_ENTROPY);
	if (rnga_suspend_state == 1) {
		RNGA_DEBUG("RNGA Module: RNGA Module is in suspend mode.\n");
		return RNGA_FAILURE;
	}
	RNGA_DEBUG("In function %s. RNGA entropy=0x%08lX\n",
		   __FUNCTION__, entropy_val);
	entropy = entropy_val;
	__raw_writel(entropy, RNGA_ENTROPY);
	return RNGA_SUCCESS;
}

/*!
 * This API gets the random number from the FIFO. If RNGA is in power down mode
 * (Sleep mode), no random numbers are generated and return RNGA_FAILURE.
 *
 * @param   rand     Random number generated is stored in this variable.
 *
 * @return  Zero for success and RNGA_FAILURE for failure.
 *
 */
rnga_ret rnga_get_rand(ulong * rand)
{
	ulong sleep, osc_dead, ctrl_reg, sts_reg, fifo;
	rnga_ret ret_val = RNGA_SUCCESS;
	ctrl_reg = __raw_readl(RNGA_CONTROL);
	sts_reg = __raw_readl(RNGA_STATUS);

	if (rnga_suspend_state == 1) {
		RNGA_DEBUG("RNGA Module: RNGA Module is in suspend mode.\n");
		ret_val = RNGA_FAILURE;
		return ret_val;
	}

	RNGA_DEBUG("In function %s. RNGA Control = 0x%08lX\n",
		   __FUNCTION__, ctrl_reg);
	sleep = ((sts_reg & RNGA_SLEEP) != 0);
        osc_dead = ((sts_reg & RNGA_OSCDEAD) != 0);

	if ((!sleep) || (!osc_dead) || (rnga_get_fifo_level() > 0x00)) {
		if ((!sleep) & (!osc_dead)) {
			/* If not in sleep mode, random numbers are
			 * generated. */
			RNGA_DEBUG("Value of control reg sec 0x%08lX\n",
				   ctrl_reg);
			RNGA_DEBUG("Value of control reg trd 0x%08lX\n",
				   ctrl_reg);
			RNGA_DEBUG("Value of status reg trd 0x%08lX\n",
				   sts_reg);
			if ((ctrl_reg & RNGA_GO) != RNGA_START_RAND) {
				ctrl_reg |= RNGA_GEN_RAND;
				__raw_writel(ctrl_reg, RNGA_CONTROL);
				RNGA_DEBUG("In function %s. RNGA Control "
					   "after setting GO bit = 0x%08lX\n",
					   __FUNCTION__, ctrl_reg);
			}
		}

		/* Checking if there are any bytes in the FIFO */
		while (rnga_get_fifo_level() == 0x00) {
			/* Do Nothing */
		}
	        fifo = __raw_readl(RNGA_FIFO);
		*rand = fifo;
		RNGA_DEBUG("In function %s. RNGA Fifo = 0x%08lX\n",
			   __FUNCTION__, *rand);
	} else {
		RNGA_DEBUG("In function %s. Currently in sleep mode. RNGA "
			   "Control = 0x%08lX\n", __FUNCTION__, ctrl_reg);
		/* RNGA in Sleep mode so returning -EPERM */
		ret_val = RNGA_FAILURE;
	}
	return ret_val;
}

/*!
 * This API clears the RNGA interrupt
 */
rnga_ret rnga_clear_int(void)
{
	ulong ctrl_reg;
	ctrl_reg = __raw_readl(RNGA_CONTROL);
	if (rnga_suspend_state == 1) {
		RNGA_DEBUG("RNGA Module: RNGA Module is in suspend mode.\n");
		return RNGA_FAILURE;
	}
	ctrl_reg |= RNGA_CLR_INTR;
	__raw_writel(ctrl_reg, RNGA_CONTROL);
	ctrl_reg = __raw_readl(RNGA_CONTROL);
	RNGA_DEBUG("In function %s. RNGA Control=0x%08lX\n",
		   __FUNCTION__, ctrl_reg);
	return RNGA_SUCCESS;
}

/*!
 * This API returns the FIFO size of the RNGA
 *
 * @return  FIFO size of the RNGA module.
 */
char rnga_get_fifo_size(void)
{
	unchar rnga_fifo_size;
	ulong temp_rnga_sts, sts_reg;
	sts_reg = __raw_readl(RNGA_STATUS);
	if (rnga_suspend_state == 1) {
		RNGA_DEBUG("RNGA Module: RNGA Module is in suspend mode.\n");
		return -EPERM;
	}

	RNGA_DEBUG("In function %s. Status=0x%08lX\n", __FUNCTION__, sts_reg);
	temp_rnga_sts = rnga_get_status();
        rnga_fifo_size = ((temp_rnga_sts >> RNGA_STATUS_FIFO_SIZE_OFFSET)
                         & RNGA_FIFO_SIZE_MASK);
	RNGA_DEBUG("In function %s. Fifo size=0x%02X\n",
		   __FUNCTION__, rnga_fifo_size);
	return rnga_fifo_size;
}

/*!
 * This API is used to set the mode of operation of RNGA module to either
 * Verification mode or oscillator frequency test mode.
 *
 * @param        rngamode  mode in which RNGA needs to be operated.
 *
 * @return   RNGA_SUCCESS  Successfully configured RNGA mode.
 *           RNGA_FAILURE  Invalid parameter passed.
 */
rnga_ret rnga_configure_mode(rnga_mode rngamode)
{
	int mode_chk;
	ulong mode;
	mode = __raw_readl(RNGA_MODE);
	if (rnga_suspend_state == 1) {
		RNGA_DEBUG("RNGA Module: RNGA Module is in suspend mode.\n");
		return RNGA_FAILURE;
	}
	RNGA_DEBUG("In function %s. rnga_mode=0x%02X\n",
		   __FUNCTION__, rngamode);
	mode_chk = RNGA_VRFY_MODE | RNGA_OSC_FREQ_TEST;
	if ((mode_chk & rngamode) == 0) {
		return RNGA_FAILURE;
	}
	/* Sets the mode of operation of RNGA Module. The mode of operation
	   can be either verification mode or oscillator frequency test mode. */
        __raw_writel(0x00, RNGA_MODE);
	mode = __raw_readl(RNGA_MODE);
	mode |= rngamode;
	__raw_writel(mode, RNGA_MODE);
	mode = __raw_readl(RNGA_MODE);
	RNGA_DEBUG("In function %s. Mode=0x%08lX\n", __FUNCTION__, mode);
	return RNGA_SUCCESS;
}

/*!
 * This API brings the RNGA module to normal state from power down mode
 * (Sleep mode). This is used in scenario where the RNGA module is been put to
 * sleep mode and needs to be brought back to normal mode.
 *
 */
rnga_ret rnga_normal(void)
{
	ulong ctrl_reg, sts_reg;
	ctrl_reg = __raw_readl(RNGA_CONTROL);
	sts_reg = __raw_readl(RNGA_STATUS);
        __raw_writel(0x00, RNGA_MODE);
	if (rnga_suspend_state == 1) {
		RNGA_DEBUG("RNGA Module: RNGA Module is in suspend mode.\n");
		return RNGA_FAILURE;
	}
	if ((sts_reg & RNGA_SLEEP) != 0) {
		ctrl_reg &= (~(RNGA_SLEEP));
		__raw_writel(ctrl_reg, RNGA_CONTROL);
	}
	RNGA_DEBUG("In function %s. RNGA Status = 0x%08lX\n",
		   __FUNCTION__, sts_reg);
	return RNGA_SUCCESS;
}

/*!
 * This API puts the RNGA module to sleep mode. After placing the RNGA module
 * to sleep mode (low power mode), it stops generating the random numbers.
 */
rnga_ret rnga_sleep(void)
{
	ulong ctrl_reg;
	ctrl_reg = __raw_readl(RNGA_CONTROL);
	if (rnga_suspend_state == 1) {
		RNGA_DEBUG("RNGA Module: RNGA Module is in suspend mode.\n");
		return RNGA_FAILURE;
	}
	ctrl_reg |= RNGA_SLEEP;
	__raw_writel(ctrl_reg, RNGA_CONTROL);
	RNGA_DEBUG("In function %s. RNGA Control=0x%08lX\n",
		   __FUNCTION__, ctrl_reg);
	RNGA_DEBUG("Value of control reg sleep 0x%08lX\n", ctrl_reg);
	return RNGA_SUCCESS;
}

/*!
 * This API is used to control verification clocks. It is used reset the shift
 * registers, use system clock and to turn on the shift clock.
 *
 * @param        rnga_vrfy_control configures the RNGA verification controller.
 *
 * @return   RNGA_SUCCESS  Successfully configured the RNGA verification
 *                         controller.
 *           RNGA_FAILURE  Parameter passed has invalid data.
 */
rnga_ret rnga_config_vrfy_ctrl(rnga_vrfy_ctrl rnga_vrfy_control)
{
	int vrfy_chk;
	ulong verify_ctrl;
	verify_ctrl = __raw_readl(RNGA_VRFY_CTRL);

	RNGA_DEBUG("In function %s. rnga_vrfy_ctrl=0x%08X\n",
		   __FUNCTION__, rnga_vrfy_control);
	vrfy_chk = RNGA_SHFT_CLK_ON | RNGA_SYS_CLK_ON | RNGA_RESET_SHFT_REG;
	/* Checking if any invalid parameter is been passed. */
	if ((vrfy_chk & rnga_vrfy_control) == 0) {
		return RNGA_FAILURE;
	}
        __raw_writel(0x00, RNGA_VRFY_CTRL);
	verify_ctrl |= rnga_vrfy_control;
	__raw_writel(verify_ctrl, RNGA_VRFY_CTRL);
	verify_ctrl = __raw_readl(RNGA_VRFY_CTRL);
	RNGA_DEBUG("In function %s. RNGA Verify Control=0x%08lX\n",
		   __FUNCTION__, verify_ctrl);

	return RNGA_SUCCESS;
}

/*!
 * This API is used for the oscillator test logic to specify the length of
 * the count oscillator clock pulses. The oscillator count value read is stored
 * in pointer variable osc_count_ptr.
 *
 * @param       osc_count_ptr   counter value of oscillator.
 * @param       rnga_rw         read/write data from/to oscillator counter.
 *
 * @return   RNGA_SUCCESS  Successfully preformed the operation.
 *           RNGA_FAILURE  Invalid parameter passed.
 */
rnga_ret rnga_config_osc_cntctrl(ulong * osc_count_ptr, rnga_readwrite rnga_rw)
{
	rnga_ret ret_val = RNGA_SUCCESS;
	ulong osccnt;
	osccnt = __raw_readl(RNGA_OSC_CTRL_CNT);

	RNGA_DEBUG("In function %s. osc_count_ptr=0x%08lX rnga_mode=%d\n",
		   __FUNCTION__, (ulong) osc_count_ptr, rnga_rw);
	/* Checking if any invalid parameter is been passed. */
	if ((osc_count_ptr == 0) || (*osc_count_ptr > RNGA_MAX_OSC_COUNT)) {
		ret_val = RNGA_FAILURE;
	} else {
		switch (rnga_rw) {
		case RNGA_WRITE:
			osccnt = *osc_count_ptr;
			__raw_writel(osccnt, RNGA_OSC_CTRL_CNT);
			RNGA_DEBUG("In function %s. OSC_CTRL_CNT = 0x%08lX\n",
				   __FUNCTION__, osccnt);
			break;

		case RNGA_READ:
			*osc_count_ptr = osccnt;
			RNGA_DEBUG("In function %s. *osc_count_ptr = "
				   "0x%08lX\n", __FUNCTION__, *osc_count_ptr);
			break;

		default:
			ret_val = RNGA_FAILURE;
			break;
		}
	}
	return ret_val;
}

/*!
 * This API is used to count the number of oscillator pulses received from
 * Oscillator #1/#2 starting from the time the Oscillator Counter Control
 * Register is written and ending at the time the Oscillator Counter Control
 * Register reaches zero. If the oscillator number is other than that specified,
 * it will return 0.
 *
 * @param       osc_num         oscillator number.
 *
 * @return      Count stored in the oscillator specified.
 */
ulong rnga_osc_cnt(rnga_osc_num osc_num)
{
	ulong osc_count, osc1cnt, osc2cnt;
	osc1cnt = __raw_readl(RNGA_OSC1_CNT);
	osc2cnt = __raw_readl(RNGA_OSC2_CNT);

	RNGA_DEBUG("In function %s. osc_num = 0x%02X\n", __FUNCTION__, osc_num);

	switch (osc_num) {
	case RNGA_OSCILLATOR1:
		osc_count = osc1cnt;
		break;

	case RNGA_OSCILLATOR2:
		osc_count = osc2cnt;
		break;

	default:
		osc_count = 0;
		RNGA_DEBUG("In function %s. In default.\n", __FUNCTION__);
		break;
	}

	RNGA_DEBUG("In function %s. osc_count = 0x%08lX\n",
		   __FUNCTION__, osc_count);

	return osc_count;
}

/*!
 * This API gives the oscillator counter which have toggled 0x400 times. If bit
 * 0 is set then oscillator #1 and if bit 1 is set the oscillator #2 has crossed
 * 0x400 times. If both the bits are set then both the oscillators have crossed
 * 0x400 times.
 *
 * @return      Oscillators that have toggled 0x400 times.
 */
ulong rnga_osc_status(void)
{
	ulong osc_sts, rnga_osc;
	osc_sts = __raw_readl(RNGA_OSC_STATUS);
	rnga_osc = (osc_sts & (RNGA_OSC1 | RNGA_OSC2));

	RNGA_DEBUG("In function %s. RNGA_OSC_STATUS = 0x%08lX\n",
		   __FUNCTION__, rnga_osc);

	return rnga_osc;
}

#ifdef CONFIG_PM
/*!
 * This function is called to put the RNGA in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   dev   the device structure used to give information on RNGA
 *                to suspend.
 * @param   state the power state the device is entering.
 * @param   level the stage in device suspension process that we want the
 *                device to be put in.
 *
 * @return  The function always returns 0.
 */
rnga_ret rnga_suspend(struct device * dev, ulong state, ulong level)
{
	rnga_ret ret_val = RNGA_SUCCESS;
	ulong ctrl_reg;
	ctrl_reg = __raw_readl(RNGA_CONTROL);
	printk("RNGA Module: RNGA suspend function.%08lX\n", level);
	switch (level) {
	case SUSPEND_DISABLE:
		RNGA_DEBUG("RNGA Module: In suspend Disable\n");
		/* Setting suspend flag. */
		rnga_suspend_state = 1;
		break;

	case SUSPEND_SAVE_STATE:
		RNGA_DEBUG("RNGA Module: In Suspend save state.\n");
		/* Do nothing. */
		break;

	case SUSPEND_POWER_DOWN:
		RNGA_DEBUG("RNGA Module: In suspend power down.\n");
		/* Go to sleep mode. */
                rnga_sleep();
		break;

	default:
		ret_val = RNGA_FAILURE;
		break;
	}
	return ret_val;
}

/*!
 * This function is called to bring the RNGA back from a low power state.
 * Refer to the document driver-model/driver.txt in the kernel source tree
 * for more information.
 *
 * @param   dev   the device structure used to give information on RNGA
 *                to resume.
 * @param   level the stage in device resumption process that we want the
 *                device to be put in.
 *
 * @return  The function always returns 0.
 */
rnga_ret rnga_resume(struct device * dev, ulong level)
{
	rnga_ret ret_val = RNGA_SUCCESS;
	ulong ctrl_reg;
	ctrl_reg = __raw_readl(RNGA_CONTROL);
	printk("RNGA Module: RNGA resume function.%08lX\n", level);
	switch (level) {
	case RESUME_POWER_ON:
		RNGA_DEBUG("RNGA Module: Resume power on.\n");
		/* Bring RNGA to normal mode. */
                rnga_normal();
		break;
	case RESUME_RESTORE_STATE:
		RNGA_DEBUG("RNGA Module: Resume restore state. \n");
		/* Do Nothing. */
		break;

	case RESUME_ENABLE:
		RNGA_DEBUG("RNGA Module: Resume Enable state called.\n");
		/* Clearing the suspend flag. */
		rnga_suspend_state = 0;
		break;

	default:
		ret_val = RNGA_FAILURE;
		break;
	}
	return ret_val;
}
#else
#define mxc_rnga_suspend NULL
#define mxc_rnga_resume  NULL
#endif				/* CONFIG_PM */
