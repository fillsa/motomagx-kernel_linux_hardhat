/*======================================================================

  Device driver for the PCMCIA control functionality of PXA2xx
  microprocessors.

    The contents of this file may be used under the
    terms of the GNU Public License version 2 (the "GPL")

    (c) Ian Molton (spyro@f2s.com) 2003
    (c) Stefan Eletzhofer (stefan.eletzhofer@inquant.de) 2003,4

    derived from sa11xx_base.c

     Portions created by John G. Dorsey are
     Copyright (C) 1999 John G. Dorsey.

  ======================================================================*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/config.h>
#include <linux/cpufreq.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#ifdef CONFIG_PM
#include <linux/pm.h>
#endif
#ifdef CONFIG_DPM
#include <linux/dpm.h>
#include <linux/notifier.h>
#endif

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/arch/pxa-regs.h>
#ifdef CONFIG_PM
#include <asm/arch/pcmcia.h>
#endif
#ifdef CONFIG_GPIO
#include <asm/arch/gpio.h>
#endif

#include <pcmcia/cs_types.h>
#include <pcmcia/ss.h>
#include <pcmcia/bulkmem.h>
#include <pcmcia/cistpl.h>

#include "cs_internal.h"
#include "soc_common.h"
#include "pxa2xx_base.h"


#define MCXX_SETUP_MASK     (0x7f)
#define MCXX_ASST_MASK      (0x1f)
#define MCXX_HOLD_MASK      (0x3f)
#define MCXX_SETUP_SHIFT    (0)
#define MCXX_ASST_SHIFT     (7)
#define MCXX_HOLD_SHIFT     (14)


struct pxa2xx_sock_timing{
	unsigned short io;
	unsigned short mem;
	unsigned short attr;
};

static struct pxa2xx_sock_timing *pxa2xx_pcmcia_timings = NULL;
static int pxa2xx_pcmcia_nr = 0;

static inline u_int pxa2xx_mcxx_hold(u_int pcmcia_cycle_ns,
				     u_int mem_clk_10khz)
{
	u_int code = pcmcia_cycle_ns * mem_clk_10khz;
	return (code / 300000) + ((code % 300000) ? 1 : 0) - 1;
}

static inline u_int pxa2xx_mcxx_asst(u_int pcmcia_cycle_ns,
				     u_int mem_clk_10khz)
{
	u_int code = pcmcia_cycle_ns * mem_clk_10khz;
	return (code / 300000) + ((code % 300000) ? 1 : 0) - 1;
}

static inline u_int pxa2xx_mcxx_setup(u_int pcmcia_cycle_ns,
				      u_int mem_clk_10khz)
{
	u_int code = pcmcia_cycle_ns * mem_clk_10khz;
	return (code / 100000) + ((code % 100000) ? 1 : 0) - 1;
}

/* This function returns the (approximate) command assertion period, in
 * nanoseconds, for a given CPU clock frequency and MCXX_ASST value:
 */
static inline u_int pxa2xx_pcmcia_cmd_time(u_int mem_clk_10khz,
					   u_int pcmcia_mcxx_asst)
{
	return (300000 * (pcmcia_mcxx_asst + 1) / mem_clk_10khz);
}

static int pxa2xx_pcmcia_set_mcmem( int sock, int speed, int clock )
{
	MCMEM(sock) = ((pxa2xx_mcxx_setup(speed, clock)
		& MCXX_SETUP_MASK) << MCXX_SETUP_SHIFT)
		| ((pxa2xx_mcxx_asst(speed, clock)
		& MCXX_ASST_MASK) << MCXX_ASST_SHIFT)
		| ((pxa2xx_mcxx_hold(speed, clock)
		& MCXX_HOLD_MASK) << MCXX_HOLD_SHIFT);

	pxa2xx_pcmcia_timings[sock].mem = speed;

	return 0;
}

static int pxa2xx_pcmcia_set_mcio( int sock, int speed, int clock )
{
	MCIO(sock) = ((pxa2xx_mcxx_setup(speed, clock)
		& MCXX_SETUP_MASK) << MCXX_SETUP_SHIFT)
		| ((pxa2xx_mcxx_asst(speed, clock)
		& MCXX_ASST_MASK) << MCXX_ASST_SHIFT)
		| ((pxa2xx_mcxx_hold(speed, clock)
		& MCXX_HOLD_MASK) << MCXX_HOLD_SHIFT);

	pxa2xx_pcmcia_timings[sock].io = speed;

	return 0;
}

