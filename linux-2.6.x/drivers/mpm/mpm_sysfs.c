/*
 * mpm_sysfs.c - sysfs interface to mpm driver
 *
 * Copyright (C) 2006-2008 Motorola, Inc.
 *
 */

/*
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Date        Author            Comment
 * ==========  ================  ========================
 * 10/04/2006  Motorola          Initial version.
 * 12/04/2006  Motorola          Improved DSM time reporting for debug
 * 12/14/2006  Motorola          Improved operating point printing.
 * 01/28/2007  Motorola          Tie DSM to IDLE and reduce IOI timeout to 10ms
 * 02/14/2007  Motorola          Add statistics gathering.
 * 04/04/2007  Motorola          Add support for Argon.
 * 10/25/2007  Motorola          Improved periodic job state collection for debug.
 * 06/18/2008  Motorola          Add /sys/mpm/registered to show registered drivers
 */

#include <linux/module.h>
#include <linux/mpm.h>
#include <linux/pm.h>
#include <linux/rtc.h>
#include <asm/arch/mxc_mu.h>
#include "mpmdrv.h"


#define  mpm_subsys_attr(_prefix,_name, _mode) \
static struct subsys_attribute _prefix##_attr = { \
    .attr = {                                   \
            .name = __stringify(_name),         \
            .mode = _mode,                      \
    },                                          \
    .show = _prefix##_show,                     \
    .store = _prefix##_store,                   \
}                                  

/* this macro creates mpm_subsys subsytem, s kset */
decl_subsys(mpm, NULL, NULL);

#ifdef CONFIG_MOT_FEAT_PM_STATS
static int start_op_stat = 0;
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
static int start_pj_stat = 0;
#endif
#endif

static char current_state[9] = {"Normal"};

/*
OP attribute - To get current Operating Point, set specified Operating Point
*/

static ssize_t state_op_show(struct subsystem *subsys, char *buf)
{
    ssize_t len = 0;
    mpm_op_t op_var = 0;
    
    op_var = mpm_getcur_op();   
    len = sprintf(buf, "%lu MHz\n", op_var/1000000);
    return len;
}


static ssize_t state_op_store(struct subsystem *subsys, const char *buf, 
                size_t n)
{
    int error = 0;  
    int cnt = 0; 
    mpm_op_t op_var = 0;
    const mpm_op_t *mpm_op_ptr = NULL;
       
    op_var = simple_strtol(buf, NULL, 0);
    op_var *= 1000000;
   
    //check for validity of operating point
    cnt = mpm_getnum_op();
    mpm_op_ptr = mpm_getall_op();
    
    while(cnt--) 
    {
        if(*mpm_op_ptr == op_var)                
            break;
        ++mpm_op_ptr;
    }
    if(cnt < 0)
    {
        printk("invalid OP\n");
        return n;
    }
    error = mpm_set_op(op_var);
    if(error)
    {
        printk("unable to set operating point: %d", error);
    }
    return n;
}

mpm_subsys_attr(state_op,op,0644); 


/*
* AVAILOP - To get all operating points
*/

static ssize_t state_availop_show(struct subsystem *subsys, char *buf)
{
    ssize_t len = 0;
    int cnt = 0; 
    const mpm_op_t *mpm_op_ptr = NULL;
    char *ptemp = NULL;

    cnt = mpm_getnum_op();
    mpm_op_ptr = mpm_getall_op();
    ptemp = buf;
   
    if(NULL == mpm_op_ptr)
    {
        printk("no operating points available");
        return len;
    }
    while(cnt--)
    {
        len = sprintf(ptemp, "%lu MHz\n", (*mpm_op_ptr)/1000000);
        ptemp += len; 
        ++mpm_op_ptr;
    }
    len = strlen(buf);
    return len;
}

static ssize_t state_availop_store(struct subsystem *subsys, const char *buf, 
                size_t n)
{
    return n;
}

mpm_subsys_attr(state_availop,availop, 0444);

