/*
 * Copyright (c) 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 */

/*!
 * @file sc55112_fixarb.c
 * @brief Flips the sc55112 arbitration bits to allow register read/write
 *        access for the secondary processor.
 *
 * Note that the initialisation function defined below must be called prior
 * to any of the other sc55112 modules. We guarantee this by putting a call
 * to sc55112_fix_arbitration() in the initialization routine of sc55112_core.c.
 *
 * Also note that the sc55112_fix_arbitration() routine requires that CSPI2 be
 * connected to the sc55112's primary processor SPI bus. This currently
 * requires a hardware board modification to the RF Deck of the Sphinx board.
 * Specifically, the following 4 resistors must be removed in order to
 * disconnect the MQSPI:
 *
 *     R151, R154, R156, and R157
 *
 * Next, the following connections must be made to connect CSPI2 to the
 * sc55112's primary processor SPI bus:
 *
 *     P6077 to R151 radio board side pad
 *     etc.
 *
 * @ingroup PMIC_SC55112_CORE
 *
 */

#include <linux/kernel.h>
#include "sc55112_register.h"
#include "sc55112_config.h"
#include "../../spi/spi.h"
#include "asm/arch/pmic_status.h"

static PMIC_STATUS init_cspi2_access(void);
static PMIC_STATUS fix_irqs(void);
static PMIC_STATUS fix_connectivity(void);
static PMIC_STATUS fix_audio(void);
static PMIC_STATUS fix_reg_access(void);
static PMIC_STATUS fix_switcher(void);

static PMIC_STATUS sc55112_cspi2_read_reg(t_prio priority, int reg,
					  unsigned int *reg_value);
static PMIC_STATUS sc55112_cspi2_write_reg(t_prio priority, int reg,
					   unsigned int reg_value,
					   unsigned int reg_mask);
static PMIC_STATUS register_access_wait(t_prio priority);
static PMIC_STATUS unlock_client(void);
static PMIC_STATUS sc55112_spi_send_frame(int num_reg, unsigned int *reg_value,
					  bool _rw);

static int pending_client_count[MAX_PRIO_QUEUE];
static wait_queue_head_t sc55112_reg_queue[MAX_PRIO_QUEUE];

static t_sc55112_status sc55112_status;
static spi_config the_config;
static void *spi_id;

/**************************************************************************
 * Module initialization and termination functions.
 *
 * Note that if this code is compiled into the kernel, then the
 * module_init() function will be called within the device_initcall()
 * group.
 **************************************************************************
 */

/*
 * Initialization
 */
PMIC_STATUS sc55112_fix_arbitration(void)
{
	PMIC_STATUS rval = PMIC_SUCCESS;
	TRACEMSG_ALL_TIME(KERN_INFO "sc55112 changing arbitration bits "
			  "to allow secondary processor write access.\n");

	/* First, configure access using CSPI2. */
	init_cspi2_access();

	/* Next, make sure that interrupts are routed to the secondary
	 * processor.
	 */
	CHECK_ERROR(fix_irqs());

	/* Now change the arbitration bits on various sc55112 registers
	 * as required to allow write access from the secondary processor.
	 */
	CHECK_ERROR(fix_connectivity());
	CHECK_ERROR(fix_audio());
	CHECK_ERROR(fix_reg_access());
	CHECK_ERROR(fix_switcher());

	TRACEMSG_ALL_TIME(KERN_INFO "sc55112 completed reconfiguration "
			  "of arbitration bits\n");
	return rval;

}

static PMIC_STATUS fix_irqs(void)
{
	PMIC_STATUS rc = PMIC_SUCCESS;
	unsigned int reg_value;

	/* Check to make sure that the INT_SEL register has the expected
	 * reset value of 0xffffff.
	 */
	sc55112_cspi2_read_reg(PRIO_CORE, REG_INT_SEL, &reg_value);
	if (reg_value != 0xffffff) {
		/* The INT_SEL (interrupt select) register does not have the
		 * expected reset value, so let's manually set it.
		 */
		TRACEMSG_ALL_TIME(KERN_INFO "sc55112 changing INT_SEL from "
				  "0x%06x to 0xffffff\n", reg_value);

		sc55112_cspi2_write_reg(PRIO_CORE, REG_INT_SEL, 0xffffff,
					0xffffff);

		/* Now check that the INT_SEL register was successfully updated. */
		sc55112_cspi2_read_reg(PRIO_CORE, REG_INT_SEL, &reg_value);
		if (reg_value != 0xffffff) {
			TRACEMSG_ALL_TIME(KERN_INFO
					  "sc55112 failed to set INT_SEL "
					  "to 0xffffff, current value is 0x%06x\n",
					  reg_value);
			rc = PMIC_ERROR;
		}
	}

	return rc;
}