static int pxa2xx_pcmcia_set_mcatt( int sock, int speed, int clock )
{
	MCATT(sock) = ((pxa2xx_mcxx_setup(speed, clock)
		& MCXX_SETUP_MASK) << MCXX_SETUP_SHIFT)
		| ((pxa2xx_mcxx_asst(speed, clock)
		& MCXX_ASST_MASK) << MCXX_ASST_SHIFT)
		| ((pxa2xx_mcxx_hold(speed, clock)
		& MCXX_HOLD_MASK) << MCXX_HOLD_SHIFT);

	pxa2xx_pcmcia_timings[sock].attr = speed;

	return 0;
}

static int pxa2xx_pcmcia_set_mcxx(struct soc_pcmcia_socket *skt, unsigned int clk)
{
	struct soc_pcmcia_timing timing;
	int sock = skt->nr;

	soc_common_pcmcia_get_timing(skt, &timing);

	pxa2xx_pcmcia_set_mcmem(sock, timing.mem, clk);
	pxa2xx_pcmcia_set_mcatt(sock, timing.attr, clk);
	pxa2xx_pcmcia_set_mcio(sock, timing.io, clk);

	return 0;
}

static int pxa2xx_pcmcia_set_timing(struct soc_pcmcia_socket *skt)
{
	unsigned int clk = get_memclk_frequency_10khz();
	return pxa2xx_pcmcia_set_mcxx(skt, clk);
}

#ifdef CONFIG_CPU_FREQ

static int
pxa2xx_pcmcia_frequency_change(struct soc_pcmcia_socket *skt,
			       unsigned long val,
			       struct cpufreq_freqs *freqs)
{
#warning "it's not clear if this is right since the core CPU (N) clock has no effect on the memory (L) clock"
	switch (val) {
	case CPUFREQ_PRECHANGE:
		if (freqs->new > freqs->old) {
			debug(skt, 2, "new frequency %u.%uMHz > %u.%uMHz, "
			       "pre-updating\n",
			       freqs->new / 1000, (freqs->new / 100) % 10,
			       freqs->old / 1000, (freqs->old / 100) % 10);
			pxa2xx_pcmcia_set_mcxx(skt, freqs->new);
		}
		break;

	case CPUFREQ_POSTCHANGE:
		if (freqs->new < freqs->old) {
			debug(skt, 2, "new frequency %u.%uMHz < %u.%uMHz, "
			       "post-updating\n",
			       freqs->new / 1000, (freqs->new / 100) % 10,
			       freqs->old / 1000, (freqs->old / 100) % 10);
			pxa2xx_pcmcia_set_mcxx(skt, freqs->new);
		}
		break;
	}
	return 0;
}
#endif

int pxa2xx_drv_pcmcia_probe(struct device *dev)
{
	int ret;
	struct pcmcia_low_level *ops;
	int first, nr;

	if (!dev || !dev->platform_data)
		return -ENODEV;

	ops = (struct pcmcia_low_level *)dev->platform_data;
	first = ops->first;
	nr = ops->nr;

	pxa2xx_pcmcia_timings = vmalloc(nr*sizeof(struct pxa2xx_sock_timing));
	if (!pxa2xx_pcmcia_timings)
		return -ENOMEM;

	pxa2xx_pcmcia_nr = nr;

	/* Provide our PXA2xx specific timing routines. */
	ops->set_timing  = pxa2xx_pcmcia_set_timing;
#ifdef CONFIG_CPU_FREQ
	ops->frequency_change = pxa2xx_pcmcia_frequency_change;
#endif

	ret = soc_common_drv_pcmcia_probe(dev, ops, first, nr);

	if (ret == 0) {
		/*
		 * We have at least one socket, so set MECR:CIT
		 * (Card Is There)
		 */
		MECR |= MECR_CIT;

		/* Set MECR:NOS (Number Of Sockets) */
		if (nr > 1)
			MECR |= MECR_NOS;
		else
			MECR &= ~MECR_NOS;

#if defined(CONFIG_PM) && defined(CONFIG_GPIO)
		gpio_set_wakeup_enable(0, 1);
		PFER |= 1;
#endif
	}

	return ret;
}
EXPORT_SYMBOL(pxa2xx_drv_pcmcia_probe);