#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
static ssize_t state_registered_show(struct subsystem *subsys, char *buf)
{
    ssize_t len = 0;
    int i = 0;
    int registered_count = 0;
    char *ptemp = NULL;
    char **registered_name = NULL;
    unsigned long driver_flags;

    spin_lock_irqsave(&mpm_driver_info_lock, driver_flags);

    registered_count = mpmdip->registered_count;
    registered_name = mpmdip->name_arr;
    if (registered_name && registered_count > 0)
    {
        for (ptemp = buf, i = 0; i < registered_count; i++)
        {
            if (registered_name[i])
            {
                len = sprintf(ptemp, "Driver number: %-2d    Driver name: %s\n", \
                              i, registered_name[i]);
            } else {
                len = sprintf(ptemp, "Driver number: %-2d    Driver name is unknown\n", i);
            }
            if (len > 0)
                ptemp += len;
            else
                printk("sprintf failed at outputing registered driver: %d\n", i);
        }
        len = strlen(buf);
    }

    spin_unlock_irqrestore(&mpm_driver_info_lock, driver_flags);
    return len;
}

static ssize_t state_registered_store(struct subsystem *subsys, 
                                      const char *buf, size_t n)
{
    return n;
}

mpm_subsys_attr(state_registered, registered, 0444);
#endif        
#ifdef CONFIG_MOT_FEAT_PM_STATS

/*
* ctl - enable or disable collection of OP or lpm statistics
*/

static ssize_t control_ctl_show(struct subsystem *subsys, char *buf)
{
    ssize_t len = 0;
    char* lpm_stats = "off";
    char* dvfs_stats = "off";

    if(start_op_stat) {
        dvfs_stats = "on";
    }
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
    char* pjs_stats = "off";
    if(start_pj_stat) {
        pjs_stats = "on";
    }
#endif
    if(LPM_STATS_ENABLED()) {
        lpm_stats = "on";
    }

#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
    len = sprintf(buf, "dvfs: %s\npjs: %s\nlpm: %s\n", dvfs_stats, pjs_stats, lpm_stats);
#else
    sprintf(buf, "dvfs: %s\nlpm: %s\n", dvfs_stats, lpm_stats);
#endif
    return len;
}