static PMIC_STATUS fix_connectivity(void)
{
	PMIC_STATUS rc = PMIC_SUCCESS;
	unsigned int reg_value;

	sc55112_cspi2_read_reg(PRIO_CONN, REG_BUSCTRL, &reg_value);
	if (!(reg_value & 0x800000)) {
		/* Need to set BUS_PRI_ADJ bit to allow secondary processor
		 * write access.
		 */
		sc55112_cspi2_write_reg(PRIO_CONN, REG_BUSCTRL, 0x800000,
					0x800000);

		/* Confirm that change was successful. */
		sc55112_cspi2_read_reg(PRIO_CONN, REG_BUSCTRL, &reg_value);
		if (!(reg_value & 0x800000)) {
			TRACEMSG_ALL_TIME(KERN_INFO "Failed to change BUSCTRL "
					  "arbitration bits (0x%06x)\n",
					  reg_value);
			rc = PMIC_ERROR;
		} else {
			TRACEMSG_ALL_TIME(KERN_INFO
					  "Changed BUS_PRI_ADJ bit in "
					  "BUSCTRL (0x%06x)\n", reg_value);
		}
	}
	return rc;
}

static PMIC_STATUS fix_audio(void)
{
	PMIC_STATUS rc = PMIC_SUCCESS;
	static const unsigned int AUD_CODEC_ARB = 0x800000;
	static const unsigned int RX_AUD_AMPS_ARB = 0x728000;
	static const unsigned int ST_DAC_ARB = 0xe00000;
	static const unsigned int TX_AUD_AMPS_ARB = 0xa83000;
	unsigned int reg_value;

	sc55112_cspi2_read_reg(PRIO_AUDIO, REG_AUD_CODEC, &reg_value);
	if ((reg_value & AUD_CODEC_ARB) != AUD_CODEC_ARB) {
		/* Need to set CDC_PRI_ADJ bit to allow secondary processor
		 * write access.
		 */
		sc55112_cspi2_write_reg(PRIO_AUDIO, REG_AUD_CODEC,
					AUD_CODEC_ARB, AUD_CODEC_ARB);

		/* Confirm that change was successful. */
		sc55112_cspi2_read_reg(PRIO_AUDIO, REG_AUD_CODEC, &reg_value);
		if ((reg_value & AUD_CODEC_ARB) != AUD_CODEC_ARB) {
			TRACEMSG_ALL_TIME(KERN_INFO
					  "Failed to change AUD_CODEC "
					  "arbitration bits (0x%06x)\n",
					  reg_value);
			rc = PMIC_ERROR;
		} else {
			TRACEMSG_ALL_TIME(KERN_INFO
					  "Changed CDC_PRI_ADJ bit in "
					  "AUD_CODEC (0x%06x)\n", reg_value);
		}
	}

	sc55112_cspi2_read_reg(PRIO_AUDIO, REG_RX_AUD_AMPS, &reg_value);
	if ((reg_value & RX_AUD_AMPS_ARB) != RX_AUD_AMPS_ARB) {
		/* Need to set RX_PRI_ADJ, MONO_PRI_ADJ, AUDOG_PRI_ADJ, and
		 * A2_ADJ_LOCK bits to allow secondary processor write access.
		 */
		sc55112_cspi2_write_reg(PRIO_AUDIO, REG_RX_AUD_AMPS,
					RX_AUD_AMPS_ARB, RX_AUD_AMPS_ARB);

		/* Confirm that change was successful. */
		sc55112_cspi2_read_reg(PRIO_AUDIO, REG_RX_AUD_AMPS, &reg_value);
		if ((reg_value & RX_AUD_AMPS_ARB) != RX_AUD_AMPS_ARB) {
			TRACEMSG_ALL_TIME(KERN_INFO
					  "Failed to change RX_AUD_AMPS "
					  "arbitration bits (0x%06x)\n",
					  reg_value);
			rc = PMIC_ERROR;
		} else {
			TRACEMSG_ALL_TIME(KERN_INFO
					  "Changed arbitration bits in "
					  "RX_AUD_AMPS (0x%06x)\n", reg_value);
		}
	}

	sc55112_cspi2_read_reg(PRIO_AUDIO, REG_ST_DAC, &reg_value);
	if ((reg_value & ST_DAC_ARB) != ST_DAC_ARB) {
		/* Need to set ST_DAC_PRI_ADJ, and AUD_MIX_PRI_ADJ bits to allow
		 * secondary processor write access.
		 */
		sc55112_cspi2_write_reg(PRIO_AUDIO, REG_ST_DAC, ST_DAC_ARB,
					ST_DAC_ARB);

		/* Confirm that change was successful. */
		sc55112_cspi2_read_reg(PRIO_AUDIO, REG_ST_DAC, &reg_value);
		if ((reg_value & ST_DAC_ARB) != ST_DAC_ARB) {
			TRACEMSG_ALL_TIME(KERN_INFO "Failed to change ST_DAC "
					  "arbitration bits (0x%06x)\n",
					  reg_value);
			rc = PMIC_ERROR;
		} else {
			TRACEMSG_ALL_TIME(KERN_INFO
					  "Changed arbitration bits in "
					  "ST_DAC (0x%06x)\n", reg_value);
		}
	}

	sc55112_cspi2_read_reg(PRIO_AUDIO, REG_TX_AUD_AMPS, &reg_value);
	if ((reg_value & TX_AUD_AMPS_ARB) != TX_AUD_AMPS_ARB) {
		/* Need to set TX_PRI_ADJ, AUDIG_PRI_ADJ, AUDIO_BIAS_ARB, and
		 * A1_Config bits to allow secondary processor write access.
		 */
		sc55112_cspi2_write_reg(PRIO_AUDIO, REG_TX_AUD_AMPS,
					TX_AUD_AMPS_ARB, TX_AUD_AMPS_ARB);

		/* Confirm that change was successful. */
		sc55112_cspi2_read_reg(PRIO_AUDIO, REG_TX_AUD_AMPS, &reg_value);
		if ((reg_value & TX_AUD_AMPS_ARB) != TX_AUD_AMPS_ARB) {
			TRACEMSG_ALL_TIME(KERN_INFO
					  "Failed to change TX_AUD_AMPS "
					  "arbitration bits (0x%06x)\n",
					  reg_value);
			rc = PMIC_ERROR;
		} else {
			TRACEMSG_ALL_TIME(KERN_INFO
					  "Changed arbitration bits in "
					  "TX_AUD_AMPS (0x%06x)\n", reg_value);
		}
	}

	return rc;
}