int pxa2xx_drv_pcmcia_remove(struct device *dev)
{
	if( pxa2xx_pcmcia_timings != NULL){
		vfree(pxa2xx_pcmcia_timings);
		pxa2xx_pcmcia_timings = NULL;
	}
#if defined(CONFIG_PM) && defined(CONFIG_GPIO)
	gpio_set_wakeup_enable(0, 0);
	PFER &= ~1;
#endif
	return soc_common_drv_pcmcia_remove(dev);
}

static int pxa2xx_drv_pcmcia_suspend(struct device *dev, u32 state, u32 level)
{
	int ret = 0;
	if (level == SUSPEND_SAVE_STATE)
		ret = pcmcia_socket_dev_suspend(dev, state);
	return ret;
}

static int pxa2xx_drv_pcmcia_resume(struct device *dev, u32 level)
{
	int ret = 0;
	if (level == RESUME_RESTORE_STATE)
	{
		struct pcmcia_low_level *ops = dev->platform_data;
		int nr = ops ? ops->nr : 0;

		MECR = nr > 1 ? MECR_CIT | MECR_NOS : (nr > 0 ? MECR_CIT : 0);
		ret = pcmcia_socket_dev_resume(dev);
	}
#ifdef CONFIG_PM
	if ((PEDR & 1) && pxa2xx_board_pcmcia_event())
		/* Wakeup on GPIO0 triggered by PCMCIA hardware detected */
		pm_set_waker(dev);
#endif
	return ret;
}

#ifdef CONFIG_DPM

static int pxa2xx_drv_pcmcia_scale_callback(struct notifier_block *nb, unsigned long val, void *data)
{
	int i=0;
	unsigned int clk = get_memclk_frequency_10khz();
	if(pxa2xx_pcmcia_timings != NULL && pxa2xx_pcmcia_nr > 0){
		for(i=0; i < pxa2xx_pcmcia_nr; i++){
			pxa2xx_pcmcia_set_mcmem(i, pxa2xx_pcmcia_timings[i].mem, clk);
			pxa2xx_pcmcia_set_mcatt(i, pxa2xx_pcmcia_timings[i].attr, clk);
			pxa2xx_pcmcia_set_mcio(i, pxa2xx_pcmcia_timings[i].io, clk);
		}
	}
	return 0;
}

static struct notifier_block pxa2xx_drv_pcmcia_notifier_block = {
        .notifier_call  = pxa2xx_drv_pcmcia_scale_callback,
};

#endif /* CONFIG_DPM */

static struct device_driver pxa2xx_pcmcia_driver = {
	.probe		= pxa2xx_drv_pcmcia_probe,
	.remove		= pxa2xx_drv_pcmcia_remove,
	.suspend 	= pxa2xx_drv_pcmcia_suspend,
	.resume 	= pxa2xx_drv_pcmcia_resume,
	.name		= "pxa2xx-pcmcia",
	.bus		= &platform_bus_type,
};

static int __init pxa2xx_pcmcia_init(void)
{
	int ret;
	
	ret = driver_register(&pxa2xx_pcmcia_driver);
	if (ret)
		return ret;
#ifdef CONFIG_DPM
	dpm_register_scale(&pxa2xx_drv_pcmcia_notifier_block, SCALE_POSTCHANGE);
#endif
	return 0;
}

static void __exit pxa2xx_pcmcia_exit(void)
{
#ifdef CONFIG_DPM
	dpm_unregister_scale(&pxa2xx_drv_pcmcia_notifier_block, SCALE_POSTCHANGE);
#endif
	driver_unregister(&pxa2xx_pcmcia_driver);
}

module_init(pxa2xx_pcmcia_init);
module_exit(pxa2xx_pcmcia_exit);

MODULE_AUTHOR("Stefan Eletzhofer <stefan.eletzhofer@inquant.de> and Ian Molton <spyro@f2s.com>");
MODULE_DESCRIPTION("Linux PCMCIA Card Services: PXA2xx core socket driver");
MODULE_LICENSE("GPL");