static ssize_t control_ctl_store(struct subsystem *subsys, 
                const char *buf, size_t n)
{
    int len = strcspn(buf, " ");

    // Required format:  [lpm | dvfs ] [on | off | reset]
    if(strnicmp(buf, "dvfs", len) == 0) {
        if(strnicmp(buf+len, " on", 3) == 0) {
            MPM_DPRINTK("Enabling dvfs stats\n");
            start_op_stat = 1;
            mpm_start_opstat( );
        }
        else if(strnicmp(buf+len, " off", 4) == 0) {
            MPM_DPRINTK("Disabling dvfs stats\n");
            start_op_stat = 0;
            mpm_stop_opstat( );
        }
        else if(strnicmp(buf+len, " reset", 6) == 0) {
            MPM_DPRINTK("Resetting dvfs stats\n");
            mpm_stop_opstat( );
            mpm_start_opstat( );
        }
    }
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
    else if(strnicmp(buf, "pjs", len) == 0) {
        if(strnicmp(buf+len, " on", 3) == 0) {
            MPM_DPRINTK("Enabling pjs stats\n");
            start_pj_stat = 1;
            mpm_start_pjstat(1);
        }
        else if(strnicmp(buf+len, " -s on", 6) == 0) {
            MPM_DPRINTK("Enabling pjs stats in sleep mode\n");
            start_pj_stat = 2;
            mpm_start_pjstat(2);
        }
        else if(strnicmp(buf+len, " off", 4) == 0) {
            MPM_DPRINTK("Disabling pjs stats\n");
            start_pj_stat = 0;
            mpm_stop_pjstat( );
        }
        else if(strnicmp(buf+len, " reset", 6) == 0) {
            MPM_DPRINTK("Resetting pjs stats\n");
            mpm_stop_pjstat( );
            mpm_start_pjstat(start_pj_stat);
        }
    }
#endif
    else if(strnicmp(buf, "lpm", len) == 0) {
        if(strnicmp(buf+len, " on", 3) == 0) {
            MPM_DPRINTK("Enabling lpm stats\n");
            mpm_lpm_stat_ctl(1);
        }
        else if(strnicmp(buf+len, " off", 4) == 0) {
            MPM_DPRINTK("Disabling lpm stats\n");
            mpm_lpm_stat_ctl(0);
        }
        else if(strnicmp(buf+len, " reset", 6) == 0) {
            MPM_DPRINTK("Resetting lpm stats\n");
            mpm_reset_lpm_stats();
        }
    }
    else if(strnicmp(buf, "all", len) == 0) {
        if(strnicmp(buf+len, " on", 3) == 0) {
            MPM_DPRINTK("Enabling all stats\n");
            start_op_stat = 1;
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
            start_pj_stat = 1;
#endif
            mpm_start_opstat( );
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
            mpm_start_pjstat(1);
#endif
            mpm_lpm_stat_ctl(1);
        }
        else if(strnicmp(buf+len, " off", 4) == 0) {
            MPM_DPRINTK("Disabling all stats\n");
            start_op_stat = 0;
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
            start_pj_stat = 0;
#endif
            mpm_stop_opstat( );
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
            mpm_stop_pjstat( );
#endif
            mpm_lpm_stat_ctl(0);
        }
        else if(strnicmp(buf+len, " reset", 6) == 0) {
            MPM_DPRINTK("Resetting all stats\n");
            mpm_stop_opstat( );
            mpm_start_opstat( );
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
            mpm_stop_pjstat( );
            mpm_start_pjstat(start_pj_stat);
#endif
            mpm_reset_lpm_stats( );
        }
    }
    else {
        printk("Invalid control command: %sExiting.", buf);
    }

    return n;
}

mpm_subsys_attr(control_ctl,ctl, 0644);


/*
* dvfs - To print dvfs statistics such as time spent in each
*        operating point, since last print command
*/

static ssize_t state_dvfs_show(struct subsystem *subsys, char *buf)
{
    ssize_t len = 0;

    mpm_print_opstat(buf, PAGE_SIZE);
    len = strlen(buf);
    return len;
}

static ssize_t state_dvfs_store(struct subsystem *subsys, 
                const char *buf, size_t n)
{
    return n;
}


mpm_subsys_attr(state_dvfs,dvfs, 0444);


#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
/*
* pjs - To print periodic job information.
*/

static ssize_t state_pjs_show(struct subsystem *subsys, char *buf)
{
    ssize_t len = 0;

    mpm_print_pjstat(buf, PAGE_SIZE);
    len = strlen(buf);
    return len;
}

static ssize_t state_pjs_store(struct subsystem *subsys,
                const char *buf, size_t n)
{
    return n;
}

mpm_subsys_attr(state_pjs, pjs, 0444);
#endif

#endif


/*
* MODE - To enter STOP, DSM mode
*
*/

static ssize_t state_mode_show(struct subsystem *subsys, char *buf)
{
    ssize_t len = 0;
    char* bp_mode_name = "unavailable";

    int bp_mode = mxc_mu_dsp_pmode_status( );

    switch(bp_mode) {
        case 0:
            bp_mode_name = "Run";
            break;
        case 1:
            bp_mode_name = "Wait";
            break;
        case 2:
            bp_mode_name = "Stop";
            break;
        case 3:
            bp_mode_name = "DSM";
            break;
        default:
            break;
    }

    sprintf(buf, "Current AP mode:  Run\nCurrent BP mode:  %s\n", bp_mode_name);
    len = strlen(buf);
    return len;
}