static PMIC_STATUS fix_reg_access(void)
{
	const unsigned int ARB_REG_VALUE = 0x7836ef;
	const unsigned int ARB_REG_MASK = 0xffffff;
	PMIC_STATUS rc = PMIC_SUCCESS;
	unsigned int reg_value;

	sc55112_cspi2_read_reg(PRIO_CORE, REG_ARB_REG, &reg_value);

	if (reg_value != ARB_REG_VALUE) {
		/* Need to set bits in ARB_REG bit to allow secondary processor
		 * write access.
		 */
		sc55112_cspi2_write_reg(PRIO_CORE, REG_ARB_REG, ARB_REG_VALUE,
					ARB_REG_MASK);

		/* Confirm that change was successful. */
		sc55112_cspi2_read_reg(PRIO_CORE, REG_ARB_REG, &reg_value);
		if (reg_value != ARB_REG_VALUE) {
			TRACEMSG_ALL_TIME(KERN_INFO "Failed to change ARB_REG "
					  "(0x%06x)\n", reg_value);
			rc = PMIC_ERROR;
		} else {
			TRACEMSG_ALL_TIME(KERN_INFO
					  "Changed ARB_REG to (0x%06x)\n",
					  reg_value);
		}
	} else {
		TRACEMSG_ALL_TIME(KERN_INFO "ARB_REG is (0x%06x)\n", reg_value);
	}

	return rc;
}

