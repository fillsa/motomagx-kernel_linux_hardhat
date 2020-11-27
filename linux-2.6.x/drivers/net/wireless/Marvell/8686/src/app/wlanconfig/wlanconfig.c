/** @file  wlanconfig.c
  * @brief Program to configure addition paramters into the wlan driver
  * 
  * (c) Copyright © 2003-2007, Marvell International Ltd. 
  *
  * This software file (the "File") is distributed by Marvell International 
  * Ltd. under the terms of the GNU General Public License Version 2, June 1991 
  * (the "License").  You may use, redistribute and/or modify this File in 
  * accordance with the terms and conditions of the License, a copy of which 
  * is available along with the File in the gpl.txt file or by writing to 
  * the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
  * 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
  *
  * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
  * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
  * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
  * this warranty disclaimer.
  *
  */
/********************************************************
Change log:
	10/12/05: Add Doxygen format comments
	11/03/05: Load priv ioctls on demand, ifdef code for features in driver
	 
	12/14/05: Support wildcard SSID in BGSCAN	
	01/11/06: Add getscantable, setuserscan, setmrvltlv, getassocrsp 
	01/31/06: Add support to selectively enabe the FW Scan channel filter	
	02/24/06: fix getscanlist function not work on linux 2.6.15 X86
	04/06/06: Add TSPEC, queue metrics, and MSDU expiry support
	04/10/06: Add hostcmd generic API and power_adapt_cfg_ext command
	04/18/06: Remove old Subscrive Event and add new Subscribe Event
		  implementation through generic hostcmd API
	05/03/06: Add auto_tx hostcmd
	05/15/06: Support EMBEDDED_TCPIP with Linux 2.6.9
********************************************************/

#include    <stdio.h>
#include    <unistd.h>
#include    <time.h>
#include    <ctype.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <string.h>
#include    <stdlib.h>
#ifndef MOTO_PLATFORM
#include    <linux/if.h>
#endif
#include    <sys/ioctl.h>
#include    <linux/wireless.h>
#include    <linux/if_ether.h>
#include    <linux/byteorder/swab.h>
#include    <errno.h>

#define WLANCONFIG_VER "1.01"   /* wlanconfig Version number */

#ifdef 	BYTE_SWAP
#define 	cpu_to_le16(x)	__swab16(x)
#else
#define		cpu_to_le16(x)	(x)
#endif

#ifndef __ATTRIB_ALIGN__
#define __ATTRIB_ALIGN__ __attribute__((aligned(4)))
#endif

#ifndef __ATTRIB_PACK__
#define __ATTRIB_PACK__  __attribute__((packed))
#endif

/*
 *  ctype from older glib installations defines BIG_ENDIAN.  Check it 
 *   and undef it if necessary to correctly process the wlan header
 *   files
 */
#if (BYTE_ORDER == LITTLE_ENDIAN)
#undef BIG_ENDIAN
#endif

#include	"wlan_types.h"
#include	"wlan_defs.h"
#include    "wlan_wmm.h"
#include    "wlan_11d.h"

#include	"host.h"
#include	"hostcmd.h"
#include    "wlan_scan.h"
#include    "wlan_join.h"
#include	"wlan_wext.h"
#include	"wlanconfig.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* MIN */

enum COMMANDS
{
    CMD_HOSTCMD,
    CMD_RDMAC,
    CMD_WRMAC,
    CMD_RDBBP,
    CMD_WRBBP,
    CMD_RDRF,
    CMD_WRRF,
    CMD_RDEEPROM,
    CMD_CMD52R,
    CMD_CMD52W,
    CMD_CMD53R,
    CMD_CMD53W,
    CMD_BG_SCAN_CONFIG,
    CMD_ADDTS,
    CMD_DELTS,
    CMD_QCONFIG,
    CMD_QSTATS,
    CMD_WMM_QSTATUS,
    CMD_WMM_ACK_POLICY,
    CMD_WMM_AC_WPAIE,
    CMD_CAL_DATA_EXT,
    CMD_GETRATE,
    CMD_SLEEPPARAMS,
    CMD_BCA_TS,
    CMD_REASSOCIATE,
    CMD_EXTSCAN,
    CMD_SCAN_LIST,
    CMD_SET_GEN_IE,
    CMD_GET_SCAN_RSP,
    CMD_SET_USER_SCAN,
    CMD_SET_MRVL_TLV,
    CMD_GET_ASSOC_RSP,
    CMD_GET_CFP_TABLE,
    CMD_ARPFILTER,
};

#define IW_MAX_PRIV_DEF		128

/********************************************************
		Local Variables
********************************************************/
static s8 *commands[] = {
    "hostcmd",
    "rdmac",
    "wrmac",
    "rdbbp",
    "wrbbp",
    "rdrf",
    "wrrf",
    "rdeeprom",
    "sdcmd52r",
    "sdcmd52w",
    "sdcmd53r",
    "sdcmd53w",
    "bgscanconfig",
    "addts",
    "delts",
    "qconfig",
    "qstats",
    "qstatus",
    "wmm_ack_policy",
    "wmmparaie",
    "caldataext",
    "getrate",
    "sleepparams",
    "bca-ts",
    "reassociate",
    "extscan",
    "getscanlist",
    "setgenie",
    "getscantable",
    "setuserscan",
    "setmrvltlv",
    "getassocrsp",
    "getcfptable",
    "arpfilter",
};

static s8 *usage[] = {
    "Usage: ",
    "   	wlanconfig -v  (version)",
    "	wlanconfig <ethX> <cmd> [...]",
    "	where",
    "	ethX	: wireless network interface",
    "	cmd	: hostcmd, rdmac, wrmac, rdbbp, wrbbp, rdrf, wrrf",
    "		: sdcmd52r, sdcmd52w, sdcmd53r",
    "		: caldataext",
    "		: rdeeprom,sleepparams",
    "		: bca-ts",
    "		: bgscanconfig",
    "		: qstatus, wmmparaie, wmm_ack_policy",
    "		: addts, delts, qconfig, qstats",
    "		: reassociate",
    "		: setgenie",
    "		: getscantable, setuserscan",
    "		: setmrvltlv, getassocrsp",
    "		: getcfptable",
    "		: arpfilter",
    "	[...]	: additional parameters for read registers are",
    "		:	<offset>",
    "		: additional parameters for write registers are",
    "		:	<offset> <value>",
    "		: additonal parameter for hostcmd",
    "		: 	<filename> <cmd>",
    "		: addition parameters for caldataext",
    "		: 	<filename>",
    "		: additional parameters for reassociate are:",
    "		:	XX:XX:XX:XX:XX:XX YY:YY:YY:YY:YY:YY < string max 32>",
    "		:	< Current BSSID > < Desired BSSID > < Desired SSID >",
    "		: additonal parameter for arpfilter",
    "		: 	<filename>",
};

static s32 sockfd;
static s8 dev_name[IFNAMSIZ + 1];
static struct iw_priv_args Priv_args[IW_MAX_PRIV_DEF];
static int we_version_compiled = 0;
#define MRV_EV_POINT_OFF (((char *) &(((struct iw_point *) NULL)->length)) - \
			  (char *) NULL)

static u16 TLVChanSize;
static u16 TLVSsidSize;
static u16 TLVProbeSize;
static u16 TLVSnrSize;
static u16 TLVBcProbeSize;
static u16 TLVNumSsidProbeSize;
static u16 TLVStartBGScanLaterSize;
static u16 ActualPos = sizeof(HostCmd_DS_802_11_BG_SCAN_CONFIG);

char rate_bitmap[16][16] =
    { "1", "2", "5.5", "11", "reserved", "6", "9", "12", "18", "24", "36",
"48", "54", "reserved", "reserved", "reserved" };

static s8 *wlan_config_get_line(s8 * s, s32 size, FILE * stream, int *line);

/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/
/** 
 *  @brief convert char to hex integer
 * 
 *  @param chr 		char to convert
 *  @return      	hex integer or 0
 */
static int
hexval(s32 chr)
{
    if (chr >= '0' && chr <= '9')
        return chr - '0';
    if (chr >= 'A' && chr <= 'F')
        return chr - 'A' + 10;
    if (chr >= 'a' && chr <= 'f')
        return chr - 'a' + 10;

    return 0;
}

/** 
 *  @brief Hump hex data
 *
 *  @param prompt	A pointer prompt buffer
 *  @param p		A pointer to data buffer
 *  @param len		the len of data buffer
 *  @param delim	delim char
 *  @return            	hex integer
 */
static void
hexdump(s8 * prompt, void *p, s32 len, s8 delim)
{
    s32 i;
    u8 *s = p;

    if (prompt) {
        printf("%s: len=%d\n", prompt, (int) len);
    }
    for (i = 0; i < len; i++) {
        if (i != len - 1)
            printf("%02x%c", *s++, delim);
        else
            printf("%02x\n", *s);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
}

/** 
 *  @brief convert char to hex integer
 *
 *  @param chr		char
 *  @return            	hex integer
 */
static u8
hexc2bin(s8 chr)
{
    if (chr >= '0' && chr <= '9')
        chr -= '0';
    else if (chr >= 'A' && chr <= 'F')
        chr -= ('A' - 10);
    else if (chr >= 'a' && chr <= 'f')
        chr -= ('a' - 10);

    return chr;
}

/** 
 *  @brief convert string to hex integer
 *
 *  @param s		A pointer string buffer
 *  @return            	hex integer
 */
static u32
a2hex(s8 * s)
{
    u32 val = 0;
    while (*s && isxdigit(*s)) {
        val = (val << 4) + hexc2bin(*s++);
    }

    return val;
}

/* 
 *  @brief convert String to integer
 *  
 *  @param value	A pointer to string
 *  @return             integer
 */
static u32
a2hex_or_atoi(s8 * value)
{
    if (value[0] == '0' && (value[1] == 'X' || value[1] == 'x'))
        return a2hex(value + 2);
    else
        return atoi(value);
}

/** 
 *  @brief convert string to integer
 * 
 *  @param ptr		A pointer to data buffer
 *  @param chr 		A pointer to return integer
 *  @return      	A pointer to next data field
 */
s8 *
convert2hex(s8 * ptr, u8 * chr)
{
    u8 val;

    for (val = 0; *ptr && isxdigit(*ptr); ptr++) {
        val = (val * 16) + hexval(*ptr);
    }

    *chr = val;

    return ptr;
}

/** 
 *  @brief Get private info.
 *   
 *  @param ifname   A pointer to net name
 *  @return 	    WLAN_STATUS_SUCCESS--success, otherwise --fail
 */
static int
get_private_info(const s8 * ifname)
{
    /* This function sends the SIOCGIWPRIV command which is
     * handled by the kernel. and gets the total number of
     * private ioctl's available in the host driver.
     */
    struct iwreq iwr;
    int s, ret = WLAN_STATUS_SUCCESS;
    struct iw_priv_args *pPriv = Priv_args;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket[PF_INET,SOCK_DGRAM]");
        return WLAN_STATUS_FAILURE;
    }

    memset(&iwr, 0, sizeof(iwr));
    strncpy(iwr.ifr_name, ifname, IFNAMSIZ);
    iwr.u.data.pointer = (caddr_t) pPriv;
    iwr.u.data.length = IW_MAX_PRIV_DEF;
    iwr.u.data.flags = 0;

    if (ioctl(s, SIOCGIWPRIV, &iwr) < 0) {
        perror("ioctl[SIOCGIWPRIV]");
        ret = WLAN_STATUS_FAILURE;
    } else {
        /* Return the number of private ioctls */
        ret = iwr.u.data.length;
    }

    close(s);

    return ret;
}

/** 
 *  @brief Get Sub command ioctl number
 *   
 *  @param i        command index
 *  @param priv_cnt     Total number of private ioctls availabe in driver
 *  @param ioctl_val    A pointer to return ioctl number
 *  @param subioctl_val A pointer to return sub-ioctl number
 *  @return 	        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
marvell_get_subioctl_no(s32 i,
                        s32 priv_cnt, int *ioctl_val, int *subioctl_val)
{
    s32 j;

    if (Priv_args[i].cmd >= SIOCDEVPRIVATE) {
        *ioctl_val = Priv_args[i].cmd;
        *subioctl_val = 0;
        return WLAN_STATUS_SUCCESS;
    }

    j = -1;

    /* Find the matching *real* ioctl */

    while ((++j < priv_cnt)
           && ((Priv_args[j].name[0] != '\0') ||
               (Priv_args[j].set_args != Priv_args[i].set_args) ||
               (Priv_args[j].get_args != Priv_args[i].get_args))) {
    }

    /* If not found... */
    if (j == priv_cnt) {
        printf("%s: Invalid private ioctl definition for: 0x%x\n",
               dev_name, Priv_args[i].cmd);
        return WLAN_STATUS_FAILURE;
    }

    /* Save ioctl numbers */
    *ioctl_val = Priv_args[j].cmd;
    *subioctl_val = Priv_args[i].cmd;

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get ioctl number
 *   
 *  @param ifname   	A pointer to net name
 *  @param priv_cmd	A pointer to priv command buffer
 *  @param ioctl_val    A pointer to return ioctl number
 *  @param subioctl_val A pointer to return sub-ioctl number
 *  @return 	        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
marvell_get_ioctl_no(const s8 * ifname,
                     const s8 * priv_cmd, int *ioctl_val, int *subioctl_val)
{
    s32 i;
    s32 priv_cnt;

    priv_cnt = get_private_info(ifname);

    /* Are there any private ioctls? */
    if (priv_cnt <= 0 || priv_cnt > IW_MAX_PRIV_DEF) {
        /* Could skip this message ? */
        printf("%-8.8s  no private ioctls.\n", ifname);
    } else {
        for (i = 0; i < priv_cnt; i++) {
            if (Priv_args[i].name[0] && !strcmp(Priv_args[i].name, priv_cmd)) {
                return marvell_get_subioctl_no(i, priv_cnt,
                                               ioctl_val, subioctl_val);
            }
        }
    }

    return WLAN_STATUS_FAILURE;
}

/** 
 *  @brief Retrieve the ioctl and sub-ioctl numbers for the given ioctl string
 *   
 *  @param ifname       Private IOCTL string name
 *  @param ioctl_val    A pointer to return ioctl number
 *  @param subioctl_val A pointer to return sub-ioctl number
 *
 *  @return             WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
get_priv_ioctl(char *ioctl_name, int *ioctl_val, int *subioctl_val)
{
    int retval;

    retval = marvell_get_ioctl_no(dev_name,
                                  ioctl_name, ioctl_val, subioctl_val);

#if 0
    /* Debug print discovered IOCTL values */
    printf("ioctl %s: %x, %x\n", ioctl_name, *ioctl_val, *subioctl_val);
#endif

    return retval;
}

/** 
 *  @brief  get range 
 *   
 *  @return	WLAN_STATUS_SUCCESS--success, otherwise --fail
 */
static int
get_range(void)
{
    struct iw_range *range;
    struct iwreq iwr;
    size_t buflen;
    WCON_HANDLE mhandle, *pHandle = &mhandle;

    buflen = sizeof(struct iw_range) + 500;
    range = malloc(buflen);
    if (range == NULL)
        return WLAN_STATUS_FAILURE;
    memset(range, 0, buflen);

    memset(pHandle, 0, sizeof(WCON_HANDLE));
    memset(&iwr, 0, sizeof(struct iwreq));

    iwr.u.data.pointer = (caddr_t) range;
    iwr.u.data.length = buflen;

    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);

    if ((ioctl(sockfd, SIOCGIWRANGE, &iwr)) < 0) {
        printf("Get Range Results Failed\n");
        free(range);
        return WLAN_STATUS_FAILURE;
    }
    we_version_compiled = range->we_version_compiled;
    printf("Driver build with Wireless Extension %d\n",
           range->we_version_compiled);
    free(range);
    return WLAN_STATUS_SUCCESS;
}

#define WLAN_MAX_RATES	14
#define	GIGA		1e9
#define	MEGA		1e6
#define	KILO		1e3

/** 
 *  @brief print bit rate
 *   
 *  @param rate  	rate to be print
 *  @param current      if current is TRUE, data rate not need convert
 *  @param fixed        not used
 *  @return 	        WLAN_STATUS_SUCCESS
 */