static ssize_t state_mode_store(struct subsystem *subsys,
                const char *buf, size_t n)
{
    int retvalue = 0;
    int len = 0;

    len = strlen(buf);

    // Clear command
    if(!(strnicmp("clear\n", buf, len)))
    {
       strcpy (current_state, "Run");
       return n;
    }

    // Sleep commands
    mpm_set_initiate_sleep_state();
    if(!(strnicmp("stop\n", buf, len)))
    {
       strcpy (current_state, "STOP");
       retvalue = pm_suspend(PM_SUSPEND_STOP);
    }
    else if(!(strnicmp("fsl_stop\n", buf, len)))
    {
       strcpy (current_state, "fsl_STOP");
#ifdef CONFIG_MOT_FEAT_PM_BUTE
       printk("STOP mode is not supported!\n");
#else
       mxc_pm_lowpower(STOP_MODE);    // STOP_MODE - 112
#endif
    }
    else if(!(strnicmp("dsm\n", buf, len)))
    {
       strcpy (current_state, "DSM");
       retvalue = pm_suspend(PM_SUSPEND_MEM);
    }
    else if(!(strnicmp("fsl_dsm\n", buf, len)))
    {
       strcpy (current_state, "fsl_DSM");
#ifdef CONFIG_MOT_FEAT_PM_BUTE
       printk("DSM is not supported!\n");
#else
       mxc_pm_lowpower(DSM_MODE);   // DSM_MODE - 113
#endif
    }
    mpm_set_awake_state();

    if(retvalue) {
        printk("Sleep attempt failed (return value %d)\n", retvalue);
    }

    return n;
}
mpm_subsys_attr(state_mode, mode, 0644);


/*
* DESENSE - To print information on desense data
*
*/