static PMIC_STATUS fix_switcher(void)
{
	/*
	 * Write SWCTRL to do:
	 *   1.0 V in standby mode   (SW10-SW12 STANDBY setting)
	 *   1.2 V in normal mode    (SW10-SW12 setting)
	 *   1.6 V in overdrive mode (SW10-SW12 LVS setting)
	 *
	 * Note that I had to clear bit 8 of the ARB_REG to get this
	 * to work.  Otherwise, some of the register settings I needed
	 * in SW_CTRL were under secondary control and others under
	 * primary control.  Specifically, bits 4,3,and 2 of SW_CTRL
	 * couldnt be changed by primary SPI without me clearing
	 * that bit in the ARB_REG.
	 */

	const unsigned int SW_CTRL_REG_VALUE = 0x0000013B;
	const unsigned int SW_CTRL_REG_MASK = 0x003803BF;

	PMIC_STATUS rc = PMIC_SUCCESS;
	unsigned int reg_value, desired_value, check_reg;

	sc55112_cspi2_read_reg(PRIO_CORE, REG_SWCTRL, &reg_value);

	desired_value = (reg_value & ~SW_CTRL_REG_MASK) | SW_CTRL_REG_VALUE;

	if (reg_value != desired_value) {
		sc55112_cspi2_write_reg(PRIO_PWM, REG_SWCTRL, SW_CTRL_REG_VALUE,
					SW_CTRL_REG_MASK);

		/* Confirm that change was successful. */
		sc55112_cspi2_read_reg(PRIO_PWM, REG_SWCTRL, &check_reg);
		if (check_reg != desired_value) {
			TRACEMSG_ALL_TIME(KERN_INFO
					  "Failed to change SW_CTRL was 0x%06x; "
					  "wanted 0x%06x; got 0x%06x.\n",
					  reg_value, desired_value, check_reg);
			rc = PMIC_ERROR;
		} else {
			TRACEMSG_ALL_TIME(KERN_INFO
					  "Changed SW_CTRL to (0x%06x)\n",
					  check_reg);
		}
	} else {
		TRACEMSG_ALL_TIME(KERN_INFO "SW_CTRL is (0x%06x)\n", reg_value);
	}

	return rc;
}

static PMIC_STATUS init_cspi2_access(void)
{
	/* initialize all semaphore for all clients */
	int i = 0;

	for (i = 0; i < MAX_PRIO_QUEUE; i++) {
		pending_client_count[i] = 0;
		init_waitqueue_head(&(sc55112_reg_queue[i]));
	}

	sc55112_status = sc55112_FREE;

	/* initialize SPI */
	the_config.module_number = SPI2;
	the_config.priority = HIGH;
	the_config.master_mode = true;
	the_config.bit_rate = 4000000;
	the_config.bit_count = 32;
	the_config.active_high_polarity = true;
	the_config.active_high_ss_polarity = false;
	the_config.phase = false;
	the_config.ss_low_between_bursts = true;
	the_config.ss_asserted = SS_0;
	the_config.tx_delay = 10;
	spi_id = spi_get_device_id(&the_config);

	return PMIC_SUCCESS;
}