static int
print_bitrate(double rate, s32 current, s32 fixed)
{
    s8 scale = 'k', buf[128];
    s32 divisor = KILO;

    if (!current)
        rate *= 500000;

    if (rate >= GIGA) {
        scale = 'G';
        divisor = GIGA;
    } else if (rate >= MEGA) {
        scale = 'M';
        divisor = MEGA;
    }

    snprintf(buf, sizeof(buf), "%g %cb/s", rate / divisor, scale);

    if (current) {
        printf("\t  Current Bit Rate%c%s\n\n", (fixed) ? '=' : ':', buf);
    } else {
        printf("\t  %s\n", buf);
    }

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief get hostcmd data
 *  
 *  @param fp			A pointer to file stream
 *  @param ln			A pointer to line number
 *  @param buf			A pointer to hostcmd data
 *  @param size			A pointer to the return size of hostcmd buffer
 *  @return      		WLAN_STATUS_SUCCESS
 */
static int
wlan_get_hostcmd_data(FILE * fp, int *ln, u8 * buf, u16 * size)
{
    s32 errors = 0, i;
    s8 line[256], *pos, *pos1, *pos2, *pos3;
    u16 len;

    while ((pos = wlan_config_get_line(line, sizeof(line), fp, ln))) {
        (*ln)++;
        if (strcmp(pos, "}") == 0) {
            break;
        }

        pos1 = strchr(pos, ':');
        if (pos1 == NULL) {
            printf("Line %d: Invalid hostcmd line '%s'\n", *ln, pos);
            errors++;
            continue;
        }
        *pos1++ = '\0';

        pos2 = strchr(pos1, '=');
        if (pos2 == NULL) {
            printf("Line %d: Invalid hostcmd line '%s'\n", *ln, pos);
            errors++;
            continue;
        }
        *pos2++ = '\0';

        len = a2hex_or_atoi(pos1);
        if (len < 1 || len > MRVDRV_SIZE_OF_CMD_BUFFER) {
            printf("Line %d: Invalid hostcmd line '%s'\n", *ln, pos);
            errors++;
            continue;
        }

        *size += len;

        if (*pos2 == '"') {
            pos2++;
            if ((pos3 = strchr(pos2, '"')) == NULL) {
                printf("Line %d: invalid quotation '%s'\n", *ln, pos);
                errors++;
                continue;
            }
            *pos3 = '\0';
            memset(buf, 0, len);
            memmove(buf, pos2, MIN(strlen(pos2), len));
            buf += len;
        } else if (*pos2 == '\'') {
            pos2++;
            if ((pos3 = strchr(pos2, '\'')) == NULL) {
                printf("Line %d: invalid quotation '%s'\n", *ln, pos);
                errors++;
                continue;
            }
            *pos3 = ',';
            for (i = 0; i < len; i++) {
                if ((pos3 = strchr(pos2, ',')) != NULL) {
                    *pos3 = '\0';
                    *buf++ = (u8) a2hex_or_atoi(pos2);
                    pos2 = pos3 + 1;
                } else
                    *buf++ = 0;
            }
        } else if (*pos2 == '{') {
            u16 *tlvlen = (u16 *) buf;
            wlan_get_hostcmd_data(fp, ln, buf + len, tlvlen);
            *size += *tlvlen;
            buf += len + *tlvlen;
        } else {
            u32 value = a2hex_or_atoi(pos2);
            while (len--) {
                *buf++ = (u8) (value & 0xff);
                value >>= 8;
            }
        }
    }
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Process host_cmd 
 *  @param argc		number of arguments
 *  @param argv		A pointer to arguments array    
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_host_cmd(int argc, char *argv[])
{
    s8 line[256], cmdname[256], *pos;
    u8 *buf;
    FILE *fp;
    HostCmd_DS_GEN *hostcmd;
    struct iwreq iwr;
    int ln = 0;
    int cmdname_found = 0, cmdcode_found = 0;
    int ret = WLAN_STATUS_SUCCESS;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("hostcmd",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    if (argc < 5) {
        printf("Error: invalid no of arguments\n");
        printf
            ("Syntax: ./wlanconfig eth1 hostcmd <hostcmd.conf> <cmdname>\n");
        exit(1);
    }

    if ((fp = fopen(argv[3], "r")) == NULL) {
        fprintf(stderr, "Cannot open file %s\n", argv[3]);
        exit(1);
    }

    buf = (u8 *) malloc(MRVDRV_SIZE_OF_CMD_BUFFER);
    if (buf == NULL) {
        printf("Error: allocate memory for hostcmd failed\n");
        return -ENOMEM;
    }
    memset(buf, 0, MRVDRV_SIZE_OF_CMD_BUFFER);
    hostcmd = (PHostCmd_DS_GEN) buf;

    hostcmd->Command = 0xffff;

    sprintf(cmdname, "%s={", argv[4]);
    cmdname_found = 0;
    while ((pos = wlan_config_get_line(line, sizeof(line), fp, &ln))) {
        if (strcmp(pos, cmdname) == 0) {
            cmdname_found = 1;
            sprintf(cmdname, "CmdCode=");
            cmdcode_found = 0;
            while ((pos = wlan_config_get_line(line, sizeof(line), fp, &ln))) {
                if (strncmp(pos, cmdname, strlen(cmdname)) == 0) {
                    cmdcode_found = 1;
                    hostcmd->Command = a2hex_or_atoi(pos + strlen(cmdname));
                    hostcmd->Size = S_DS_GEN;
                    wlan_get_hostcmd_data(fp, &ln, buf + hostcmd->Size,
                                          &hostcmd->Size);
                    break;
                }
            }
            if (!cmdcode_found) {
                fprintf(stderr,
                        "wlanconfig: CmdCode not found in file '%s'\n",
                        argv[3]);
            }
            break;
        }
    }

    fclose(fp);

    if (!cmdname_found)
        fprintf(stderr, "wlanconfig: cmdname '%s' not found in file '%s'\n",
                argv[4], argv[3]);

    if (!cmdname_found || !cmdcode_found) {
        ret = -1;
        goto _exit_;
    }

    buf = (u8 *) hostcmd;

    hostcmd->SeqNum = 0;
    hostcmd->Result = 0;

    memset(&iwr, 0, sizeof(iwr));
    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = (u8 *) hostcmd;
    iwr.u.data.length = hostcmd->Size;

    iwr.u.data.flags = 0;
    if (ioctl(sockfd, ioctl_val, &iwr)) {
        fprintf(stderr, "wlanconfig: WLANHOSTCMD is not supported by %s\n",
                dev_name);
        ret = -1;
        goto _exit_;
    }

    if (!hostcmd->Result) {
        switch (hostcmd->Command) {
        case HostCmd_RET_MEM_ACCESS:
            {
                PHostCmd_DS_MEM_ACCESS ma =
                    (PHostCmd_DS_MEM_ACCESS) (buf + S_DS_GEN);
                if (ma->Action == HostCmd_ACT_GET) {
                    printf("Address: %#08lx   Value=%#08lx\n", ma->Addr,
                           ma->Value);
                }
                break;
            }
        case HostCmd_RET_802_11_POWER_ADAPT_CFG_EXT:
            {
                PHostCmd_DS_802_11_POWER_ADAPT_CFG_EXT pace =
                    (PHostCmd_DS_802_11_POWER_ADAPT_CFG_EXT) (buf + S_DS_GEN);
                int i, j;
                printf("EnablePA=%#04x\n", pace->EnablePA);
                for (i = 0;
                     i <
                     pace->PowerAdaptGroup.Header.Len / sizeof(PA_Group_t);
                     i++) {
                    printf("PA Group #%d: level=%ddbm, Rate Bitmap=%#04x (",
                           i,
                           pace->PowerAdaptGroup.PA_Group[i].PowerAdaptLevel,
                           pace->PowerAdaptGroup.PA_Group[i].RateBitmap);
                    for (j =
                         8 *
                         sizeof(pace->PowerAdaptGroup.PA_Group[i].
                                RateBitmap) - 1; j >= 0; j--) {
                        if ((j == 4) || (j >= 13))      // reserved
                            continue;
                        if (pace->PowerAdaptGroup.PA_Group[i].
                            RateBitmap & (1 << j))
                            printf("%s ", rate_bitmap[j]);
                    }
                    printf("Mbps)\n");
                }
                break;
            }
        case HostCmd_RET_802_11_SUBSCRIBE_EVENT:
            {
                HostCmd_DS_802_11_SUBSCRIBE_EVENT *se =
                    (HostCmd_DS_802_11_SUBSCRIBE_EVENT *) (buf + S_DS_GEN);
                if (se->Action == HostCmd_ACT_GET) {
                    int len =
                        S_DS_GEN + sizeof(HostCmd_DS_802_11_SUBSCRIBE_EVENT);
                    printf("\nEvent\t\tValue\tFreq\tsubscribed\n\n");
                    while (len < hostcmd->Size) {
                        MrvlIEtypesHeader_t *header =
                            (MrvlIEtypesHeader_t *) (buf + len);
                        switch (header->Type) {
                        case TLV_TYPE_RSSI_LOW:
                            {
                                MrvlIEtypes_RssiParamSet_t *LowRssi =
                                    (MrvlIEtypes_RssiParamSet_t *) (buf +
                                                                    len);
                                printf("Low RSSI\t%d\t%d\t%s\n",
                                       LowRssi->RSSIValue, LowRssi->RSSIFreq,
                                       (se->Events & 0x0001) ? "yes" : "no");
                                len += sizeof(MrvlIEtypes_RssiParamSet_t);
                                break;
                            }
                        case TLV_TYPE_SNR_LOW:
                            {
                                MrvlIEtypes_SnrThreshold_t *LowSnr =
                                    (MrvlIEtypes_SnrThreshold_t *) (buf +
                                                                    len);
                                printf("Low SNR\t\t%d\t%d\t%s\n",
                                       LowSnr->SNRValue, LowSnr->SNRFreq,
                                       (se->Events & 0x0002) ? "yes" : "no");
                                len += sizeof(MrvlIEtypes_SnrThreshold_t);
                                break;
                            }
                        case TLV_TYPE_FAILCOUNT:
                            {
                                MrvlIEtypes_FailureCount_t *FailureCount =
                                    (MrvlIEtypes_FailureCount_t *) (buf +
                                                                    len);
                                printf("Failure Count\t%d\t%d\t%s\n",
                                       FailureCount->FailValue,
                                       FailureCount->FailFreq,
                                       (se->Events & 0x0004) ? "yes" : "no");
                                len += sizeof(MrvlIEtypes_FailureCount_t);
                                break;
                            }
                        case TLV_TYPE_BCNMISS:
                            {
                                MrvlIEtypes_BeaconsMissed_t *BcnMissed =
                                    (MrvlIEtypes_BeaconsMissed_t *) (buf +
                                                                     len);
                                printf("Beacon Missed\t%d\tN/A\t%s\n",
                                       BcnMissed->BeaconMissed,
                                       (se->Events & 0x0008) ? "yes" : "no");
                                len += sizeof(MrvlIEtypes_BeaconsMissed_t);
                                break;
                            }
                        case TLV_TYPE_RSSI_HIGH:
                            {
                                MrvlIEtypes_RssiParamSet_t *HighRssi =
                                    (MrvlIEtypes_RssiParamSet_t *) (buf +
                                                                    len);
                                printf("High RSSI\t%d\t%d\t%s\n",
                                       HighRssi->RSSIValue,
                                       HighRssi->RSSIFreq,
                                       (se->Events & 0x0010) ? "yes" : "no");
                                len += sizeof(MrvlIEtypes_RssiParamSet_t);
                                break;
                            }
                        case TLV_TYPE_SNR_HIGH:
                            {
                                MrvlIEtypes_SnrThreshold_t *HighSnr =
                                    (MrvlIEtypes_SnrThreshold_t *) (buf +
                                                                    len);
                                printf("High SNR\t%d\t%d\t%s\n",
                                       HighSnr->SNRValue, HighSnr->SNRFreq,
                                       (se->Events & 0x0020) ? "yes" : "no");
                                len += sizeof(MrvlIEtypes_SnrThreshold_t);
                                break;
                            }
                        default:
                            printf
                                ("unknown subscribed event TLV Type=%#x, Len=%d\n",
                                 header->Type, header->Len);
                            len += sizeof(MrvlIEtypesHeader_t) + header->Len;
                            break;
                        }
                    }
                }
                break;
            }
        case HostCmd_RET_802_11_AUTO_TX:
            {
                PHostCmd_DS_802_11_AUTO_TX at =
                    (PHostCmd_DS_802_11_AUTO_TX) (buf + S_DS_GEN);
                if (at->Action == HostCmd_ACT_GET) {
                    if (S_DS_GEN + sizeof(at->Action) == hostcmd->Size) {
                        printf("auto_tx not configured\n");
                    } else {
                        MrvlIEtypesHeader_t *header = &at->AutoTx.Header;
                        if ((S_DS_GEN + sizeof(at->Action) +
                             sizeof(MrvlIEtypesHeader_t) + header->Len ==
                             hostcmd->Size) &&
                            (header->Type == TLV_TYPE_AUTO_TX)) {
                            AutoTx_MacFrame_t *atmf =
                                &at->AutoTx.AutoTx_MacFrame;

                            printf("Interval: %d second(s)\n",
                                   atmf->Interval);
                            printf("Priority: %#x\n", atmf->Priority);

                            printf("Frame Length: %d\n", atmf->FrameLen);
                            printf
                                ("Dest Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                                 atmf->DestMacAddr[0], atmf->DestMacAddr[1],
                                 atmf->DestMacAddr[2], atmf->DestMacAddr[3],
                                 atmf->DestMacAddr[4], atmf->DestMacAddr[5]);
                            printf
                                ("Src Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                                 atmf->SrcMacAddr[0], atmf->SrcMacAddr[1],
                                 atmf->SrcMacAddr[2], atmf->SrcMacAddr[3],
                                 atmf->SrcMacAddr[4], atmf->SrcMacAddr[5]);

                            hexdump("Frame Payload", atmf->Payload,
                                    atmf->FrameLen - ETH_ALEN * 2, ' ');
                        } else {
                            printf("incorrect auto_tx command response\n");
                        }
                    }
                }
                break;
            }
        default:
            printf("HOSTCMD_RESP: ReturnCode=%#04x, Result=%#04x\n",
                   hostcmd->Command, hostcmd->Result);
            break;
        }
    } else {
        printf("HOSTCMD failed: ReturnCode=%#04x, Result=%#04x\n",
               hostcmd->Command, hostcmd->Result);
    }

  _exit_:
    if (buf)
        free(buf);

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Rate
 *   
 *  @return      WLAN_STATUS_SUCCESS--success, otherwise --fail
 */
static int
process_get_rate(void)
{
    u32 bitrate[WLAN_MAX_RATES];
    struct iwreq iwr;
    s32 i = 0;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("getrate",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    memset(&iwr, 0, sizeof(iwr));
    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = (caddr_t) bitrate;
    iwr.u.data.length = sizeof(bitrate);
    iwr.u.data.flags = subioctl_val;

    if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
        perror("wlanconfig");
        return WLAN_STATUS_FAILURE;
    }

    printf("%-8.16s  %d available bit-rates :\n",
           dev_name, iwr.u.data.length);

    for (i = 0; i < iwr.u.data.length; i++) {
        print_bitrate(bitrate[i], 0, 0);
    }

    if (ioctl(sockfd, SIOCGIWRATE, &iwr)) {
        perror("wlanconfig");
        return WLAN_STATUS_FAILURE;
    }

    print_bitrate(iwr.u.bitrate.value, 1, iwr.u.bitrate.fixed);

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Check the Hex String
 *  @param s		A pointer to the string      
 *  @return      	0--HexString, -1--not HexString
 */
static int
ishexstring(s8 * s)
{
    int ret = -1;
    s32 tmp;

    while (*s) {
        tmp = toupper(*s);
        if (tmp >= 'A' && tmp <= 'F') {
            ret = 0;
            break;
        }
        s++;
    }

    return ret;
}

/** 
 *  @brief Convert String to Integer
 *  @param buf		A pointer to the string      
 *  @return      	Integer
 */
static int
atoval(s8 * buf)
{
    if (!strncasecmp(buf, "0x", 2))
        return a2hex(buf + 2);
    else if (!ishexstring(buf))
        return a2hex(buf);
    else
        return atoi(buf);
}

/** 
 *  @brief Display sleep params
 *  @param sp		A pointer to wlan_ioctl_sleep_params_config structure    
 *  @return      	NA
 */
void
display_sleep_params(wlan_ioctl_sleep_params_config * sp)
{
    printf("Sleep Params for %s:\n", sp->Action ? "set" : "get");
    printf("----------------------------------------\n");
    printf("Error		: %u\n", sp->Error);
    printf("Offset		: %u\n", sp->Offset);
    printf("StableTime	: %u\n", sp->StableTime);
    printf("CalControl	: %u\n", sp->CalControl);
    printf("ExtSleepClk	: %u\n", sp->ExtSleepClk);
    printf("Reserved	: %u\n", sp->Reserved);
}

/** 
 *  @brief Process sleep params
 *  @param argc		number of arguments
 *  @param argv         A pointer to arguments array    
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_sleep_params(int argc, char *argv[])
{
    struct iwreq iwr;
    int ret;
    wlan_ioctl_sleep_params_config sp;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("sleepparams",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    if (argc < 4) {
        printf("Error: invalid no of arguments\n");
        printf("Syntax: ./wlanconfig eth1 sleepparams get/set <p1>"
               " <p2> <p3> <p4> <p5> <p6>\n");
        exit(1);
    }

    memset(&sp, 0, sizeof(wlan_ioctl_sleep_params_config));
    if (!strcmp(argv[3], "get")) {
        sp.Action = 0;
    } else if (!strncmp(argv[3], "set", 3)) {
        if (argc != 10) {
            printf("Error: invalid no of arguments\n");
            printf("Syntax: ./wlanconfig eth1 sleepparams get/set"
                   "<p1> <p2> <p3> <p4> <p5> <p6>\n");
            exit(1);
        }

        sp.Action = 1;
        if ((ret = atoval(argv[4])) < 0)
            return -EINVAL;
        sp.Error = (u16) ret;
        if ((ret = atoval(argv[5])) < 0)
            return -EINVAL;
        sp.Offset = (u16) ret;
        if ((ret = atoval(argv[6])) < 0)
            return -EINVAL;
        sp.StableTime = (u16) ret;
        if ((ret = atoval(argv[7])) < 0)
            return -EINVAL;
        sp.CalControl = (u8) ret;
        if ((ret = atoval(argv[8])) < 0)
            return -EINVAL;
        sp.ExtSleepClk = (u8) ret;
        if ((ret = atoval(argv[9])) < 0)
            return -EINVAL;
        sp.Reserved = (u16) ret;
    } else {
        return -EINVAL;
    }

    memset(&iwr, 0, sizeof(iwr));

    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = (caddr_t) & sp;
    iwr.u.data.length = sizeof(wlan_ioctl_sleep_params_config);
    iwr.u.data.flags = subioctl_val;

    if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
        perror("wlanconfig");
        return -1;
    }

    display_sleep_params(&sp);

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Display BCA Time Share Params
 *  @param sp		A point to wlan_ioctl_bca_timeshare_config structure    
 *  @return      	NA
 */
static void
display_bca_ts_params(wlan_ioctl_bca_timeshare_config * bca_ts)
{
    printf("BCA Time Share Params for %s:\n", bca_ts->Action ? "set" : "get");
    printf("----------------------------------------\n");
    printf("TrafficType		: %u\n", bca_ts->TrafficType);
    printf("TimeShareInterval	: %lu\n", bca_ts->TimeShareInterval);
    printf("BTTime			: %lu\n", bca_ts->BTTime);
}

/** 
 *  @brief Process BCA Time Share Params
 *  @param argc		number of arguments
 *  @param argv         A pointer to arguments array    
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_bca_ts(int argc, char *argv[])
{
    int ret, i;
    struct iwreq iwr;
    wlan_ioctl_bca_timeshare_config bca_ts;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("bca-ts",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    if (argc < 5) {
        printf("Error: invalid no of arguments\n");
        printf("Syntax: ./wlanconfig eth1 bca_ts get/set <p1>"
               " <p2> <p3>\n");
        exit(1);
    }

    memset(&bca_ts, 0, sizeof(wlan_ioctl_bca_timeshare_config));

    if ((ret = atoval(argv[4])) < 0)
        return -EINVAL;
    if (ret > 1)
        return -EINVAL;
    bca_ts.TrafficType = (u16) ret;     // 0 or 1

    if (!strcmp(argv[3], "get")) {
        bca_ts.Action = 0;
    } else if (!strncmp(argv[3], "set", 3)) {
        if (argc != 7) {
            printf("Error: invalid no of arguments\n");
            printf("Syntax: ./wlanconfig eth1 bca_ts get/set"
                   " <p1> <p2> <p3>\n");
            exit(1);
        }

        bca_ts.Action = 1;

        if ((ret = atoval(argv[5])) < 0)
            return -EINVAL;
        /* If value is not multiple of 10 then take the floor value */
        i = ret % 10;
        ret -= i;
        /* Valid Range for TimeShareInterval: < 20 ... 60_000 > ms */
        if (ret < 20 || ret > 60000) {
            printf("Invalid TimeShareInterval Range:"
                   " < 20 ... 60000 > ms\n");
            return -EINVAL;
        }
        bca_ts.TimeShareInterval = (u32) ret;

        if ((ret = atoval(argv[6])) < 0)
            return -EINVAL;
        /* If value is not multiple of 10 then take the floor value */
        i = ret % 10;
        ret -= i;

        if (ret > bca_ts.TimeShareInterval) {
            printf("Invalid BTTime"
                   "  Range: < 0 .. TimeShareInterval > ms\n");
            return -EINVAL;
        }
        bca_ts.BTTime = (u32) ret;
    } else {
        return -EINVAL;
    }

    memset(&iwr, 0, sizeof(iwr));

    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = (caddr_t) & bca_ts;
    iwr.u.data.length = sizeof(wlan_ioctl_bca_timeshare_config);
    iwr.u.data.flags = subioctl_val;

    if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
        perror("wlanconfig");
        return -1;
    }

    display_bca_ts_params(&bca_ts);

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Process reassoication
 *
 *  @param argc     number of arguments
 *  @param argv     A pointer to arguments array    
 *
 *  @return         WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_reassociation(int argc, char *argv[])
{
    wlan_ioctl_reassociation_info reassocInfo;
    struct iwreq iwr;
    unsigned int mac[MRVDRV_ETH_ADDR_LEN];
    u32 idx;
    s32 numToks;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("reassociate",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    /*
     * Reassociation request is expected to be in the following format:
     *
     *      <xx:xx:xx:xx:xx:xx>      <yy:yy:yy:yy:yy:yy>  <ssid string>
     *
     *      where xx..xx is the current AP BSSID to be included in the reassoc req
     *                yy..yy is the desired AP to send the reassoc req to
     *                <ssid string> is the desired AP SSID to send the reassoc req to.
     *
     *      The current and desired AP BSSIDs are required.  
     *      The SSID string can be omitted if the desired BSSID is provided.
     *
     *      If we fail to find the desired BSSID, we attempt the SSID.
     *      If the desired BSSID is set to all 0's, the ssid string is used.
     *      
     */

    /* Verify the number of arguments is either 5 or 6 */
    if (argc != 5 && argc != 6) {
        fprintf(stderr, "Invalid number of parameters!\n");
        return -EINVAL;
    }

    memset(&iwr, 0, sizeof(iwr));
    memset(&reassocInfo, 0x00, sizeof(reassocInfo));

    /*
     *      Scan in and set the current AP BSSID
     */
    numToks = sscanf(argv[3], "%2x:%2x:%2x:%2x:%2x:%2x",
                     mac + 0, mac + 1, mac + 2, mac + 3, mac + 4, mac + 5);

    if (numToks != 6) {
        fprintf(stderr, "Invalid number of parameters!\n");
        return -EINVAL;
    }

    for (idx = 0; idx < NELEMENTS(mac); idx++) {
        reassocInfo.CurrentBSSID[idx] = (u8) mac[idx];
    }

    /*
     *      Scan in and set the desired AP BSSID
     */
    numToks = sscanf(argv[4], "%2x:%2x:%2x:%2x:%2x:%2x",
                     mac + 0, mac + 1, mac + 2, mac + 3, mac + 4, mac + 5);

    if (numToks != 6) {
        fprintf(stderr, "Invalid number of parameters!\n");
        return -EINVAL;
    }

    for (idx = 0; idx < NELEMENTS(mac); idx++) {
        reassocInfo.DesiredBSSID[idx] = (u8) mac[idx];
    }

    /*
     * If the ssid string is provided, save it; otherwise it is an empty string
     */
    if (argc == 6) {
        strcpy(reassocInfo.DesiredSSID, argv[5]);
    }

    /* 
     * Set up and execute the ioctl call
     */
    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = (caddr_t) & reassocInfo;
    iwr.u.data.length = sizeof(reassocInfo);
    iwr.u.data.flags = subioctl_val;

    if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
        perror("wlanconfig: reassociate ioctl");
        return -EFAULT;
    }

    /* No error return */
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Provision the driver with a IEEE IE for use in the next join cmd
 *
 *    Test function used to check the ioctl and driver funcionality 
 *  
 *  @return WLAN_STATUS_SUCCESS or ioctl error code
 */
static int
process_setgenie(void)
{
    int ioctl_val, subioctl_val;
    struct iwreq iwr;

    u8 testIE[] = { 0xdd, 0x09,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99
    };

    if (get_priv_ioctl("setgenie",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = (caddr_t) testIE;
    iwr.u.data.length = sizeof(testIE);
    iwr.u.data.flags = subioctl_val;

    if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
        perror("wlanconfig: setgenie ioctl");
        return -EFAULT;
    }

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Retrieve and display the contents of the driver scan table.
 *
 *  The ioctl to retrieve the scan table contents will be invoked, and portions
 *   of the scan data will be displayed on stdout.  The entire beacon or 
 *   probe response is also retrieved (if available in the driver).  This 
 *   data would be needed in case the application was explicitly controlling
 *   the association (inserting IEs, TLVs, etc).
 *
 *  @param argc     number of arguments
 *  @param argv     A pointer to arguments array    
 *
 *  @return         WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_getscantable(int argc, char *argv[])
{
    int ioctl_val, subioctl_val;
    struct iwreq iwr;
    u8 scanRspBuffer[500];      /* Stack buffer can be as large as ioctl allows */

    uint scanStart;
    uint idx;

    u8 *pCurrent;
    u8 *pNext;
    IEEEtypes_ElementId_e *pElementId;
    u8 *pElementLen;
    int bssInfoLen;
    int ssidIdx;
    u16 tmpCap;
    u8 *pByte;

    IEEEtypes_CapInfo_t capInfo = { 0 };
    u8 tsf[8];
    u16 beaconInterval;

    wlan_ioctl_get_scan_table_info *pRspInfo;
    wlan_ioctl_get_scan_table_entry *pRspEntry;

    pRspInfo = (wlan_ioctl_get_scan_table_info *) scanRspBuffer;

    if (get_priv_ioctl("getscantable",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    scanStart = 0;

    printf("---------------------------------------");
    printf("---------------------------------------\n");
    printf("# | ch  | ss  |       bssid       |   cap    |   SSID \n");
    printf("---------------------------------------");
    printf("---------------------------------------\n");

    do {
        pRspInfo->scanNumber = scanStart;

        /* 
         * Set up and execute the ioctl call
         */
        strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
        iwr.u.data.pointer = (caddr_t) pRspInfo;
        iwr.u.data.length = sizeof(scanRspBuffer);
        iwr.u.data.flags = subioctl_val;

        if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
            perror("wlanconfig: getscantable ioctl");
            return -EFAULT;
        }

        pCurrent = 0;
        pNext = pRspInfo->scan_table_entry_buffer;

        for (idx = 0; idx < pRspInfo->scanNumber; idx++) {

            /* 
             * Set pCurrent to pNext in case pad bytes are at the end
             *   of the last IE we processed.
             */
            pCurrent = pNext;

            pRspEntry = (wlan_ioctl_get_scan_table_entry *) pCurrent;

            printf("%02u| %03d | %03d | %02x:%02x:%02x:%02x:%02x:%02x |",
                   scanStart + idx,
                   pRspEntry->fixedFields.channel,
                   pRspEntry->fixedFields.rssi,
                   pRspEntry->fixedFields.bssid[0],
                   pRspEntry->fixedFields.bssid[1],
                   pRspEntry->fixedFields.bssid[2],
                   pRspEntry->fixedFields.bssid[3],
                   pRspEntry->fixedFields.bssid[4],
                   pRspEntry->fixedFields.bssid[5]);

#if 0
            printf("fixed = %u, bssInfo = %u\n",
                   (unsigned int) pRspEntry->fixedFieldLength,
                   (unsigned int) pRspEntry->bssInfoLength);
#endif

            pCurrent += (sizeof(pRspEntry->fixedFieldLength) +
                         pRspEntry->fixedFieldLength);

            bssInfoLen = pRspEntry->bssInfoLength;
            pCurrent += sizeof(pRspEntry->bssInfoLength);
            pNext = pCurrent + pRspEntry->bssInfoLength;

            if (bssInfoLen >= (sizeof(tsf)
                               + sizeof(beaconInterval) + sizeof(capInfo))) {
                /* time stamp is 8 byte long */
                memcpy(tsf, pCurrent, sizeof(tsf));
                pCurrent += sizeof(tsf);
                bssInfoLen -= sizeof(tsf);

                /* beacon interval is 2 byte long */
                memcpy(&beaconInterval, pCurrent, sizeof(beaconInterval));
                pCurrent += sizeof(beaconInterval);
                bssInfoLen -= sizeof(beaconInterval);

                /* capability information is 2 byte long */
                memcpy(&capInfo, pCurrent, sizeof(capInfo));
                memcpy(&tmpCap, pCurrent, sizeof(tmpCap));
                pCurrent += sizeof(capInfo);
                bssInfoLen -= sizeof(capInfo);

                printf(" %04x-", tmpCap);

                printf("%c%c%c | ",
                       capInfo.Ibss ? 'A' : 'I',
                       capInfo.Privacy ? 'P' : ' ',
                       capInfo.SpectrumMgmt ? 'S' : ' ');
            } else {
                printf("          | ");
            }

            while (bssInfoLen >= 2) {
                pElementId = (IEEEtypes_ElementId_e *) pCurrent;
                pElementLen = pCurrent + 1;
                pCurrent += 2;

                switch (*pElementId) {

                case SSID:
                    if (*pElementLen &&
                        *pElementLen <= MRVDRV_MAX_SSID_LENGTH) {
                        for (ssidIdx = 0; ssidIdx < *pElementLen; ssidIdx++) {
                            if (isprint(*(pCurrent + ssidIdx))) {
                                printf("%c", *(pCurrent + ssidIdx));
                            } else {
                                printf("\\%02x", *(pCurrent + ssidIdx));
                            }
                        }
                    }
                    break;

                default:
#if 0
                    printf("% d(%d), bil=%d\n",
                           *pElementId, *pElementLen, bssInfoLen);
#endif
                    break;
                }

                pCurrent += *pElementLen;
                bssInfoLen -= (2 + *pElementLen);
            }

            printf("\n");

            if (argc > 3) {
                /* TSF is a u64, some formatted printing libs have
                 *   trouble printing long longs, so cast and dump as bytes
                 */
                pByte = (u8 *) & pRspEntry->fixedFields.networkTSF;
                printf("    TSF=%02x%02x%02x%02x%02x%02x%02x%02x\n",
                       pByte[7], pByte[6], pByte[5], pByte[4],
                       pByte[3], pByte[2], pByte[1], pByte[0]);
            }
        }

        scanStart += pRspInfo->scanNumber;

    } while (pRspInfo->scanNumber);

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Request a scan from the driver and display the scan table afterwards
 *
 *  Command line interface for performing a specific immediate scan based
 *    on the following keyword parsing:
 *
 *     chan=[chan#][band][mode] where band is [a,b,g] and mode is 
 *                              blank for active or 'p' for passive
 *     bssid=xx:xx:xx:xx:xx:xx  specify a BSSID filter for the scan
 *     ssid="[SSID]"            specify a SSID filter for the scan
 *     keep=[0 or 1]            keep the previous scan results (1), discard (0)
 *     dur=[scan time]          time to scan for each channel in milliseconds
 *     probes=[#]               number of probe requests to send on each chan
 *     type=[1,2,3]             BSS type: 1 (Infra), 2(Adhoc), 3(Any)
 *
 *  Any combination of the above arguments can be supplied on the command line.
 *    If the chan token is absent, a full channel scan will be completed by 
 *    the driver.  If the dur or probes tokens are absent, the driver default
 *    setting will be used.  The bssid and ssid fields, if blank, 
 *    will produce an unfiltered scan. The type field will default to 3 (Any)
 *    and the keep field will default to 0 (Discard).  
 *
 *  @param argc     number of arguments
 *  @param argv     A pointer to arguments array    
 *
 *  @return         WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_setuserscan(int argc, char *argv[])
{
    wlan_ioctl_user_scan_cfg scanReq;
    int ioctl_val, subioctl_val;
    struct iwreq iwr;
    char *pArgTok;
    char *pChanTok;
    char *pArgCookie;
    char *pChanCookie;
    int argIdx;
    int chanParseIdx;
    int chanCmdIdx;
    char chanScratch[10];
    char *pScratch;
    int tmpIdx;
    unsigned int mac[MRVDRV_ETH_ADDR_LEN];
    int scanTime;

    memset(&scanReq, 0x00, sizeof(scanReq));
    chanCmdIdx = 0;
    scanTime = 0;

    if (get_priv_ioctl("setuserscan",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    for (argIdx = 0; argIdx < argc; argIdx++) {
        if (strncmp(argv[argIdx], "chan=", strlen("chan=")) == 0) {
            /* 
             *  "chan" token string handler
             */
            pArgTok = argv[argIdx] + strlen("chan=");

            while ((pArgTok = strtok_r(pArgTok, ",", &pArgCookie)) != NULL) {

                memset(chanScratch, 0x00, sizeof(chanScratch));
                pScratch = chanScratch;

                for (chanParseIdx = 0;
                     chanParseIdx < strlen(pArgTok); chanParseIdx++) {
                    if (isalpha(*(pArgTok + chanParseIdx))) {
                        *pScratch++ = ' ';
                    }

                    *pScratch++ = *(pArgTok + chanParseIdx);
                }
                *pScratch = 0;
                pArgTok = NULL;

                pChanTok = chanScratch;

                while ((pChanTok = strtok_r(pChanTok, " ",
                                            &pChanCookie)) != NULL) {
                    if (isdigit(*pChanTok)) {
                        scanReq.chanList[chanCmdIdx].chanNumber
                            = atoi(pChanTok);
                    } else {
                        switch (toupper(*pChanTok)) {
                        case 'A':
                            scanReq.chanList[chanCmdIdx].radioType = 1;
                            break;
                        case 'B':
                        case 'G':
                            scanReq.chanList[chanCmdIdx].radioType = 0;
                            break;
                        case 'P':
                            scanReq.chanList[chanCmdIdx].scanType = 1;
                            break;
                        }
                    }
                    pChanTok = NULL;
                }
                chanCmdIdx++;
            }
        } else if (strncmp(argv[argIdx], "bssid=", strlen("bssid=")) == 0) {
            /* 
             *  "bssid" token string handler
             */
            sscanf(argv[argIdx] + strlen("bssid="), "%2x:%2x:%2x:%2x:%2x:%2x",
                   mac + 0, mac + 1, mac + 2, mac + 3, mac + 4, mac + 5);

            for (tmpIdx = 0; tmpIdx < NELEMENTS(mac); tmpIdx++) {
                scanReq.specificBSSID[tmpIdx] = (u8) mac[tmpIdx];
            }
        } else if (strncmp(argv[argIdx], "keep=", strlen("keep=")) == 0) {
            /* 
             *  "keep" token string handler
             */
            scanReq.keepPreviousScan = atoi(argv[argIdx] + strlen("keep="));
        } else if (strncmp(argv[argIdx], "dur=", strlen("dur=")) == 0) {
            /* 
             *  "dur" token string handler
             */
            scanTime = atoi(argv[argIdx] + strlen("dur="));
        } else if (strncmp(argv[argIdx], "ssid=", strlen("ssid=")) == 0) {
            /* 
             *  "ssid" token string handler
             */
            strncpy(scanReq.specificSSID, argv[argIdx] + strlen("ssid="),
                    sizeof(scanReq.specificSSID));
        } else if (strncmp(argv[argIdx], "probes=", strlen("probes=")) == 0) {
            /* 
             *  "probes" token string handler
             */
            scanReq.numProbes = atoi(argv[argIdx] + strlen("probes="));
        } else if (strncmp(argv[argIdx], "type=", strlen("type=")) == 0) {
            /* 
             *  "type" token string handler
             */
            scanReq.bssType = atoi(argv[argIdx] + strlen("type="));
            switch (scanReq.bssType) {
            case WLAN_SCAN_BSS_TYPE_BSS:
            case WLAN_SCAN_BSS_TYPE_IBSS:
                break;

            default:
            case WLAN_SCAN_BSS_TYPE_ANY:
                /* Set any unknown types to ANY */
                scanReq.bssType = WLAN_SCAN_BSS_TYPE_ANY;
            }
        }
    }

    /*
     * Update all the channels to have the same scan time
     */
    for (tmpIdx = 0; tmpIdx < chanCmdIdx; tmpIdx++) {
        scanReq.chanList[tmpIdx].scanTime = scanTime;
    }

    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = (caddr_t) & scanReq;
    iwr.u.data.length = sizeof(scanReq);
    iwr.u.data.flags = subioctl_val;

    if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
        perror("wlanconfig: setuserscan ioctl");
        return -EFAULT;
    }

    process_getscantable(0, 0);

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Provision the driver with a Marvell TLV for use in the next join cmd
 *
 *    Test function used to check the ioctl and driver funcionality 
 *  
 *  @return WLAN_STATUS_SUCCESS or ioctl error code
 */
static int
process_setmrvltlv(void)
{
    int ioctl_val, subioctl_val;
    struct iwreq iwr;

    u8 testTlv[] = { 0x0A, 0x01, 0x0C, 0x00,
        0xdd, 10, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    };

    if (get_priv_ioctl("setmrvltlv",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = (caddr_t) testTlv;
    iwr.u.data.length = sizeof(testTlv);
    iwr.u.data.flags = subioctl_val;

    if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
        perror("wlanconfig: setmrvltlv ioctl");
        return -EFAULT;
    }

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Retrieve the association response from the driver
 *
 *  Retrieve the buffered (re)association management frame from the driver.
 *    The response is identical to the one received from the AP and conforms
 *    to the IEEE specification.
 *
 *  @return WLAN_STATUS_SUCCESS or ioctl error code
 */
static int
process_getassocrsp(void)
{
    int ioctl_val, subioctl_val;
    struct iwreq iwr;
    u8 assocRspBuffer[500];     /* Stack buffer can be as large as ioctl allows */
    IEEEtypes_AssocRsp_t *pAssocRsp;

    pAssocRsp = (IEEEtypes_AssocRsp_t *) assocRspBuffer;

    if (get_priv_ioctl("getassocrsp",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = (caddr_t) assocRspBuffer;
    iwr.u.data.length = sizeof(assocRspBuffer);
    iwr.u.data.flags = subioctl_val;

    if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
        perror("wlanconfig: getassocrsp ioctl");
        return -EFAULT;
    }

    if (iwr.u.data.length) {
        printf("getassocrsp: Status[%d], Cap[0x%04x]: ",
               pAssocRsp->StatusCode, *(u16 *) & pAssocRsp->Capability);
        hexdump(NULL, assocRspBuffer, iwr.u.data.length, ' ');
    } else {
        printf("getassocrsp: <empty>\n");
    }

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief scan network with specific ssid
 *  @param argc		number of arguments
 *  @param argv         A pointer to arguments array    
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_extscan(int argc, char *argv[])
{
    struct iwreq iwr;
    WCON_SSID Ssid;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("extscan",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    if (argc != 4) {
        printf("Error: invalid no of arguments\n");
        printf("Syntax: ./wlanconfig eth1 extscan <SSID>\n");
        exit(1);
    }

    printf("Ssid: %s\n", argv[3]);

    memset(&Ssid, 0, sizeof(Ssid));
    memset(&iwr, 0, sizeof(iwr));

    if (strlen(argv[3]) <= IW_ESSID_MAX_SIZE) {
        Ssid.ssid_len = strlen(argv[3]);
    } else {
        printf("Invalid SSID:Exceeds more than 32 charecters\n");
        exit(1);
    }
    memcpy(Ssid.ssid, argv[3], Ssid.ssid_len);

    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = (caddr_t) & Ssid;
    iwr.u.data.length = sizeof(Ssid);
    iwr.u.data.flags = subioctl_val;

    if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
        perror("wlanconfig");
        return -1;
    }

    return WLAN_STATUS_SUCCESS;
}

#if WIRELESS_EXT > 14
/** 
 *  @brief parse custom info
 *  @param pHandle	A pointer to WCON_HANDLE
 *  @param data         A pointer to iw_point structure
 *  @param idx          AP index
 *  @return      	NA
 */
static void
parse_custom_info(WCON_HANDLE * pHandle, struct iw_point *data, s32 idx)
{
    s32 i = 0;
    s8 *custom_cmd[] = { "wpa_ie", "rsn_ie", NULL };

    if (!data->pointer || !data->length) {
        printf("iw_point: Invalid Pointer/Length\n");
        return;
    }

    if (!strncmp(data->pointer, "wmm_ie", strlen("wmm_ie"))) {
        pHandle->ScanList[idx].Wmm = WCON_WMM_ENABLED;
    }

    while (custom_cmd[i]) {
        if (!strncmp(data->pointer, custom_cmd[i], strlen(custom_cmd[i]))) {
            pHandle->ScanList[idx].WpaAP = WCON_WPA_ENABLED;
            break;
        }
        i++;
    }

    printf("Wpa:\t %s\n", pHandle->ScanList[idx].WpaAP ?
           "enabled" : "disabled");
    printf("Wmm:\t %s\n", pHandle->ScanList[idx].Wmm ?
           "enabled" : "disabled");
}
#endif

/** 
 *  @brief parse scan info
 *  @param pHandle	A pointer to WCON_HANDLE
 *  @param buffer       A pointer to scan result buffer
 *  @param length       length of scan result buffer
 *  @return      	NA
 */
static void
parse_scan_info(WCON_HANDLE * pHandle, u8 buffer[], s32 length)
{
    s32 len = 0;
    u32 ap_index = 0;
    int new_index = FALSE;
    s8 *mode[3] = { "auto", "ad-hoc", "infra" };
    struct iw_event iwe;
    struct iw_point iwp;

    memset(pHandle->ScanList, 0, sizeof(pHandle->ScanList));
    memset((u8 *) & iwe, 0, sizeof(struct iw_event));
    memset((u8 *) & iwp, 0, sizeof(struct iw_point));
    pHandle->ApNum = 0;

    while (len + IW_EV_LCP_LEN < length) {
        memcpy((s8 *) & iwe, buffer + len, sizeof(struct iw_event));
        if ((iwe.cmd == SIOCGIWESSID) || (iwe.cmd == SIOCGIWENCODE) ||
            (iwe.cmd == IWEVCUSTOM)) {
            if (we_version_compiled > 18)
                memcpy((s8 *) & iwp,
                       buffer + len + IW_EV_LCP_LEN - MRV_EV_POINT_OFF,
                       sizeof(struct iw_point));
            else
                memcpy((s8 *) & iwp, buffer + len + IW_EV_LCP_LEN,
                       sizeof(struct iw_point));
            iwp.pointer = buffer + len + IW_EV_POINT_LEN;
        }
        switch (iwe.cmd) {
        case SIOCGIWAP:
            if (new_index && ap_index < IW_MAX_AP - 1)
                ap_index++;
            memcpy(pHandle->ScanList[ap_index].Bssid,
                   iwe.u.ap_addr.sa_data, ETH_ALEN);
            printf("\nBSSID:\t %02X:%02X:%02X:%02X:%02X:%02X\n",
                   HWA_ARG(pHandle->ScanList[ap_index].Bssid));
            new_index = TRUE;
            break;

        case SIOCGIWESSID:
            if ((iwp.pointer) && (iwp.length)) {
                memcpy(pHandle->ScanList[ap_index].Ssid.ssid,
                       (s8 *) iwp.pointer, iwp.length);
                pHandle->ScanList[ap_index].Ssid.ssid_len = iwp.length;
            }
            printf("SSID:\t %s\n", pHandle->ScanList[ap_index].Ssid.ssid);
            break;

        case SIOCGIWENCODE:
            if (!(iwp.flags & IW_ENCODE_DISABLED)) {
                pHandle->ScanList[ap_index].Privacy = WCON_ENC_ENABLED;
            }
            printf("Privacy: %s\n",
                   pHandle->ScanList[ap_index].Privacy ?
                   "enabled" : "disabled");
            break;

        case SIOCGIWMODE:
            pHandle->ScanList[ap_index].NetMode = iwe.u.mode;
            printf("NetMode: %s\n",
                   mode[pHandle->ScanList[ap_index].NetMode]);
            break;

#if WIRELESS_EXT > 14
        case IWEVCUSTOM:
            parse_custom_info(pHandle, &iwp, ap_index);
            break;
#endif

        case IWEVQUAL:
            pHandle->ScanList[ap_index].Rssi = iwe.u.qual.level;
            printf("Quality: %d\n", pHandle->ScanList[ap_index].Rssi);
            break;
        }

        len += iwe.len;
    }
    if (new_index)
        pHandle->ApNum = ap_index + 1;
    printf("\nNo of AP's = %d\n", pHandle->ApNum);

    return;
}

/* 
 *  @brief Process scan results
 *  @param argc		number of arguments
 *  @param argv         A pointer to arguments array    
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_scan_results(int argc, char *argv[])
{
    u8 *buffer = NULL;
    u8 *newbuf = NULL;
    int buflen = IW_SCAN_MAX_DATA;
    struct iwreq iwr;
    WCON_HANDLE mhandle, *pHandle = &mhandle;

    memset(pHandle, 0, sizeof(WCON_HANDLE));
    memset(&iwr, 0, sizeof(struct iwreq));
  realloc:
    /* (Re)allocate the buffer - realloc(NULL, len) == malloc(len) */
    newbuf = realloc(buffer, buflen);
    if (newbuf == NULL) {
        if (buffer)
            free(buffer);
        fprintf(stderr, "%s: Allocation failed\n", __FUNCTION__);
        return (-1);
    }
    buffer = newbuf;

    iwr.u.data.pointer = buffer;
    iwr.u.data.length = buflen;
    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);

    if ((ioctl(sockfd, SIOCGIWSCAN, &iwr)) < 0) {
        if ((errno == E2BIG)) {
            /* Some driver may return very large scan results, either
             * because there are many cells, or because they have many
             * large elements in cells (like IWEVCUSTOM). Most will
             * only need the regular sized buffer. We now use a dynamic
             * allocation of the buffer to satisfy everybody. Of course,
             * as we don't know in advance the size of the array, we try
             * various increasing sizes. */

            /* Check if the driver gave us any hints. */
            if (iwr.u.data.length > buflen)
                buflen = iwr.u.data.length;
            else
                buflen *= 2;

            /* Try again */
            goto realloc;
        }
        printf("Get Scan Results Failed\n");
        free(buffer);
        return -1;
    }

    parse_scan_info(pHandle, buffer, iwr.u.data.length);
    free(buffer);
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Process read eeprom
 *
 *  @param stroffset	A pointer to the offset string
 *  @param strnob	A pointer to NOB string
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_read_eeprom(s8 * stroffset, s8 * strnob)
{
    s8 buffer[MAX_EEPROM_DATA];
    struct ifreq userdata;
    wlan_ioctl_regrdwr *reg = (wlan_ioctl_regrdwr *) buffer;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("regrdwr",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    memset(buffer, 0, sizeof(buffer));
    reg->WhichReg = REG_EEPROM;
    reg->Action = 0;

    if (!strncasecmp(stroffset, "0x", 2))
        reg->Offset = a2hex((stroffset + 2));
    else
        reg->Offset = atoi(stroffset);

    if (!strncasecmp(strnob, "0x", 2))
        reg->NOB = a2hex((strnob + 2));
    else
        reg->NOB = atoi(strnob);

    if ((reg->NOB + sizeof(wlan_ioctl_regrdwr)) > MAX_EEPROM_DATA) {
        fprintf(stderr, "Number of bytes exceeds MAX EEPROM Read size\n");
        return WLAN_STATUS_FAILURE;
    }

    strncpy(userdata.ifr_name, dev_name, IFNAMSIZ);
    userdata.ifr_data = buffer;

    if (ioctl(sockfd, ioctl_val, &userdata)) {
        perror("wlanconfig");
        fprintf(stderr,
                "wlanconfig: EEPROM read not possible "
                "by interface %s\n", dev_name);
        return WLAN_STATUS_FAILURE;
    }

    hexdump("RD EEPROM", &reg->Value, MIN(reg->NOB, MAX_EEPROM_DATA), ' ');

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief Display usage
 *  
 *  @return       NA
 */
static void
display_usage(void)
{
    s32 i;

    for (i = 0; i < NELEMENTS(usage); i++)
        fprintf(stderr, "%s\n", usage[i]);
}

/* 
 *  @brief Find command
 *  
 *  @param maxcmds	max command number
 *  @param cmds		A pointer to commands buffer
 *  @param cmd		A pointer to command buffer
 *  @return      	index of command or WLAN_STATUS_FAILURE
 */
static int
findcommand(s32 maxcmds, s8 * cmds[], s8 * cmd)
{
    s32 i;

    for (i = 0; i < maxcmds; i++) {
        if (!strcasecmp(cmds[i], cmd)) {
            return i;
        }
    }

    return WLAN_STATUS_FAILURE;
}

/* 
 *  @brief SD comand52 read
 *  @param argc		number of arguments
 *  @param argv         A pointer to arguments array    
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_sdcmd52r(int argc, char *argv[])
{
    struct ifreq userdata;
    u8 buf[6];
    u32 tmp;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("sdcmd52rw",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    buf[0] = 0;                 //CMD52 read
    if (argc == 5) {
        buf[1] = atoval(argv[3]);       //func
        tmp = atoval(argv[4]);  //reg
        buf[2] = tmp & 0xff;
        buf[3] = (tmp >> 8) & 0xff;
        buf[4] = (tmp >> 16) & 0xff;
        buf[5] = (tmp >> 24) & 0xff;
    } else {
        fprintf(stderr, "Invalid number of parameters!\n");
        return WLAN_STATUS_FAILURE;
    }

    strncpy(userdata.ifr_name, dev_name, IFNAMSIZ);
    userdata.ifr_data = buf;

    if (ioctl(sockfd, ioctl_val, &userdata)) {
        perror("wlanconfig");
        fprintf(stderr,
                "wlanconfig: CMD52 R/W not supported by "
                "interface %s\n", dev_name);
        return WLAN_STATUS_FAILURE;
    }
    printf("sdcmd52r returns 0x%02X\n", buf[0]);

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief SD comand52 write
 *  @param argc		number of arguments
 *  @param argv         A pointer to arguments array    
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_sdcmd52w(int argc, char *argv[])
{
    struct ifreq userdata;
    u8 buf[7];
    u32 tmp;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("sdcmd52rw",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    buf[0] = 1;                 //CMD52 write
    if (argc == 6) {
        buf[1] = atoval(argv[3]);       //func
        tmp = atoval(argv[4]);  //reg
        buf[2] = tmp & 0xff;
        buf[3] = (tmp >> 8) & 0xff;
        buf[4] = (tmp >> 16) & 0xff;
        buf[5] = (tmp >> 24) & 0xff;
        buf[6] = atoval(argv[5]);       //dat
    } else {
        fprintf(stderr, "Invalid number of parameters!\n");
        return WLAN_STATUS_FAILURE;
    }

    strncpy(userdata.ifr_name, dev_name, IFNAMSIZ);
    userdata.ifr_data = buf;

    if (ioctl(sockfd, ioctl_val, &userdata)) {
        perror("wlanconfig");
        fprintf(stderr,
                "wlanconfig: CMD52 R/W not supported by "
                "interface %s\n", dev_name);
        return WLAN_STATUS_FAILURE;
    }
    printf("sdcmd52w returns 0x%02X\n", buf[0]);

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief SD comand53 read
 *  
 *  @param argc		number of arguments
 *  @param argv         A pointer to arguments array    
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_sdcmd53r(void)
{
    struct ifreq userdata;
    s8 buf[CMD53BUFLEN];
    int i;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("sdcmd53rw",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }
    strncpy(userdata.ifr_name, dev_name, IFNAMSIZ);
    userdata.ifr_data = buf;

    for (i = 0; i < NELEMENTS(buf); i++)
        buf[i] = i & 0xff;

    if (ioctl(sockfd, ioctl_val, &userdata)) {
        perror("wlanconfig");
        fprintf(stderr,
                "wlanconfig: CMD53 R/W not supported by "
                "interface %s\n", dev_name);
        return WLAN_STATUS_FAILURE;
    }

    for (i = 0; i < NELEMENTS(buf); i++) {
        if (buf[i] != (i ^ 0xff))
            printf("i=%02X  %02X\n", i, buf[i]);
    }

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief Get the current status of the WMM Queues
 *  
 *  Command: wlanconfig eth1 qstatus
 *
 *  Retrieve the following information for each AC if wmm is enabled:
 *        - WMM IE ACM Required
 *        - Firmware Flow Required 
 *        - Firmware Flow Established
 *        - Firmware Queue Enabled
 *  
 *  @param argc     number of arguments
 *  @param argv     A pointer to arguments array    
 *
 *  @return         WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_wmm_qstatus(int argc, char *argv[])
{
    int ioctl_val, subioctl_val;
    struct iwreq iwr;
    wlan_ioctl_wmm_queue_status_t qstatus;
    wlan_wmm_ac_e acVal;

    if (get_priv_ioctl("qstatus",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    memset(&qstatus, 0x00, sizeof(qstatus));

    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.flags = subioctl_val;
    iwr.u.data.pointer = (caddr_t) & qstatus;
    iwr.u.data.length = (sizeof(qstatus));

    if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
        perror("wlanconfig: qstatus ioctl");
        return -EFAULT;
    }

    for (acVal = AC_PRIO_BK; acVal <= AC_PRIO_VO; acVal++) {
        switch (acVal) {
        case AC_PRIO_BK:
            printf("BK: ");
            break;
        case AC_PRIO_BE:
            printf("BE: ");
            break;
        case AC_PRIO_VI:
            printf("VI: ");
            break;
        case AC_PRIO_VO:
            printf("VO: ");
            break;
        default:
            printf("??: ");
        }

        printf("ACM[%c], FlowReq[%c], FlowCreated[%c], Enabled[%c]\n",
               (qstatus.acStatus[acVal].wmmACM ? 'X' : ' '),
               (qstatus.acStatus[acVal].flowRequired ? 'X' : ' '),
               (qstatus.acStatus[acVal].flowCreated ? 'X' : ' '),
               (qstatus.acStatus[acVal].disabled ? ' ' : 'X'));
    }

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief Get/Set WMM ack policy
 *  
 *  @param argc		number of arguments
 *  @param argv         A pointer to arguments array    
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_wmm_ack_policy(int argc, char *argv[])
{
    u8 buf[(WMM_ACK_POLICY_PRIO * 2) + 3];
    s32 count, i;
    struct ifreq userdata;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("setgetconf",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    if ((argc != 3) && (argc != 5)) {
        printf("Error: invalid no of arguments\n");
        printf("Syntax: ./wlanconfig eth1 wmm_ack_policy\n");
        printf("Syntax: ./wlanconfig eth1 wmm_ack_policy <AC> <POLICY>\n");
        exit(1);
    }

    memset(buf, 0, (WMM_ACK_POLICY_PRIO * 2) + 3);

    buf[0] = WMM_ACK_POLICY;

    if (argc == 5) {
        buf[1] = HostCmd_ACT_SET;
        buf[2] = 0;

        buf[3] = atoi(argv[3]);
        if (buf[3] > WMM_ACK_POLICY_PRIO - 1) {
            printf("Invalid Priority. Should be between 0 and %d\n",
                   WMM_ACK_POLICY_PRIO - 1);
            exit(1);
        }

        buf[4] = atoi(argv[4]);
        if (buf[4] > 1) {
            printf("Invalid Ack Policy. Should be 1 or 0\n");
            exit(1);
        }

        count = 5;
    } else {
        count = 2;
        buf[1] = HostCmd_ACT_GET;
    }

    strncpy(userdata.ifr_name, dev_name, IFNAMSIZ);
    userdata.ifr_data = buf;

    if (ioctl(sockfd, ioctl_val, &userdata)) {
        fprintf(stderr, "wlanconfig: %s not supported by %s\n",
                argv[2], dev_name);
        return WLAN_STATUS_FAILURE;
    }

    if (buf[1] == HostCmd_ACT_GET) {
        printf("AC Value    Priority\n");
        for (i = 0; i < WMM_ACK_POLICY_PRIO; ++i) {
            count = SKIP_TYPE_ACTION + (i * 2);
            printf("%4x       %5x\n", buf[count], buf[count + 1]);
        }
    }

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief read current command
 *  @param ptr		A pointer to data
 *  @param curCmd       A pointer to the buf which will hold current command    
 *  @return      	NULL or the pointer to the left command buf
 */
static s8 *
readCurCmd(s8 * ptr, s8 * curCmd)
{
    s32 i = 0;
#define MAX_CMD_SIZE 64

    while (*ptr != ']' && i < (MAX_CMD_SIZE - 1))
        curCmd[i++] = *(++ptr);

    if (*ptr != ']')
        return NULL;

    curCmd[i - 1] = '\0';

    return ++ptr;
}

/** 
 *  @brief parse command and hex data
 *  @param fp 		A pointer to FILE stream
 *  @param dst       	A pointer to the dest buf
 *  @param cmd		A pointer to command buf for search
 *  @return            	length of hex data or WLAN_STATUS_FAILURE  		
 */
static int
fparse_for_cmd_and_hex(FILE * fp, u8 * dst, u8 * cmd)
{
    s8 *ptr;
    u8 *dptr;
    s8 buf[256], curCmd[64];
    s32 isCurCmd = 0;

    dptr = dst;
    while (fgets(buf, sizeof(buf), fp)) {
        ptr = buf;

        while (*ptr) {
            // skip leading spaces
            while (*ptr && isspace(*ptr))
                ptr++;

            // skip blank lines and lines beginning with '#'
            if (*ptr == '\0' || *ptr == '#')
                break;

            if (*ptr == '[' && *(ptr + 1) != '/') {
                if (!(ptr = readCurCmd(ptr, curCmd)))
                    return WLAN_STATUS_FAILURE;

                if (strcasecmp(curCmd, cmd))    /* Not equal */
                    isCurCmd = 0;
                else
                    isCurCmd = 1;
            }

            /* Ignore the rest if it is not correct cmd */
            if (!isCurCmd)
                break;

            if (*ptr == '[' && *(ptr + 1) == '/')
                return (dptr - dst);

            if (isxdigit(*ptr)) {
                ptr = convert2hex(ptr, dptr++);
            } else {
                /* Invalid character on data line */
                ptr++;
            }

        }
    }

    return WLAN_STATUS_FAILURE;
}

/* 
 *  @brief Config WMM parameters
 *  
 *  @param argc		number of arguments
 *  @param argv         A pointer to arguments array    
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_wmm_para_conf(int argc, char *argv[], s32 cmd)
{
    s32 count;
    FILE *fp;
    s8 buf[256];
    s8 filename[48] = "";
    struct ifreq userdata;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("setgetconf",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    if (argc != 4) {
        printf("Error: invalid no of arguments\n");
        printf("Syntax: ./wlanconfig eth1 %s <filename>\n", argv[2]);
        exit(1);
    }

    strncpy(filename, argv[3], MIN(sizeof(filename) - 1, strlen(argv[3])));
    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Cannot open file %s\n", argv[3]);
        exit(1);
    }

    count = fparse_for_cmd_and_hex(fp, buf + SKIP_TYPE, argv[2]);
    if (count < 0) {
        printf("Invalid command parsing failed !!!\n");
        return -EINVAL;
    }

    /* This will set the type of command sent */
    buf[0] = (cmd - CMD_WMM_ACK_POLICY) + WMM_ACK_POLICY;

    hexdump(argv[2], buf, count + SKIP_TYPE, ' ');
    strncpy(userdata.ifr_name, dev_name, IFNAMSIZ);
    userdata.ifr_data = buf;

    if (ioctl(sockfd, ioctl_val, &userdata)) {
        fprintf(stderr, "wlanconfig: %s not supported by %s\n",
                argv[2], dev_name);
        return WLAN_STATUS_FAILURE;
    }

    hexdump(argv[2], buf, count + SKIP_TYPE, ' ');

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Send an ADDTS command to the associated AP
 *
 *  Process a given conf file for a specific TSPEC data block.  Send the
 *    TSPEC along with any other IEs to the driver/firmware for transmission
 *    in an ADDTS request to the associated AP.  
 *
 *  Return the execution status of the command as well as the ADDTS response
 *    from the AP if any.
 *  
 *  wlanconfig ethX addts <filename.conf> <section# of tspec> <timeout in ms>
 *
 *  @param argc     number of arguments
 *  @param argv     A pointer to arguments array    
 *
 *  @return         WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_addts(int argc, char *argv[])
{
    int ioctl_val, subioctl_val;
    struct iwreq iwr;
    unsigned int ieBytes;
    wlan_ioctl_wmm_addts_req_t addtsReq;

    FILE *fp;
    char filename[48];
    char config_id[20];

    memset(&addtsReq, 0x00, sizeof(addtsReq));
    memset(filename, 0x00, sizeof(filename));

    if (get_priv_ioctl("addts",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    if (argc != 6) {
        fprintf(stderr, "Invalid number of parameters!\n");
        return -EINVAL;
    }

    ieBytes = 0;

    strncpy(filename, argv[3], MIN(sizeof(filename) - 1, strlen(argv[3])));
    if ((fp = fopen(filename, "r")) == NULL) {
        perror("fopen");
        fprintf(stderr, "Cannot open file %s\n", argv[3]);
        exit(1);
    }

    sprintf(config_id, "tspec%d", atoi(argv[4]));

    ieBytes = fparse_for_cmd_and_hex(fp, addtsReq.tspecData, config_id);

    if (ieBytes > 0) {
        printf("Found %d bytes in the %s section of conf file %s\n",
               ieBytes, config_id, filename);
    } else {
        fprintf(stderr, "section %s not found in %s\n", config_id, filename);
        exit(1);
    }

    addtsReq.timeout_ms = atoi(argv[5]);

    printf("Cmd Input:\n");
    hexdump(config_id, addtsReq.tspecData, ieBytes, ' ');

    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.flags = subioctl_val;
    iwr.u.data.pointer = (caddr_t) & addtsReq;
    iwr.u.data.length = (sizeof(addtsReq.timeout_ms)
                         + sizeof(addtsReq.commandResult)
                         + sizeof(addtsReq.ieeeStatusCode)
                         + ieBytes);

    if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
        perror("wlanconfig: addts ioctl");
        return -EFAULT;
    }

    ieBytes = iwr.u.data.length - (sizeof(addtsReq.timeout_ms)
                                   + sizeof(addtsReq.commandResult)
                                   + sizeof(addtsReq.ieeeStatusCode));
    printf("Cmd Output:\n");
    printf("ADDTS Command Result = %d\n", addtsReq.commandResult);
    printf("ADDTS IEEE Status    = %d\n", addtsReq.ieeeStatusCode);
    hexdump(config_id, addtsReq.tspecData, ieBytes, ' ');

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Send a DELTS command to the associated AP
 *
 *  Process a given conf file for a specific TSPEC data block.  Send the
 *    TSPEC along with any other IEs to the driver/firmware for transmission
 *    in a DELTS request to the associated AP.  
 *
 *  Return the execution status of the command.  There is no response to a
 *    DELTS from the AP.
 *  
 *  wlanconfig ethX delts <filename.conf> <section# of tspec>
 *
 *  @param argc     number of arguments
 *  @param argv     A pointer to arguments array    
 *
 *  @return         WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_delts(int argc, char *argv[])
{
    int ioctl_val, subioctl_val;
    struct iwreq iwr;
    unsigned int ieBytes;
    wlan_ioctl_wmm_delts_req_t deltsReq;

    FILE *fp;
    char filename[48];
    char config_id[20];

    memset(&deltsReq, 0x00, sizeof(deltsReq));
    memset(filename, 0x00, sizeof(filename));

    if (get_priv_ioctl("delts",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    if (argc != 5) {
        fprintf(stderr, "Invalid number of parameters!\n");
        return -EINVAL;
    }

    ieBytes = 0;

    strncpy(filename, argv[3], MIN(sizeof(filename) - 1, strlen(argv[3])));
    if ((fp = fopen(filename, "r")) == NULL) {
        perror("fopen");
        fprintf(stderr, "Cannot open file %s\n", argv[3]);
        exit(1);
    }

    sprintf(config_id, "tspec%d", atoi(argv[4]));

    ieBytes = fparse_for_cmd_and_hex(fp, deltsReq.tspecData, config_id);

    if (ieBytes > 0) {
        printf("Found %d bytes in the %s section of conf file %s\n",
               ieBytes, config_id, filename);
    } else {
        fprintf(stderr, "section %s not found in %s\n", config_id, filename);
        exit(1);
    }

    deltsReq.ieeeReasonCode = 0x20;     /* 32, unspecified QOS reason */

    printf("Cmd Input:\n");
    hexdump(config_id, deltsReq.tspecData, ieBytes, ' ');

    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.flags = subioctl_val;
    iwr.u.data.pointer = (caddr_t) & deltsReq;
    iwr.u.data.length = (sizeof(deltsReq.commandResult)
                         + sizeof(deltsReq.ieeeReasonCode)
                         + ieBytes);

    if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
        perror("wlanconfig: delts ioctl");
        return -EFAULT;
    }

    printf("Cmd Output:\n");
    printf("DELTS Command Result = %d\n", deltsReq.commandResult);

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Send a WMM AC Queue configuration command to get/set/default params
 *
 *  Configure or get the parameters of a WMM AC queue. The command takes
 *    an optional Queue Id as a last parameter.  Without the queue id, all
 *    queues will be acted upon.
 *  
 *  wlanconfig ethX qconfig set msdu <lifetime in TUs> [Queue Id: 0-3]
 *  wlanconfig ethX qconfig get [Queue Id: 0-3]
 *  wlanconfig ethX qconfig def [Queue Id: 0-3]
 *
 *  @param argc     number of arguments
 *  @param argv     A pointer to arguments array    
 *
 *  @return         WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_qconfig(int argc, char *argv[])
{
    int ioctl_val, subioctl_val;
    struct iwreq iwr;
    wlan_ioctl_wmm_queue_config_t queue_config_cmd;
    wlan_wmm_ac_e ac_idx;
    wlan_wmm_ac_e ac_idx_start;
    wlan_wmm_ac_e ac_idx_stop;

    const char *ac_str_tbl[] = { "BK", "BE", "VI", "VO" };

    if (argc < 4) {
        fprintf(stderr, "Invalid number of parameters!\n");
        return -EINVAL;
    }

    if (get_priv_ioctl("qconfig",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    memset(&queue_config_cmd, 0x00, sizeof(queue_config_cmd));
    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = (caddr_t) & queue_config_cmd;
    iwr.u.data.length = sizeof(queue_config_cmd);
    iwr.u.data.flags = subioctl_val;

    if (strcmp(argv[3], "get") == 0) {
        /*    3     4    5   */
        /* qconfig get [qid] */
        if (argc == 4) {
            ac_idx_start = AC_PRIO_BK;
            ac_idx_stop = AC_PRIO_VO;
        } else if (argc == 5) {
            ac_idx_start = atoi(argv[4]);
            ac_idx_stop = ac_idx_start;
        } else {
            fprintf(stderr, "Invalid number of parameters!\n");
            return -EINVAL;
        }
        queue_config_cmd.action = WMM_QUEUE_CONFIG_ACTION_GET;

        for (ac_idx = ac_idx_start; ac_idx <= ac_idx_stop; ac_idx++) {
            queue_config_cmd.accessCategory = ac_idx;
            if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
                perror("qconfig ioctl");
            } else {
                printf("qconfig %s(%d): MSDU Lifetime GET = 0x%04x (%d)\n",
                       ac_str_tbl[ac_idx],
                       ac_idx,
                       queue_config_cmd.msduLifetimeExpiry,
                       queue_config_cmd.msduLifetimeExpiry);
            }
        }
    } else if (strcmp(argv[3], "set") == 0) {
        if (strcmp(argv[4], "msdu") == 0) {
            /*    3     4    5     6      7   */
            /* qconfig set msdu <value> [qid] */
            if (argc == 6) {
                ac_idx_start = AC_PRIO_BK;
                ac_idx_stop = AC_PRIO_VO;
            } else if (argc == 7) {
                ac_idx_start = atoi(argv[6]);
                ac_idx_stop = ac_idx_start;
            } else {
                fprintf(stderr, "Invalid number of parameters!\n");
                return -EINVAL;
            }
            queue_config_cmd.action = WMM_QUEUE_CONFIG_ACTION_SET;
            queue_config_cmd.msduLifetimeExpiry = atoi(argv[5]);

            for (ac_idx = ac_idx_start; ac_idx <= ac_idx_stop; ac_idx++) {
                queue_config_cmd.accessCategory = ac_idx;
                if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
                    perror("qconfig ioctl");
                } else {
                    printf
                        ("qconfig %s(%d): MSDU Lifetime SET = 0x%04x (%d)\n",
                         ac_str_tbl[ac_idx], ac_idx,
                         queue_config_cmd.msduLifetimeExpiry,
                         queue_config_cmd.msduLifetimeExpiry);
                }
            }
        } else {
            /* Only MSDU Lifetime provisioning accepted for now */
            fprintf(stderr, "Invalid set parameter: s/b [msdu]\n");
            return -EINVAL;
        }
    } else if (strncmp(argv[3], "def", strlen("def")) == 0) {
        /*    3     4    5   */
        /* qconfig def [qid] */
        if (argc == 4) {
            ac_idx_start = AC_PRIO_BK;
            ac_idx_stop = AC_PRIO_VO;
        } else if (argc == 5) {
            ac_idx_start = atoi(argv[4]);
            ac_idx_stop = ac_idx_start;
        } else {
            fprintf(stderr, "Invalid number of parameters!\n");
            return -EINVAL;
        }
        queue_config_cmd.action = WMM_QUEUE_CONFIG_ACTION_DEFAULT;

        for (ac_idx = ac_idx_start; ac_idx <= ac_idx_stop; ac_idx++) {
            queue_config_cmd.accessCategory = ac_idx;
            if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
                perror("qconfig ioctl");
            } else {
                printf
                    ("qconfig %s(%d): MSDU Lifetime DEFAULT = 0x%04x (%d)\n",
                     ac_str_tbl[ac_idx], ac_idx,
                     queue_config_cmd.msduLifetimeExpiry,
                     queue_config_cmd.msduLifetimeExpiry);
            }
        }
    } else {
        fprintf(stderr, "Invalid qconfig command; s/b [set, get, default]\n");
        return -EINVAL;
    }

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Turn on/off or retrieve and clear the queue statistics for an AC
 *
 *  Turn the queue statistics collection on/off for a given AC or retrieve the
 *    current accumulated stats and clear them from the firmware.  The command
 *    takes an optional Queue Id as a last parameter.  Without the queue id,
 *    all queues will be acted upon.
 *
 *  wlanconfig ethX qstats on [Queue Id: 0-3]
 *  wlanconfig ethX qstats off [Queue Id: 0-3]
 *  wlanconfig ethX qstats get [Queue Id: 0-3]
 *
 *  @param argc     number of arguments
 *  @param argv     A pointer to arguments array    
 *
 *  @return         WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_qstats(int argc, char *argv[])
{
    int ioctl_val, subioctl_val;
    struct iwreq iwr;
    wlan_ioctl_wmm_queue_stats_t queue_stats_cmd;
    wlan_wmm_ac_e ac_idx;
    wlan_wmm_ac_e ac_idx_start;
    wlan_wmm_ac_e ac_idx_stop;

    const char *ac_str_tbl[] = { "BK", "BE", "VI", "VO" };

    if (argc < 3) {
        fprintf(stderr, "Invalid number of parameters!\n");
        return -EINVAL;
    }

    if (get_priv_ioctl("qstats",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    printf("\n");

    memset(&queue_stats_cmd, 0x00, sizeof(queue_stats_cmd));
    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = (caddr_t) & queue_stats_cmd;
    iwr.u.data.length = sizeof(queue_stats_cmd);
    iwr.u.data.flags = subioctl_val;

    if ((argc > 3) && strcmp(argv[3], "on") == 0) {
        if (argc == 4) {
            ac_idx_start = AC_PRIO_BK;
            ac_idx_stop = AC_PRIO_VO;
        } else if (argc == 5) {
            ac_idx_start = atoi(argv[4]);
            ac_idx_stop = ac_idx_start;
        } else {
            fprintf(stderr, "Invalid number of parameters!\n");
            return -EINVAL;
        }

        queue_stats_cmd.action = WMM_STATS_ACTION_START;
        for (ac_idx = ac_idx_start; ac_idx <= ac_idx_stop; ac_idx++) {
            queue_stats_cmd.accessCategory = ac_idx;
            if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
                perror("qstats ioctl");
            } else {
                printf("qstats %s(%d) turned on\n",
                       ac_str_tbl[ac_idx], ac_idx);
            }
        }
    } else if ((argc > 3) && strcmp(argv[3], "off") == 0) {
        if (argc == 4) {
            ac_idx_start = AC_PRIO_BK;
            ac_idx_stop = AC_PRIO_VO;
        } else if (argc == 5) {
            ac_idx_start = atoi(argv[4]);
            ac_idx_stop = ac_idx_start;
        } else {
            fprintf(stderr, "Invalid number of parameters!\n");
            return -EINVAL;
        }

        queue_stats_cmd.action = WMM_STATS_ACTION_STOP;
        for (ac_idx = ac_idx_start; ac_idx <= ac_idx_stop; ac_idx++) {
            queue_stats_cmd.accessCategory = ac_idx;
            if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
                perror("qstats ioctl");
            } else {
                printf("qstats %s(%d) turned off\n",
                       ac_str_tbl[ac_idx], ac_idx);
            }
        }
    }
    /* If the user types: "wlanconfig eth1 qstats" without get argument.
       The wlanconfig application invokes "get" option for all the queues */
    else if (((argc > 3) || (argc == 3)) &&
             ((argc == 3) ? 1 : (strcmp(argv[3], "get") == 0))) {
        printf("AC  Count   Loss  TxDly   QDly"
               "    <=5   <=10   <=20   <=30   <=40   <=50    >50\n");
        printf("----------------------------------"
               "---------------------------------------------\n");
        if ((argc == 4) || (argc == 3)) {
            ac_idx_start = AC_PRIO_BK;
            ac_idx_stop = AC_PRIO_VO;
        } else if (argc == 5) {
            ac_idx_start = atoi(argv[4]);
            ac_idx_stop = ac_idx_start;
        } else {
            fprintf(stderr, "Invalid number of parameters!\n");
            return -EINVAL;
        }
        queue_stats_cmd.action = WMM_STATS_ACTION_GET_CLR;

        for (ac_idx = ac_idx_start; ac_idx <= ac_idx_stop; ac_idx++) {
            queue_stats_cmd.accessCategory = ac_idx;
            if (ioctl(sockfd, ioctl_val, &iwr) < 0) {
                perror("qstats ioctl");
            } else {
                printf("%s  %5u  %5u %6u %6u"
                       "  %5u  %5u  %5u  %5u  %5u  %5u  %5u\n",
                       ac_str_tbl[ac_idx],
                       queue_stats_cmd.pktCount,
                       queue_stats_cmd.pktLoss,
                       (unsigned int) queue_stats_cmd.avgTxDelay,
                       (unsigned int) queue_stats_cmd.avgQueueDelay,
                       queue_stats_cmd.delayHistogram[0],
                       queue_stats_cmd.delayHistogram[1],
                       queue_stats_cmd.delayHistogram[2],
                       queue_stats_cmd.delayHistogram[3],
                       queue_stats_cmd.delayHistogram[4],
                       queue_stats_cmd.delayHistogram[5],
                       queue_stats_cmd.delayHistogram[6]);
            }
        }
    } else {
        fprintf(stderr, "Invalid qstats command;\n");
        return -EINVAL;
    }
    printf("\n");

    return 0;
}

/* 
 *  @brief Get one line from the File
 *  
 *  @param s	        Storage location for data.
 *  @param size 	Maximum number of characters to read. 
 *  @param stream 	File stream	  	
 *  @param line		A pointer to return current line number
 *  @return             returns string or NULL 
 */
static s8 *
wlan_config_get_line(s8 * s, s32 size, FILE * stream, int *line)
{
    s8 *pos, *end, *sstart;

    while (fgets(s, size, stream)) {
        (*line)++;
        s[size - 1] = '\0';
        pos = s;

        while (*pos == ' ' || *pos == '\t')
            pos++;
        if (*pos == '#' || (*pos == '\r' && *(pos + 1) == '\n') ||
            *pos == '\n' || *pos == '\0')
            continue;

        /* Remove # comments unless they are within a double quoted
         * string. Remove trailing white space. */
        sstart = strchr(pos, '"');
        if (sstart)
            sstart = strchr(sstart + 1, '"');
        if (!sstart)
            sstart = pos;
        end = strchr(sstart, '#');
        if (end)
            *end-- = '\0';
        else
            end = pos + strlen(pos) - 1;
        while (end > pos && (*end == '\r' || *end == '\n' ||
                             *end == ' ' || *end == '\t')) {
            *end-- = '\0';
        }
        if (*pos == '\0')
            continue;
        return pos;
    }

    return NULL;
}

/* 
 *  @brief convert hex char to integer
 *  
 *  @param c		char
 *  @return      	integer or WLAN_STATUS_FAILURE
 */
static int
hex2num(s8 c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return WLAN_STATUS_FAILURE;
}

/* 
 *  @brief convert hex char to integer
 *  
 *  @param c		char
 *  @return      	integer or WLAN_STATUS_FAILURE
 */
static int
hex2byte(const s8 * hex)
{
    s32 a, b;
    a = hex2num(*hex++);
    if (a < 0)
        return -1;
    b = hex2num(*hex++);
    if (b < 0)
        return -1;
    return (a << 4) | b;
}

/* 
 *  @brief convert hex char to integer
 *  
 *  @param hex		A pointer to hex string
 *  @param buf		buffer to storege the data
 *  @param len		
 *  @return      	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
hexstr2bin(const s8 * hex, u8 * buf, size_t len)
{
    s32 i, a;
    const s8 *ipos = hex;
    s8 *opos = buf;

    for (i = 0; i < len; i++) {
        a = hex2byte(ipos);
        if (a < 0)
            return WLAN_STATUS_FAILURE;
        *opos++ = a;
        ipos += 2;
    }
    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse string
 *  
 *  @param value	A pointer to string
 *  @param buf		buffer to storege the data
 *  @param str		buffer to hold return string
 *  @param len 		use to return the length of parsed string
 *  @param maxlen	use to return the max length of ssid
 *  @return      	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_config_parse_string(const s8 * value, s8 * str, size_t * len,
                         size_t * maxlen)
{
    s8 *p;
    p = strchr(value, ',');
    if (p) {
        *maxlen = (u16) a2hex_or_atoi(p + 1);
        *p = '\0';
    } else {
#define WILDCARD_CHAR		'*'
        p = strchr(value, WILDCARD_CHAR);
        if (p)
            *maxlen = (size_t) WILDCARD_CHAR;
        else
            *maxlen = 0;
    }

    if (*value == '"') {
        s8 *pos;
        value++;
        pos = strchr(value, '"');

        if (pos == NULL || pos[1] != '\0') {
            value--;
            return WLAN_STATUS_FAILURE;
        }
        *pos = '\0';
        *len = strlen(value);
        strcpy(str, value);
        return WLAN_STATUS_SUCCESS;
    } else {
        s32 hlen = strlen(value);

        if (hlen % 1)
            return WLAN_STATUS_FAILURE;
        *len = hlen / 2;
        if (str == NULL)
            return WLAN_STATUS_FAILURE;
        if (hexstr2bin(value, str, *len)) {
            return WLAN_STATUS_FAILURE;
        }

        return WLAN_STATUS_SUCCESS;
    }
}

/* 
 *  @brief parse bgscan action
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to Action buffer
 *  @return      	WLAN_STATUS_SUCCESS
 */
static int
bgscan_parse_action(u8 * CmdBuf, s32 line, s8 * value)
{
    HostCmd_DS_802_11_BG_SCAN_CONFIG *bgscan_config =
        (HostCmd_DS_802_11_BG_SCAN_CONFIG *) CmdBuf;

    bgscan_config->Action = (u16) a2hex_or_atoi(value);
    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse bgscan enable parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to Enable buffer
 *  @return      	WLAN_STATUS_SUCCESS
 */
static int
bgscan_parse_enable(u8 * CmdBuf, s32 line, s8 * value)
{
    HostCmd_DS_802_11_BG_SCAN_CONFIG *bgscan_config =
        (HostCmd_DS_802_11_BG_SCAN_CONFIG *) CmdBuf;

    bgscan_config->Enable = (u8) a2hex_or_atoi(value);

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse bgscan BssType
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to BssType buffer
 *  @return      	WLAN_STATUS_SUCCESS
 */
static int
bgscan_parse_bsstype(u8 * CmdBuf, s32 line, s8 * value)
{
    HostCmd_DS_802_11_BG_SCAN_CONFIG *bgscan_config =
        (HostCmd_DS_802_11_BG_SCAN_CONFIG *) CmdBuf;

    bgscan_config->BssType = (u8) a2hex_or_atoi(value);

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse bgscan channels per scan parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to channels per scan buffer
 *  @return      	WLAN_STATUS_SUCCESS
 */
static int
bgscan_parse_channelsperscan(u8 * CmdBuf, s32 line, s8 * value)
{
    HostCmd_DS_802_11_BG_SCAN_CONFIG *bgscan_config =
        (HostCmd_DS_802_11_BG_SCAN_CONFIG *) CmdBuf;

    bgscan_config->ChannelsPerScan = (u8) a2hex_or_atoi(value);

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse bgscan discardwhenfull parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to channels per scan buffer
 *  @return      	WLAN_STATUS_SUCCESS
 */
static int
bgscan_parse_discardwhenfull(u8 * CmdBuf, s32 line, s8 * value)
{
    HostCmd_DS_802_11_BG_SCAN_CONFIG *bgscan_config =
        (HostCmd_DS_802_11_BG_SCAN_CONFIG *) CmdBuf;

    bgscan_config->DiscardWhenFull = (u8) a2hex_or_atoi(value);

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse bgscan scan interval parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to scan interval buffer
 *  @return      	WLAN_STATUS_SUCCESS
 */
static int
bgscan_parse_scaninterval(u8 * CmdBuf, s32 line, s8 * value)
{
    HostCmd_DS_802_11_BG_SCAN_CONFIG *bgscan_config =
        (HostCmd_DS_802_11_BG_SCAN_CONFIG *) CmdBuf;

    bgscan_config->ScanInterval = (u32) a2hex_or_atoi(value);

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse bgscan store condition parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to store condition buffer
 *  @return      	WLAN_STATUS_SUCCESS
 */
static int
bgscan_parse_storecondition(u8 * CmdBuf, s32 line, s8 * value)
{
    HostCmd_DS_802_11_BG_SCAN_CONFIG *bgscan_config =
        (HostCmd_DS_802_11_BG_SCAN_CONFIG *) CmdBuf;

    bgscan_config->StoreCondition = a2hex_or_atoi(value);

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse bgscan report conditions parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to report conditionsn buffer
 *  @return      	WLAN_STATUS_SUCCESS
 */
static int
bgscan_parse_reportconditions(u8 * CmdBuf, s32 line, s8 * value)
{
    HostCmd_DS_802_11_BG_SCAN_CONFIG *bgscan_config =
        (HostCmd_DS_802_11_BG_SCAN_CONFIG *) CmdBuf;

    bgscan_config->ReportConditions = a2hex_or_atoi(value);

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse bgscan max scan results parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to max scan results buffer
 *  @return      	WLAN_STATUS_SUCCESS
 */
static int
bgscan_parse_maxscanresults(u8 * CmdBuf, s32 line, s8 * value)
{
    HostCmd_DS_802_11_BG_SCAN_CONFIG *bgscan_config =
        (HostCmd_DS_802_11_BG_SCAN_CONFIG *) CmdBuf;

    bgscan_config->MaxScanResults = (u16) a2hex_or_atoi(value);

    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse bgscan ssid parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to ssid buffer
 *  @return      	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
bgscan_parse_ssid(u8 * CmdBuf, s32 line, s8 * value)
{
    static int ssidCnt;
    MrvlIEtypes_SsIdParamSet_t *SsIdParamSet = NULL;
    MrvlIEtypes_WildCardSsIdParamSet_t *WildcardSsIdParamSet = NULL;
    s8 *buf = NULL;
    size_t len = 0;
    size_t maxlen = 0;

    SsIdParamSet = (MrvlIEtypes_SsIdParamSet_t *) (CmdBuf + ActualPos);
    WildcardSsIdParamSet =
        (MrvlIEtypes_WildCardSsIdParamSet_t *) (CmdBuf + ActualPos);

    buf = (s8 *) malloc(strlen(value));
    if (buf == NULL)
        return WLAN_STATUS_FAILURE;
    memset(buf, 0, strlen(value));

    if (wlan_config_parse_string(value, buf, &len, &maxlen)) {
        printf("Invalid SSID\n");
        free(buf);
        return WLAN_STATUS_FAILURE;
    }

    ssidCnt++;

    if (!strlen(buf)) {
        printf("The %dth SSID is NULL.\n", ssidCnt);
    }

    if (maxlen > len) {
        WildcardSsIdParamSet->Header.Type =
            cpu_to_le16(TLV_TYPE_WILDCARDSSID);
        WildcardSsIdParamSet->Header.Len =
            strlen(buf) + sizeof(WildcardSsIdParamSet->MaxSsidLength);
        WildcardSsIdParamSet->MaxSsidLength = maxlen;
        TLVSsidSize +=
            WildcardSsIdParamSet->Header.Len + sizeof(MrvlIEtypesHeader_t);
        ActualPos +=
            WildcardSsIdParamSet->Header.Len + sizeof(MrvlIEtypesHeader_t);
        WildcardSsIdParamSet->Header.Len =
            cpu_to_le16(WildcardSsIdParamSet->Header.Len);
        memcpy(WildcardSsIdParamSet->SsId, buf, strlen(buf));
    } else {
        SsIdParamSet->Header.Type = cpu_to_le16(TLV_TYPE_SSID); /*0x0000; */
        SsIdParamSet->Header.Len = strlen(buf);
        TLVSsidSize += SsIdParamSet->Header.Len + sizeof(MrvlIEtypesHeader_t);

        ActualPos += SsIdParamSet->Header.Len + sizeof(MrvlIEtypesHeader_t);

        SsIdParamSet->Header.Len = cpu_to_le16(SsIdParamSet->Header.Len);

        memcpy(SsIdParamSet->SsId, buf, strlen(buf));
    }
    free(buf);
    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse bgscan probes parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to probes buffer
 *  @return      	WLAN_STATUS_SUCCESS 
 */
static int
bgscan_parse_probes(u8 * CmdBuf, s32 line, s8 * value)
{
    MrvlIEtypes_NumProbes_t *Probes = NULL;

#define PROBES_PAYLOAD_SIZE	2

    Probes = (MrvlIEtypes_NumProbes_t *) (CmdBuf + ActualPos);

    Probes->Header.Type = TLV_TYPE_NUMPROBES;
    Probes->Header.Len = PROBES_PAYLOAD_SIZE;

    Probes->NumProbes = (u16) a2hex_or_atoi(value);

    if (Probes->NumProbes) {
        TLVProbeSize += sizeof(MrvlIEtypesHeader_t) + Probes->Header.Len;
    } else {
        TLVProbeSize = 0;
    }

    ActualPos += TLVProbeSize;
    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse bgscan channel list parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to channel list buffer
 *  @return      	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
bgscan_parse_channellist(u8 * CmdBuf, s32 line, s8 * value)
{
    MrvlIEtypes_ChanListParamSet_t *chan;
    char *buf, *grp0, *grp1;
    s32 len, idx;

    chan = (MrvlIEtypes_ChanListParamSet_t *) (CmdBuf + ActualPos);

    len = strlen(value) + 1;
    buf = malloc(len);

    if (buf == NULL)
        return WLAN_STATUS_FAILURE;

    memset(buf, 0, len);
    strcpy(buf, value);

    chan->Header.Type = cpu_to_le16(TLV_TYPE_CHANLIST);
    grp1 = buf;
    idx = 0;
    while ((grp1 != NULL) && (*grp1 != 0)) {
        grp0 = strsep(&grp1, "\";");

        if ((grp0 != NULL) && (*grp0 != 0)) {

            char *ps8Resultstr = NULL;

            ps8Resultstr = strtok(grp0, ",");
            if (ps8Resultstr == NULL) {
                goto failure;
            }
            chan->ChanScanParam[idx].RadioType = atoi(ps8Resultstr);

            ps8Resultstr = strtok(NULL, ",");
            if (ps8Resultstr == NULL) {
                goto failure;
            }
            chan->ChanScanParam[idx].ChanNumber = atoi(ps8Resultstr);

            ps8Resultstr = strtok(NULL, ",");
            if (ps8Resultstr == NULL) {
                goto failure;
            }
            chan->ChanScanParam[idx].ChanScanMode.PassiveScan
                = atoi(ps8Resultstr);

            chan->ChanScanParam[idx].ChanScanMode.DisableChanFilt = 1;

            ps8Resultstr = strtok(NULL, ",");
            if (ps8Resultstr == NULL) {
                goto failure;
            }
            chan->ChanScanParam[idx].MinScanTime = atoi(ps8Resultstr);

            ps8Resultstr = strtok(NULL, ",");
            if (ps8Resultstr == NULL) {
                goto failure;
            }
            chan->ChanScanParam[idx].MaxScanTime = atoi(ps8Resultstr);
            idx++;
        }
    }

    chan->Header.Len = (idx * sizeof(ChanScanParamSet_t));
    TLVChanSize += (chan->Header.Len + sizeof(MrvlIEtypesHeader_t));
    chan->Header.Len = cpu_to_le16(chan->Header.Len);
    ActualPos += TLVChanSize;

    free(buf);
    return WLAN_STATUS_SUCCESS;
  failure:
    free(buf);
    printf("Invalid string: Check the bg_scan config file\n");
    return WLAN_STATUS_FAILURE;
}

/* 
 *  @brief parse bgscan snr threshold parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to bgscan snr threshold buffer
 *  @return      	WLAN_STATUS_SUCCESS
 */
static int
bgscan_parse_snrthreshold(u8 * CmdBuf, s32 line, s8 * value)
{
    MrvlIEtypes_SnrThreshold_t *SnrThreshold = NULL;
    u32 tmp;

    SnrThreshold = (MrvlIEtypes_SnrThreshold_t *) (CmdBuf + ActualPos);

    SnrThreshold->Header.Type = TLV_TYPE_SNR_LOW;
    SnrThreshold->Header.Len = PROBES_PAYLOAD_SIZE;

    tmp = (u16) a2hex_or_atoi(value);
    SnrThreshold->SNRValue = tmp & 0xff;
    SnrThreshold->SNRFreq = (tmp >> 8) & 0xff;

    TLVSnrSize += sizeof(MrvlIEtypesHeader_t) + SnrThreshold->Header.Len;
    ActualPos += TLVSnrSize;
    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse bgscan broadcast probe parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to broadcast probe buffer
 *  @return      	WLAN_STATUS_SUCCESS
 */
static int
bgscan_parse_bcastprobe(u8 * CmdBuf, s32 line, s8 * value)
{
    MrvlIEtypes_BcastProbe_t *BcastProbe = NULL;

    BcastProbe = (MrvlIEtypes_BcastProbe_t *) (CmdBuf + ActualPos);

    BcastProbe->Header.Type = TLV_TYPE_BCASTPROBE;
    BcastProbe->Header.Len = PROBES_PAYLOAD_SIZE;

    BcastProbe->BcastProbe = (u16) a2hex_or_atoi(value);

    TLVBcProbeSize = sizeof(MrvlIEtypesHeader_t) + BcastProbe->Header.Len;
    ActualPos += TLVBcProbeSize;
    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse number ssid probe parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to number ssid probe buffer
 *  @return      	WLAN_STATUS_SUCCESS
 */
static int
bgscan_parse_numssidprobe(u8 * CmdBuf, s32 line, s8 * value)
{
    MrvlIEtypes_NumSSIDProbe_t *NumSSIDProbe = NULL;

    NumSSIDProbe = (MrvlIEtypes_NumSSIDProbe_t *) (CmdBuf + ActualPos);

    NumSSIDProbe->Header.Type = TLV_TYPE_NUMSSID_PROBE;
    NumSSIDProbe->Header.Len = PROBES_PAYLOAD_SIZE;

    NumSSIDProbe->NumSSIDProbe = (u16) a2hex_or_atoi(value);

    TLVNumSsidProbeSize =
        sizeof(MrvlIEtypesHeader_t) + NumSSIDProbe->Header.Len;
    ActualPos += TLVNumSsidProbeSize;
    return WLAN_STATUS_SUCCESS;
}

/* 
 *  @brief parse bgscan start later parameter
 *  
 *  @param CmdBuf	A pointer to command buffer
 *  @param line	        line number
 *  @param value	A pointer to bgscan start later buffer
 *  @return      	WLAN_STATUS_SUCCESS
 */
static int
bgscan_parse_startlater(u8 * CmdBuf, s32 line, s8 * value)
{
    MrvlIEtypes_StartBGScanLater_t *StartBGScanLater = NULL;

    StartBGScanLater =
        (MrvlIEtypes_StartBGScanLater_t *) (CmdBuf + ActualPos);

    StartBGScanLater->Header.Type = TLV_TYPE_STARTBGSCANLATER;
    StartBGScanLater->Header.Len = PROBES_PAYLOAD_SIZE;

    StartBGScanLater->StartLater = (u16) a2hex_or_atoi(value);

    TLVStartBGScanLaterSize =
        sizeof(MrvlIEtypesHeader_t) + StartBGScanLater->Header.Len;
    ActualPos += TLVStartBGScanLaterSize;
    return WLAN_STATUS_SUCCESS;
}

static struct bgscan_fields
{
    s8 *name;
    int (*parser) (u8 * CmdBuf, s32 line, s8 * value);
} bgscan_fields[] = {
    {
    "Action", bgscan_parse_action}, {
    "Enable", bgscan_parse_enable}, {
    "BssType", bgscan_parse_bsstype}, {
    "ChannelsPerScan", bgscan_parse_channelsperscan}, {
    "DiscardWhenFull", bgscan_parse_discardwhenfull}, {
    "ScanInterval", bgscan_parse_scaninterval}, {
    "StoreCondition", bgscan_parse_storecondition}, {
    "ReportConditions", bgscan_parse_reportconditions}, {
    "MaxScanResults", bgscan_parse_maxscanresults}, {
    "SSID1", bgscan_parse_ssid}, {
    "SSID2", bgscan_parse_ssid}, {
    "SSID3", bgscan_parse_ssid}, {
    "SSID4", bgscan_parse_ssid}, {
    "SSID5", bgscan_parse_ssid}, {
    "SSID6", bgscan_parse_ssid}, {
    "SSID7", bgscan_parse_ssid}, {
    "SSID8", bgscan_parse_ssid}, {
    "SSID9", bgscan_parse_ssid}, {
    "SSID10", bgscan_parse_ssid}, {
    "Probes", bgscan_parse_probes}, {
    "ChannelList", bgscan_parse_channellist}, {
    "SnrThreshold", bgscan_parse_snrthreshold}, {
    "BcastProbe", bgscan_parse_bcastprobe}, {
    "NumSSIDProbe", bgscan_parse_numssidprobe}, {
"StartLater", bgscan_parse_startlater},};

/* 
 *  @brief get bgscan data
 *  
 *  @param fp			A pointer to file stream
 *  @param line	        	A pointer to line number
 *  @param bgscan_config	A pointer to HostCmd_DS_802_11_BG_SCAN_CONFIG structure
 *  @return      		WLAN_STATUS_SUCCESS
 */
static int
wlan_get_bgscan_data(FILE * fp, int *line,
                     HostCmd_DS_802_11_BG_SCAN_CONFIG * bgscan_config)
{
    s32 errors = 0, i, end = 0;
    s8 buf[256], *pos, *pos2;

    while ((pos = wlan_config_get_line(buf, sizeof(buf), fp, line))) {
        if (strcmp(pos, "}") == 0) {
            end = 1;
            break;
        }

        pos2 = strchr(pos, '=');
        if (pos2 == NULL) {
            printf("Line %d: Invalid bgscan line '%s'.", *line, pos);
            errors++;
            continue;
        }

        *pos2++ = '\0';
        if (*pos2 == '"') {
            if (strchr(pos2 + 1, '"') == NULL) {
                printf("Line %d: invalid quotation '%s'.", *line, pos2);
                errors++;
                continue;
            }
        }

        for (i = 0; i < NELEMENTS(bgscan_fields); i++) {
            if (strcmp(pos, bgscan_fields[i].name) == 0) {
                if (bgscan_fields[i].parser((u8 *) bgscan_config,
                                            *line, pos2)) {
                    printf("Line %d: failed to parse %s"
                           "'%s'.", *line, pos, pos2);
                    errors++;
                }
                break;
            }
        }
        if (i == NELEMENTS(bgscan_fields)) {
            printf("Line %d: unknown bgscan field '%s'.\n", *line, pos);
            errors++;
        }
    }
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Process bgscan config 
 *  @param argc		number of arguments
 *  @param argv         A pointer to arguments array    
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_bg_scan_config(int argc, char *argv[])
{
    u8 scanCfg[256], *pos, *buf = NULL;
    s8 filename[48] = "";
    FILE *fp;
    HostCmd_DS_802_11_BG_SCAN_CONFIG *bgscan_config;
    struct ifreq userdata;
    int line = 0;
    s32 CmdNum = BG_SCAN_CONFIG;
    s32 Action;
    u16 Size;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("setgetconf",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    if (argc != 4) {
        printf("Error: invalid no of arguments\n");
        printf("Syntax: ./wlanconfig eth1 bgscanconfig <filename>\n");
        exit(1);
    }

    strncpy(filename, argv[3], MIN(sizeof(filename) - 1, strlen(argv[3])));
    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Cannot open file %s\n", filename);
        exit(1);
    }
#define BGSCANCFG_BUF_LEN	1024
    buf = (u8 *) malloc(BGSCANCFG_BUF_LEN);
    if (buf == NULL) {
        printf("Error: allocate memory for bgscan fail\n");
        fclose(fp);
        return -ENOMEM;
    }
    memset(buf, 0, BGSCANCFG_BUF_LEN);

    bgscan_config = (HostCmd_DS_802_11_BG_SCAN_CONFIG *)
        (buf + sizeof(s32) + sizeof(u16));

    while ((pos =
            (u8 *) wlan_config_get_line((s8 *) scanCfg, sizeof(scanCfg), fp,
                                        &line))) {
        if (strcmp((char *) pos, "bgscan={") == 0) {
            wlan_get_bgscan_data(fp, &line, bgscan_config);
        }
    }

    fclose(fp);

    Action = bgscan_config->Action;

    Size = sizeof(HostCmd_DS_802_11_BG_SCAN_CONFIG) +
        TLVSsidSize + TLVProbeSize + TLVChanSize + TLVSnrSize +
        TLVBcProbeSize + TLVNumSsidProbeSize;
    Size += TLVStartBGScanLaterSize;

    memcpy(buf, &CmdNum, sizeof(s32));
    memcpy(buf + sizeof(s32), &Size, sizeof(u16));

    strncpy(userdata.ifr_name, dev_name, IFNAMSIZ);
    userdata.ifr_data = buf;

    if (ioctl(sockfd, ioctl_val, &userdata)) {
        fprintf(stderr, "wlanconfig: BG_SCAN is not supported by %s\n",
                dev_name);
        return -1;
    }

    if (Action == HostCmd_ACT_GEN_GET) {
        Size = *(u16 *) buf;
        hexdump("BGSCAN Configuration setup", &buf[SKIP_TYPE_SIZE], Size,
                ' ');
    }

    free(buf);

    return WLAN_STATUS_SUCCESS;

}

/** 
 *  @brief parse hex data
 *  @param fp 		A pointer to FILE stream
 *  @param dst		A pointer to receive hex data
 *  @return            	length of hex data
 */
static int
fparse_for_hex(FILE * fp, u8 * dst)
{
    s8 *ptr;
    u8 *dptr;
    s8 buf[256];

    dptr = dst;
    while (fgets(buf, sizeof(buf), fp)) {
        ptr = buf;

        while (*ptr) {
            // skip leading spaces
            while (*ptr && isspace(*ptr))
                ptr++;

            // skip blank lines and lines beginning with '#'
            if (*ptr == '\0' || *ptr == '#')
                break;

            if (isxdigit(*ptr)) {
                ptr = convert2hex(ptr, dptr++);
            } else {
                /* Invalid character on data line */
                ptr++;
            }
        }
    }

    return (dptr - dst);
}

/** 
 *  @brief Process calibration data
 *  @param argc		number of arguments
 *  @param argv         A pointer to arguments array    
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
int
process_cal_data_ext(int argc, char *argv[])
{
    s32 count;
    u8 *buf = NULL;
    FILE *fp;
    s8 filename[48] = "";
    HostCmd_DS_802_11_CAL_DATA_EXT *pcal_data;
    struct ifreq userdata;
    s32 CmdNum = CAL_DATA_EXT_CONFIG;
    int ioctl_val, subioctl_val;
    u16 action;

    if (get_priv_ioctl("setgetconf",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    if (argc != 4) {
        printf("Error: invalid no of arguments\n");
        printf("Syntax: ./wlanconfig eth1 caldataext <filename>\n");
        exit(1);
    }

    strncpy(filename, argv[3], MIN(sizeof(filename) - 1, strlen(argv[3])));
    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Cannot open file %s\n", argv[1]);
        exit(1);
    }

    buf = malloc(sizeof(HostCmd_DS_802_11_CAL_DATA_EXT) + sizeof(s32));
    if (buf == NULL)
        return WLAN_STATUS_FAILURE;
    pcal_data = (HostCmd_DS_802_11_CAL_DATA_EXT *) (buf + sizeof(s32));
    memset(pcal_data, 0, sizeof(HostCmd_DS_802_11_CAL_DATA_EXT));

    count = fparse_for_hex(fp, (u8 *) pcal_data);
    fclose(fp);

    action = pcal_data->Action;
    memcpy(buf, &CmdNum, sizeof(s32));
    strncpy(userdata.ifr_name, dev_name, IFNAMSIZ);
    userdata.ifr_data = buf;

    if (ioctl(sockfd, ioctl_val, &userdata)) {
        fprintf(stderr, "wlanconfig: CAL DATA not supported by %s\n",
                dev_name);
        free(buf);
        return WLAN_STATUS_FAILURE;
    }

    if (action == HostCmd_ACT_GET) {
        printf("Cal Data revision: %04x\n", pcal_data->Revision);
        printf("Cal Data length: %0d", pcal_data->CalDataLen);
        for (count = 0; count < pcal_data->CalDataLen; count++) {
            if ((count % 16) == 0) {
                printf("\n");
            }
            printf("%02x ", pcal_data->CalData[count]);
        }
        printf("\n");
    }

    free(buf);
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief read register
 *  @param cmd 		the type of register
 *  @param stroffset	A pointer to register index string
 *  @return            	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
process_read_register(s32 cmd, s8 * stroffset)
{
    struct ifreq userdata;
    wlan_ioctl_regrdwr reg;
    s8 *whichreg;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("regrdwr",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }
    memset(&reg, 0, sizeof(reg));
    switch (cmd) {
    case CMD_RDMAC:
        /*
         * HostCmd_CMD_MAC_REG_ACCESS 
         */
        reg.WhichReg = REG_MAC;
        whichreg = "MAC";
        break;
    case CMD_RDBBP:
        /*
         * HostCmd_CMD_BBP_REG_ACCESS 
         */
        reg.WhichReg = REG_BBP;
        whichreg = "BBP";
        break;
    case CMD_RDRF:
        /*
         * HostCmd_CMD_RF_REG_ACCESS 
         */
        reg.WhichReg = REG_RF;
        whichreg = "RF";
        break;
    default:
        fprintf(stderr, "Invalid Register set specified.\n");
        return -1;
    }

    reg.Action = 0;             /* READ */

    if (!strncasecmp(stroffset, "0x", 2))
        reg.Offset = a2hex((stroffset + 2));
    else
        reg.Offset = atoi(stroffset);

    strncpy(userdata.ifr_name, dev_name, IFNAMSIZ);
    userdata.ifr_data = (s8 *) & reg;

    if (ioctl(sockfd, ioctl_val, &userdata)) {
        perror("wlanconfig");
        fprintf(stderr,
                "wlanconfig: Register Reading not supported by"
                "interface %s\n", dev_name);
        return WLAN_STATUS_FAILURE;
    }

    printf("%s[0x%04lx] = 0x%08lx\n", whichreg, reg.Offset, reg.Value);

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief write register
 *  @param cmd 		the type of register
 *  @param stroffset	A pointer to register index string
 *  @param strvalue	A pointer to the register value
 *  @return            	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
process_write_register(s32 cmd, s8 * stroffset, s8 * strvalue)
{
    struct ifreq userdata;
    wlan_ioctl_regrdwr reg;
    s8 *whichreg;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("regrdwr",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    switch (cmd) {
    case CMD_WRMAC:
        /*
         * HostCmd_CMD_MAC_REG_ACCESS 
         */
        reg.WhichReg = REG_MAC;
        whichreg = "MAC";
        break;
    case CMD_WRBBP:
        /*
         * HostCmd_CMD_BBP_REG_ACCESS 
         */
        reg.WhichReg = REG_BBP;
        whichreg = "BBP";
        break;
    case CMD_WRRF:
        /*
         * HostCmd_CMD_RF_REG_ACCESS 
         */
        reg.WhichReg = REG_RF;
        whichreg = "RF";
        break;
    default:
        fprintf(stderr, "Invalid register set specified.\n");
        return -1;
    }

    reg.Action = 1;             /* WRITE */

    if (!strncasecmp(stroffset, "0x", 2))
        reg.Offset = a2hex((stroffset + 2));
    else
        reg.Offset = atoi(stroffset);

    if (!strncasecmp(strvalue, "0x", 2))
        reg.Value = a2hex((strvalue + 2));
    else
        reg.Value = atoi(strvalue);

    printf("Writing %s Register 0x%04lx with 0x%08lx\n", whichreg,
           reg.Offset, reg.Value);

    strncpy(userdata.ifr_name, dev_name, IFNAMSIZ);
    userdata.ifr_data = (s8 *) & reg;

    if (ioctl(sockfd, ioctl_val, &userdata)) {
        perror("wlanconfig");
        fprintf(stderr,
                "wlanconfig: Register Writing not supported "
                "by interface %s\n", dev_name);
        return WLAN_STATUS_FAILURE;
    }

    printf("%s[0x%04lx] = 0x%08lx\n", whichreg, reg.Offset, reg.Value);

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Get the CFP table based on the region code
 *
 *  @param argc     number of arguments
 *  @param argv     A pointer to arguments array    
 *
 *  @return         WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_getcfptable(int argc, char *argv[])
{
    pwlan_ioctl_cfp_table cfptable;
    int ioctl_val, subioctl_val;
    struct iwreq iwr;
    int region = 0;
    int i;

    if (get_priv_ioctl("getcfptable",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    if (argc > 4) {
        printf("Error: invalid number of arguments\n");
        printf("Syntax: ./wlanconfig ethX getcfptable [regioncode]");
        return -EINVAL;
    }

    if (argc == 4) {
        if ((region = atoval(argv[3])) < 0)
            return -EINVAL;
    }

    cfptable = (pwlan_ioctl_cfp_table) malloc(sizeof(wlan_ioctl_cfp_table));
    if (cfptable == NULL) {
        printf("Error: allocate memory for CFP table failed\n");
        return -ENOMEM;
    }
    memset(cfptable, 0, sizeof(wlan_ioctl_cfp_table));

    cfptable->region = region;
    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = (caddr_t) cfptable;
    iwr.u.data.length = sizeof(wlan_ioctl_cfp_table);
    iwr.u.data.flags = subioctl_val;

    if (ioctl(sockfd, ioctl_val, &iwr)) {
        fprintf(stderr, "wlanconfig: get CFP table failed\n");
        free(cfptable);
        return WLAN_STATUS_FAILURE;
    }

    printf("-------------------------------\n");
    printf(" #  | ch  | freq | max_tx_power\n");
    printf("-------------------------------\n");

    for (i = 0; i < cfptable->cfp_no; i++) {
        printf(" %02u | %03d | %04ld | %02d\n",
               i + 1,
               cfptable->cfp[i].Channel,
               cfptable->cfp[i].Freq, cfptable->cfp[i].MaxTxPower);
    }

    free(cfptable);
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Process arpfilter
 *  @param argc     number of arguments
 *  @param argv     A pointer to arguments array    
 *  @return	    WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_arpfilter(int argc, char *argv[])
{
    s8 line[256], *pos;
    u8 *buf;
    FILE *fp;
    struct iwreq iwr;
    int ln = 0;
    int arpfilter_found = 0;
    u16 length = 0;
    int ret = WLAN_STATUS_SUCCESS;
    int ioctl_val, subioctl_val;

    if (get_priv_ioctl("arpfilter",
                       &ioctl_val, &subioctl_val) == WLAN_STATUS_FAILURE) {
        return -EOPNOTSUPP;
    }

    if (argc < 4) {
        printf("Error: invalid no of arguments\n");
        printf("Syntax: ./wlanconfig eth1 arpfilter <arpfilter.conf>\n");
        exit(1);
    }

    if ((fp = fopen(argv[3], "r")) == NULL) {
        fprintf(stderr, "Cannot open file %s\n", argv[3]);
        exit(1);
    }

    buf = (u8 *) malloc(MRVDRV_SIZE_OF_CMD_BUFFER);
    if (buf == NULL) {
        printf("Error: allocate memory for arpfilter failed\n");
        return -ENOMEM;
    }
    memset(buf, 0, MRVDRV_SIZE_OF_CMD_BUFFER);

    arpfilter_found = 0;
    while ((pos = wlan_config_get_line(line, sizeof(line), fp, &ln))) {
        if (strcmp(pos, "arpfilter={") == 0) {
            arpfilter_found = 1;
            wlan_get_hostcmd_data(fp, &ln, buf + length, &length);
            break;
        }
    }

    fclose(fp);

    if (!arpfilter_found) {
        fprintf(stderr, "wlanconfig: 'arpfilter' not found in file '%s'\n",
                argv[3]);
        ret = -1;
        goto _exit_;
    }

    memset(&iwr, 0, sizeof(iwr));
    strncpy(iwr.ifr_name, dev_name, IFNAMSIZ);
    iwr.u.data.pointer = buf;
    iwr.u.data.length = length;

    iwr.u.data.flags = 0;
    if (ioctl(sockfd, ioctl_val, &iwr)) {
        fprintf(stderr, "wlanconfig: WLANARPFILTER failed\n");
        ret = -1;
        goto _exit_;
    }

  _exit_:
    if (buf)
        free(buf);

    return WLAN_STATUS_SUCCESS;
}

/********************************************************
		Global Functions
********************************************************/
/* 
 *  @brief Entry function for wlanconfig
 *  @param argc		number of arguments
 *  @param argv         A pointer to arguments array    
 *  @return      	WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
int
main(int argc, char *argv[])
{
    s32 cmd;

    if ((argc == 2) && (strcmp(argv[1], "-v") == 0)) {
        fprintf(stdout, "Marvell wlanconfig version %s\n", WLANCONFIG_VER);
        exit(0);
    }
    if (argc < 3) {
        fprintf(stderr, "Invalid number of parameters!\n");
        display_usage();
        exit(1);
    }

    strncpy(dev_name, argv[1], IFNAMSIZ);

    /*
     * create a socket 
     */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "wlanconfig: Cannot open socket.\n");
        exit(1);
    }
    if (get_range() < 0) {
        fprintf(stderr, "wlanconfig: Cannot get range.\n");
        exit(1);
    }
    switch ((cmd = findcommand(NELEMENTS(commands), commands, argv[2]))) {
    case CMD_HOSTCMD:
        process_host_cmd(argc, argv);
        break;
    case CMD_RDMAC:
    case CMD_RDBBP:
    case CMD_RDRF:
        if (argc < 4) {
            fprintf(stderr, "Register offset required!\n");
            display_usage();
            exit(1);
        }

        if (process_read_register(cmd, argv[3])) {
            fprintf(stderr, "Read command failed!\n");
            exit(1);
        }
        break;
    case CMD_WRMAC:
    case CMD_WRBBP:
    case CMD_WRRF:
        if (argc < 5) {
            fprintf(stderr, "Register offset required & value!\n");
            display_usage();
            exit(1);
        }
        if (process_write_register(cmd, argv[3], argv[4])) {
            fprintf(stderr, "Write command failed!\n");
            exit(1);
        }
        break;
    case CMD_CMD52R:
        process_sdcmd52r(argc, argv);
        break;
    case CMD_CMD52W:
        process_sdcmd52w(argc, argv);
        break;
    case CMD_CMD53R:
        process_sdcmd53r();
        break;
    case CMD_BG_SCAN_CONFIG:
        process_bg_scan_config(argc, argv);
        break;
    case CMD_WMM_QSTATUS:
        process_wmm_qstatus(argc, argv);
        break;
    case CMD_WMM_ACK_POLICY:
        process_wmm_ack_policy(argc, argv);
        break;
    case CMD_WMM_AC_WPAIE:
        process_wmm_para_conf(argc, argv, cmd);
        break;
    case CMD_ADDTS:
        if (process_addts(argc, argv)) {
            exit(1);
        }
        break;
    case CMD_DELTS:
        if (process_delts(argc, argv)) {
            exit(1);
        }
        break;
    case CMD_QCONFIG:
        if (process_qconfig(argc, argv)) {
            exit(1);
        }
        break;
    case CMD_QSTATS:
        if (process_qstats(argc, argv)) {
            exit(1);
        }
        break;
    case CMD_CAL_DATA_EXT:
        process_cal_data_ext(argc, argv);
        break;
    case CMD_RDEEPROM:
        printf("proces read eeprom\n");

        if (argc < 5) {
            fprintf(stderr, "Register offset, number of bytes required\n");
            display_usage();
            exit(1);
        }

        if (process_read_eeprom(argv[3], argv[4])) {
            fprintf(stderr, "EEPROM Read failed\n");
            display_usage();
            exit(1);
        }
        break;
    case CMD_GETRATE:
        if (process_get_rate()) {
            fprintf(stderr, "Get Rate Failed\n");
            display_usage();
            exit(1);
        }
        break;
    case CMD_SLEEPPARAMS:
        if (process_sleep_params(argc, argv)) {
            fprintf(stderr, "Sleep Params Failed\n");
            display_usage();
            exit(1);
        }
        break;
    case CMD_BCA_TS:
        if (process_bca_ts(argc, argv)) {
            fprintf(stderr, "SetBcaTs Failed\n");
            display_usage();
            exit(1);
        }
        break;
    case CMD_REASSOCIATE:
        if (process_reassociation(argc, argv)) {
            exit(1);
        }
        break;
    case CMD_EXTSCAN:
        if (process_extscan(argc, argv)) {
            fprintf(stderr, "ExtScan Failed\n");
            display_usage();
            exit(1);
        }
        break;
    case CMD_SCAN_LIST:
        if (process_scan_results(argc, argv)) {
            fprintf(stderr, "getscanlist Failed\n");
            display_usage();
            exit(1);
        }
        break;
    case CMD_SET_GEN_IE:
        if (process_setgenie()) {
            exit(1);
        }
        break;
    case CMD_GET_SCAN_RSP:
        if (process_getscantable(argc, argv)) {
            exit(1);
        }
        break;

    case CMD_SET_USER_SCAN:
        if (process_setuserscan(argc, argv)) {
            exit(1);
        }
        break;

    case CMD_SET_MRVL_TLV:
        if (process_setmrvltlv()) {
            exit(1);
        }
        break;

    case CMD_GET_ASSOC_RSP:
        if (process_getassocrsp()) {
            exit(1);
        }
        break;
    case CMD_GET_CFP_TABLE:
        if (process_getcfptable(argc, argv)) {
            exit(1);
        }
        break;
    case CMD_ARPFILTER:
        process_arpfilter(argc, argv);
        break;
    default:
        fprintf(stderr, "Invalid command specified!\n");
        display_usage();
        exit(1);
    }

    return WLAN_STATUS_SUCCESS;
}