static ssize_t state_desense_show(struct subsystem *subsys, char *buf)
{
    ssize_t len = 0;
    char *p;
#ifdef CONFIG_MOT_FEAT_PM_DESENSE
    ap_all_plls_mfn_values_t pllvals;
    ap_pll_mfn_values_t *pvp;
    mpm_desense_info_t desense;
    int desense_result;
    dvfs_op_point_index_t dvfs_op_index;
#endif /* CONFIG_MOT_FEAT_PM_DESENSE */

    p = buf;

#ifdef CONFIG_MOT_FEAT_PM_DESENSE
    mxc_pm_dither_report(&pllvals);

    /*
     * AP CORE NORMAL PLL information.
     */
    len = sprintf(p, "AP CORE NORMAL PLL desense:\n");
    p += len;
    for (dvfs_op_index=INDX_FIRST; dvfs_op_index<NUM_DVFSOP_INDEXES; dvfs_op_index++)
    {
	switch (dvfs_op_index)
	{
	case INDX_133:
	    len = sprintf(p, "    CORE_133:");
	    p += len;
	    break;
	case INDX_266:
	    len = sprintf(p, "    CORE_266:");
	    p += len;
	    break;
	case INDX_399:
	    len = sprintf(p, "    CORE_399:");
	    p += len;
	    break;
	case INDX_532:
	    len = sprintf(p, "    CORE_532:");
	    p += len;
	    break;
#ifdef CONFIG_FEAT_MXC31_CORE_OVERCLOCK_740
	case INDX_636:
	    len = sprintf(p, "    CORE_636:");
	    p += len;
	    break;
	case INDX_740:
	    len = sprintf(p, "    CORE_740:");
	    p += len;
	    break;
#elif CONFIG_FEAT_MXC31_CORE_OVERCLOCK_780
	case INDX_665:
	    len = sprintf(p, "    CORE_665:");
	    p += len;
	    break;
	case INDX_780:
	    len = sprintf(p, "    CORE_780:");
	    p += len;
	    break;
#endif // CONFIG_FEAT_MXC31_CORE_OVERCLOCK_
	default:
	    len = sprintf(p, "    UNKNOWN_OP_PT:");
	    p += len;
	    break;
	}

	pvp = &pllvals.ap_core_normal_pll[dvfs_op_index];

	len = sprintf(p, "        ppm: 0x%08x (%d)\n",
			 pvp->ap_pll_ppm_value, pvp->ap_pll_ppm_value);
	p += len;
	len = sprintf(p, "        hfs op: 0x%08x (%d)   mfn: 0x%08x (%d)   mfd: 0x%08x (%d)\n",
			 pvp->ap_pll_dp_hfs_op, pvp->ap_pll_dp_hfs_op,
			 pvp->ap_pll_dp_hfs_mfn, pvp->ap_pll_dp_hfs_mfn,
			 pvp->ap_pll_dp_hfs_mfd, pvp->ap_pll_dp_hfs_mfd);
	p += len;
	len = sprintf(p, "        hfs mfnminus: 0x%08x (%d)   mfnplus: 0x%08x (%d)\n",
			 pvp->ap_pll_dp_hfs_mfnminus, pvp->ap_pll_dp_hfs_mfnminus,
			 pvp->ap_pll_dp_hfs_mfnplus, pvp->ap_pll_dp_hfs_mfnplus);
	p += len;
    }

    /*
     * AP USB PLL information.
     */
    pvp = &pllvals.ap_usb_pll;

    len = sprintf(p, "AP USB PLL desense:\n");
    p += len;
    len = sprintf(p, "        ppm: 0x%08x (%d)\n",
		     pvp->ap_pll_ppm_value, pvp->ap_pll_ppm_value);
    p += len;
    len = sprintf(p, "        hfs op: 0x%08x (%d)   mfn: 0x%08x (%d)   mfd: 0x%08x (%d)\n",
		     pvp->ap_pll_dp_hfs_op, pvp->ap_pll_dp_hfs_op,
		     pvp->ap_pll_dp_hfs_mfn, pvp->ap_pll_dp_hfs_mfn,
		     pvp->ap_pll_dp_hfs_mfd, pvp->ap_pll_dp_hfs_mfd);
    p += len;
    len = sprintf(p, "        hfs mfnminus: 0x%08x (%d)   mfnplus: 0x%08x (%d)\n",
		     pvp->ap_pll_dp_hfs_mfnminus, pvp->ap_pll_dp_hfs_mfnminus,
		     pvp->ap_pll_dp_hfs_mfnplus, pvp->ap_pll_dp_hfs_mfnplus);
    p += len;

    /*
     * Provide information about the most recent desense request, but
     * only if it failed.  If the most recent desense request succeeded,
     * then the requested PPM values are already provided above.
     */
    mpm_get_last_desense_request(&desense, &desense_result);
    if (desense_result != 0)
    {
        len = sprintf(p, "The most recent desense request failed:\n");
        p += len;
        len = sprintf(p, "    result:  %d\n", desense_result);
        p += len;
        len = sprintf(p, "    version: %d\n", desense.version);
        p += len;
        len = sprintf(p, "    ap_core_normal_pll_ppm: 0x%08x (%d)\n",
			      desense.ap_core_normal_pll_ppm,
			      desense.ap_core_normal_pll_ppm);
        p += len;
        len = sprintf(p, "    ap_core_turbo_pll_ppm:  0x%08x (%d)\n",
			      desense.ap_core_turbo_pll_ppm,
			      desense.ap_core_turbo_pll_ppm);
        p += len;
        len = sprintf(p, "    ap_usb_pll_ppm:         0x%08x (%d)\n",
			      desense.ap_usb_pll_ppm,
			      desense.ap_usb_pll_ppm);
        p += len;
    }

#else  /* CONFIG_MOT_FEAT_PM_DESENSE */

    len = sprintf(p, "Desense not implemented for this hardware.\n");
    p += len;

#endif /* CONFIG_MOT_FEAT_PM_DESENSE */

    return (p - buf);
}

static ssize_t state_desense_store(struct subsystem *subsys, 
                const char *buf, size_t n)
{
    return n;
}

mpm_subsys_attr(state_desense, desense, 0644);


/* HELP */

