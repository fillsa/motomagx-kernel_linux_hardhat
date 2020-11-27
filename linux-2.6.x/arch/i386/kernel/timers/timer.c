#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/hrtime.h>
#include <asm/timer.h>

#ifdef CONFIG_HPET_TIMER
/*
 * If high res, we put that first...
 * HPET memory read is slower than tsc reads, but is more dependable as it
 * always runs at constant frequency and reduces complexity due to
 * cpufreq. So, we prefer HPET timer to tsc based one. Also, we cannot use
 * timer_pit when HPET is active. So, we default to timer_tsc.
 */
#endif
/* list of timers, ordered by preference, NULL terminated */
static struct init_timer_opts* __initdata timers[] = {
#ifdef CONFIG_HIGH_RES_TIMERS
#ifdef CONFIG_HIGH_RES_TIMER_ACPI_PM
	&hrtimer_pm_init,
#elif  CONFIG_HIGH_RES_TIMER_TSC
	&hrtimer_tsc_init,
#endif  /* CONFIG_HIGH_RES_TIMER_ACPI_PM */
#endif
#ifdef CONFIG_X86_CYCLONE_TIMER
#ifdef CONFIG_HIGH_RES_TIMERS
#error "The High Res Timers option is incompatable with the Cyclone timer"
#endif
	&timer_cyclone_init,
#endif
#ifdef CONFIG_HPET_TIMER
	&timer_hpet_init,
#endif
#ifdef CONFIG_X86_PM_TIMER
	&timer_pmtmr_init,
#endif
	&timer_tsc_init,
	&timer_pit_init,
	NULL,
};

static char clock_override[10] __initdata;
#ifndef CONFIG_HIGH_RES_TIMERS_try

static int __init clock_setup(char* str)
{
	if (str)
		strlcpy(clock_override, str, sizeof(clock_override));
	return 1;
}
__setup("clock=", clock_setup);


/* The chosen timesource has been found to be bad.
 * Fall back to a known good timesource (the PIT)
 */
void clock_fallback(void)
{
	cur_timer = &timer_pit;
}
#endif
/* iterates through the list of timers, returning the first 
 * one that initializes successfully.
 */
struct timer_opts* __init select_timer(void)
{
	int i = 0;
	
	/* find most preferred working timer */
	while (timers[i]) {
		if (timers[i]->init)
			if (timers[i]->init(clock_override) == 0)
				return timers[i]->opts;
		++i;
	}
		
	panic("select_timer: Cannot find a suitable timer\n");
	return NULL;
}