/*!
 * This function reads a register.
 *
 * @param        priority   priority of access
 * @param        reg        register.
 * @param        reg_value   register value.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
static PMIC_STATUS sc55112_cspi2_read_reg(t_prio priority, int reg,
					  unsigned int *reg_value)
{
	if (sc55112_status == sc55112_BUSY) {
		CHECK_ERROR(register_access_wait(priority));
	}

	TRACEMSG(_K_D("read reg = %d"), reg);
	CHECK_ERROR(sc55112_spi_send_frame(reg, reg_value, false));
	TRACEMSG(_K_D("read register done, read value = 0x%x"), *reg_value);
	CHECK_ERROR(unlock_client());

	return PMIC_SUCCESS;
}

/*!
 * This function writes a register.
 *
 * @param        priority   priority of access
 * @param        reg        register.
 * @param        reg_value  register value.
 * @param        reg_mask   bitmap mask indicating which bits to be modified.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
static PMIC_STATUS sc55112_cspi2_write_reg(t_prio priority, int reg,
					   unsigned int reg_value,
					   unsigned int reg_mask)
{
	unsigned int temp;

	if (sc55112_status == sc55112_BUSY) {
		CHECK_ERROR(register_access_wait(priority));
	}

	TRACEMSG(_K_D("write reg = %d"), reg);
	if ((reg_mask & PMIC_ALL_BITS) ^ PMIC_ALL_BITS) {
		CHECK_ERROR(sc55112_spi_send_frame(reg, &temp, false));
		temp &= ~reg_mask;
		temp |= (reg_value & reg_mask);
	} else {
		temp = reg_value;
	}

	CHECK_ERROR(sc55112_spi_send_frame(reg, &temp, true));

	TRACEMSG(_K_D("write register done"));
	CHECK_ERROR(unlock_client());

	return PMIC_SUCCESS;
}

/*!
 * This function sleeps for R/W access if the spi is busy.
 *
 * @param        priority   priority of access
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
static PMIC_STATUS register_access_wait(t_prio priority)
{
	DEFINE_WAIT(wait);

	if (pending_client_count[priority] < MAX_CLIENTS) {
		TRACEMSG(_K_D("sleep register access prio %d ...."), priority);

		pending_client_count[priority]++;
		prepare_to_wait(&(sc55112_reg_queue[priority]), &wait,
				TASK_INTERRUPTIBLE);
		schedule();
		sc55112_status = sc55112_BUSY;
		finish_wait(&(sc55112_reg_queue[priority]), &wait);

		TRACEMSG(_K_D("wake up register access prio %d ...."),
			 priority);
	} else {
		return PMIC_CLIENT_NBOVERFLOW;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function unlocks on locked register access.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
static PMIC_STATUS unlock_client(void)
{
	int i;
	bool queue_date = false;

	for (i = 0; i < MAX_PRIO_QUEUE; i++) {
		if (pending_client_count[i] > 0) {
			queue_date = true;
			TRACEMSG(_K_D("unlock register prio %d"), i);
			pending_client_count[i]--;
			wake_up_interruptible(&(sc55112_reg_queue[i]));
			break;
		}
	}

	if (queue_date == false) {
		TRACEMSG(_K_D("no pending client release SPI"));
		sc55112_status = sc55112_FREE;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function is used to send a frame on SPI bus.
 *
 * @param        num_reg    PMIC register number
 * @param        reg_value  Register value
 * @param        _rw       read or write operation
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
static PMIC_STATUS sc55112_spi_send_frame(int num_reg, unsigned int *reg_value,
					  bool _rw)
{
	unsigned int send_val;
	unsigned int frame = 0;
	unsigned int frame_ret = 0;
	unsigned int result = 0;

	if (_rw == true) {
		frame |= 0x80000000;
		frame |= (*reg_value & 0x0ffffff);
	} else {
		frame = 0;
	}
	frame |= ((unsigned int)num_reg & 0x3f) << 0x19;

	TRACEMSG(_K_D("\t\tspi send frame : value=0x%.8x"), frame & 0xffffffff);

	send_val = (((frame & 0x000000ff) << 0x18) |
		    ((frame & 0x0000ff00) << 0x08) |
		    ((frame & 0x00ff0000) >> 0x08) |
		    ((frame & 0xff000000) >> 0x18));
	/* use this to launch SPI operation. */
	result = spi_send_frame((unsigned char *)(&send_val),
				(unsigned long)4, spi_id);

	frame_ret = (((send_val & 0x000000ff) << 0x18) |
		     ((send_val & 0x0000ff00) << 0x08) |
		     ((send_val & 0x00ff0000) >> 0x08) |
		     ((send_val & 0xff000000) >> 0x18));
	*reg_value = frame_ret & 0x00ffffff;

	TRACEMSG(_K_D("\t\tspi received frame : value=0x%.8x"),
		 frame_ret & 0xffffffff);

	return PMIC_SUCCESS;
};