static ssize_t state_help_show(struct subsystem *subsys, char *buf)
{
    ssize_t len = 0;

    sprintf
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
        (buf,"\nUSAGE:\n\n"                                             \
         "cat /sys/mpm/availop\n"                                       \
         "    Display the list of available operating point frequencies.\n\n" \
         "cat /sys/mpm/help\n"                                          \
         "    Print this help page\n\n"                                 \
         "cat /sys/mpm/mode\n"                                          \
         "    Display the current power mode of the AP and BP.\n\n"     \
         "echo value > /sys/mpm/mode\n"                                 \
         "    Place the system in the specified low power mode\n"       \
         "    Valid values are clear, stop, fsl_stop, DSM, fsl_DSM.\n\n" \
         "cat /sys/mpm/desense\n"                                       \
         "    Display desense information.\n\n"                         \
         "cat /sys/mpm/registered\n"                                    \
         "    Display information of drivers registered in mpm.\n\n"   \
         "cat /sys/mpm/op\n"                                            \
         "    Display current operating point.\n\n"                     \
         "echo value > /sys/mpm/op\n"                                   \
         "    Set value(in MHz) as the operating frequency.\n\n"        \
         "(The following cmds are only available if stats collection is enabled"
         "cat /sys/mpm/ctl\n"                                      \
         "    Print the status of statistics collection.\n\n"           \
         "echo [dvfs | lpm | pjs | all] [\"on\" | \"off\" | \"reset\"] "  \
         "> /sys/mpm/ctl\n"                                             \
         "    Enable/disable/reset statistics collection for dvfs, lpm or pjs.\n\n" \
         "echo pjs -s on > /sys/mpm/ctl\n"                              \
         "    Enable statistics collection when phone sleep for pjs.\n\n" \
         "cat /sys/mpm/dvfs\n"                                          \
         "    Display dvfs statistics\n\n"                              \
         "cat /sys/mpm/lpm\n"                                      \
         "    Display low power modes statistics\n\n"                   \
         );
#else         
        (buf,"\nUSAGE:\n\n"                                             \
         "cat /sys/mpm/availop\n"                                       \
         "    Display the list of available operating point frequencies.\n\n" \
         "cat /sys/mpm/help\n"                                          \
         "    Print this help page\n\n"                                 \
         "cat /sys/mpm/mode\n"                                          \
         "    Display the current power mode of the AP and BP.\n\n"     \
         "echo value > /sys/mpm/mode\n"                                 \
         "    Place the system in the specified low power mode\n"       \
         "    Valid values are clear, stop, fsl_stop, DSM, fsl_DSM.\n\n" \
         "cat /sys/mpm/desense\n"                                       \
         "    Display desense information.\n\n"                         \
         "cat /sys/mpm/op\n"                                            \
         "    Display current operating point.\n\n"                     \
         "echo value > /sys/mpm/op\n"                                   \
         "    Set value(in MHz) as the operating frequency.\n\n"        \
         "(The following cmds are only available if stats collection is enabled"
         "cat /sys/mpm/stat/ctl\n"                                      \
         "    Print the status of statistics collection.\n\n"           \
         "echo [dvfs | lpm | all] [\"on\" | \"off\" | \"reset\"] "      \
         "> /sys/mpm/stat/ctl\n"                                        \
         "    Enable/disable/reset statistics collection for dvfs or lpm.\n\n" \
         "cat /sys/mpm/stat/dvfs\n"                                     \
         "    Display dvfs statistics\n\n"                              \
         "cat /sys/mpm/stat/lpm\n"                                      \
         "    Display low power modes statistics\n\n"                   \
         );
#endif

    len = strlen(buf);
    return len;
}


static ssize_t state_help_store(struct subsystem *subsys,
                const char *buf, size_t n)
{
    return n;            
}

mpm_subsys_attr(state_help, help, 0444);

#ifdef CONFIG_MOT_FEAT_PM_STATS

/*
 * sprintf_time formats the time given in milliseconds into a days,
 * hours, minutes, seconds, milliseconds format.
 *
 * The prefix is formatted into the string before the time.  If prefix
 * is NULL, then no prefix is printed.
 * The suffix is formatted into the string before the time.  If suffix
 * is NULL, then no suffix is printed.
 *
 * It returns the number of bytes formatted into the pointer (p).
 */
static ssize_t sprintf_time(char *p, char *prefix, u32 time, char* suffix)
{
    u32 days = 0;
    u32 hours = 0;
    u32 minutes = 0;
    u32 seconds = 0;
    int len;
    char *origp;

    origp = p;

    if (prefix)
    {
        len = sprintf(p, "%s", prefix);
        p += len;
    }

    if (time >= MPM_MS_PER_DAY)
    {
	days = time / MPM_MS_PER_DAY;
	time -= days * MPM_MS_PER_DAY;
        len = sprintf(p, "%d days, ", days);
	p += len;
    }

    hours = time / MPM_MS_PER_HOUR;
    time -= hours * MPM_MS_PER_HOUR;

    minutes = time / MPM_MS_PER_MINUTE;
    time -= minutes * MPM_MS_PER_MINUTE;

    seconds = time / MPM_MS_PER_SECOND;
    time -= seconds * MPM_MS_PER_SECOND;

    len = sprintf(p, "%02d:%02d:%02d.%03d", hours, minutes, seconds, time);
    p += len;

    if (suffix)
    {
        len = sprintf(p, "%s", suffix);
        p += len;
    }

    return (p - origp);
}


static ssize_t state_lpm_show(struct subsystem *subsys, char *buf)
{
    ssize_t len=0;
    u32 stat_time;
    char *p;
    int indx;
    int i;
    mpm_stats_t *sp;
    unsigned long flags;

    sp = kmalloc (sizeof(mpm_stats_t), GFP_KERNEL);
    if (sp == NULL)
    {
        len = sprintf(buf, "--- Cannot provide low power mode statistics "
                           "(OUT OF MEMORY) ---\n");
        return (len);
    }

    spin_lock_irqsave(&mpmsplk, flags);

    if (!LPM_STATS_ENABLED()) {
        spin_unlock_irqrestore(&mpmsplk, flags);
        kfree(sp);
    
        len = sprintf(buf, "--- Low power mode statistics are not enabled ---\n");
        return (len);
    }
    memcpy(sp, mpmsp, sizeof(mpm_stats_t));
    
    spin_unlock_irqrestore(&mpmsplk, flags);

    stat_time = rtc_sw_msfromboot();

    p = buf;

    len = sprintf(p, "--- Low power mode statistics "
		     "(all times are HH:MM:SS.ddd) ---\n");
    p += len;

    len = sprintf(p, "Stat collection is currently enabled\n");
    p += len;

    len = sprintf_time(p, "Stat collection elapsed time: ", stat_time, "\n");
    p += len;

    for (i=0; i<MAX_PMMODES; i++)
    {
        indx = sp->pmmode[i].indx;
        if (--indx < 0)
	    indx = MAX_MODE_TIMES - 1;

        len = sprintf(p, "  Statistics for ");
        p += len;

	switch (indx2cpumode(i))
	{
	  case WAIT_MODE:
		len = sprintf(p, "%s", "WAIT_MODE");
		break;
	  case DOZE_MODE:
		len = sprintf(p, "%s", "DOZE_MODE");
		break;
	  case STOP_MODE:
		len = sprintf(p, "%s", "STOP_MODE");
		break;
	  case DSM_MODE:
		len = sprintf(p, "%s", "DSM_MODE");
		break;
	  default:
		len = sprintf(p, "%s", "UNKNOWN MODE");
		break;
	}
        p += len;

	/*
	 * If there are no stats, then print that and skip to the
	 * next mode.
	 */
	if (sp->pmmode[i].count == 0)
	{
	    len = sprintf(p, "%s\n", ":  no stats (count==0)");
            p += len;
	    continue;
	}
        len = sprintf(p, ":\n");
        p += len;

        len = sprintf(p, "      number of times mode entered: %u\n",
				sp->pmmode[i].count);
        p += len;

        len = sprintf_time(p, "      total time spent in mode:  ",
			   sp->pmmode[i].total_time, "\n");
        p += len;

        len = sprintf_time(p, "      mininum duration spent in mode:  ",
			   sp->pmmode[i].minduration, "\n");
        p += len;

        len = sprintf_time(p, "      maximum duration spent in mode:  ",
			   sp->pmmode[i].maxduration, "\n");
        p += len;
    }

    len = sprintf_time(p, "  last time spent suspending devices: ",
		          sp->last_suspend_time, "\n");
    p += len;

    len = sprintf_time(p, "  last time spent suspending devices "
		          "with interrupts blocked: ",
		          sp->last_suspend_time_interrupts_blocked,
			  "\n");
    p += len;

    len = sprintf(p, "Failed DSM attempts: %d\n", sp->failed_dsm_count);
    p += len;

    len = sprintf_time(p, "Time spent suspending devices (total): ",
                          sp->total_suspend_time, "\n");
    p += len;

    len = sprintf_time(p, "Time spent suspending devices "
		          "with interrupts blocked (total): ",
                          sp->total_suspend_time_interrupts_blocked,
			  "\n");
    p += len;

    len = sprintf(p, "Device suspend failures: %d\n",
		     sp->suspend_device_fail_count);
    p += len;

    len = sprintf(p, "  Last device that failed to suspend: ");
    p += len;
    if (sp->failed_dev_name_len)
        len = sprintf(p, "%s\n", sp->failed_dev_name);
    else
        len = sprintf(p, "None\n");
    p += len;

    len = sprintf(p, "Suspend cancelled by user activity: %d\n",
		     sp->suspend_cancel_count);
    p += len;

    len = sprintf(p, "Suspend delayed by interrupt: %d\n",
		     sp->suspend_delay_ioi_count);
    p += len;

    len = sprintf(p, "Suspend delayed by periodic job: %d\n",
		     sp->suspend_delay_pj_count);
    p += len;
    len = sprintf(p, "State transition errors: %d\n",
		     sp->state_transition_error_count);
    p += len;

    kfree(sp);

    return (p - buf);
}

static ssize_t state_lpm_store(struct subsystem *subsys,
                const char *buf, size_t n)
{
    return n;
}

mpm_subsys_attr(state_lpm, lpm, 0444);
#endif

/*
* Init 
*/

static struct attribute * mpm_attr_array[] = {
        &state_op_attr.attr,
        &state_availop_attr.attr,
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
        &state_registered_attr.attr,
#endif
        &state_mode_attr.attr,
        &state_desense_attr.attr,
        &state_help_attr.attr,
#ifdef CONFIG_MOT_FEAT_PM_STATS
        &state_lpm_attr.attr,
        &control_ctl_attr.attr,
        &state_dvfs_attr.attr,
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) || defined(CONFIG_MACH_KEYWEST) || defined(CONFIG_MACH_PAROS) || defined(CONFIG_MACH_PEARL)
        &state_pjs_attr.attr,
#endif
#endif        
        NULL
};


static struct attribute_group mpm_attr_group = {
        .attrs = mpm_attr_array,
};


int init_mpm_sysfs(void)
{
    int error;

    MPM_DPRINTK("Initializing mpm sysfs ..\n");
    error =  subsystem_register(&mpm_subsys);
    if(!error)
    {
        error = sysfs_create_group(&mpm_subsys.kset.kobj,&mpm_attr_group);
        if(error) {
            printk("unable to create mpm group\n");
            subsystem_unregister(&mpm_subsys);
        }
    }
    return error;
}

/* 
 * EXIT
*/
 
void exit_mpm_sysfs(void)
{
    subsystem_unregister(&mpm_subsys);
    return;
}
