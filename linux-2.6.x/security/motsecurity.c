/*  
 * security/motsecurity.c
 *
 * Motorola specific security hooks
 *  
 * Copyright (C) 2006-2008 Motorola, Inc.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 *  Date     Author    Comment
 *  10/2006  Motorola  Added initial security hooks for secure mounts
 *  10/2006  Motorola  Added kernel level file permission hooks
 *  11/2006  Motorola  Added code from commoncap.c to add OMA2 DRM permissions
 *  12/2006  Motorola  Changed location of pds security files
 *  12/2006  Motorola  Removed wmdrmapp as trusted application
 *  12/2006  Motorola  Added security hooks for rmdir, mkdir, and symlink
 *  12/2006  Motorola  Added OMA2 DRM permissions to additional files
 *  02/2007  Motorola  Updated permissions for generate_ivt and secclkd
 *  03/2007  Motorola  Added /usr/SYSqtapp/motosync/motosync to secure_drm_paths
 *  04/2007  Motorola  Used correct pointer in call to free_page()
 *  04/2007  Motorola  Created USER_APPS_AUTH domain
 *  04/2007  Motorola  Enable full coredump for non-security related applications
 *  05/2007  Motorola  Added ivtpdsFScounter to etc_security_files
 *  05/2007  Motorola  Added /usr/SYSqtapp/provision/as_prov to secure_drm_paths
 *  05/2007  Motorola  Added /usr/SYSqtapp/downloadmanager/downloadmanager to secure_drm_paths
 *  07/2007  Motorola  Added /usr/SYSqtapp/email/email to secure_drm_paths  
 *  07/2007  Motorola  Removed /usr/SYSqtapp/wmdrm/wmdrmapp
 *  07/2007  Motorola  Added /usr/SYSqtapps/mediafinder/mediafinder
 *  09/2007  Motorola  Added /usr/bin/mediafinder
 *  03/2008  Motorola  Protected /ezxlocal/etc from renaming
 *  10/2008  Motorola  Disable /usr/SYSqtapp/wmdrm/wmdrmapp
*/

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/security.h>
#include <linux/dcache.h>
#include <linux/namei.h>
#include <linux/list.h>
#include <linux/rwsem.h>

#define MY_NAME "mot_security"

#if defined(CONFIG_MOT_FEAT_SECURE_CLOCK) || defined(CONFIG_MOT_FEAT_SECURE_DRM)
#include <linux/root_dev.h>

static inline int on_rootfs(struct dentry* dentry)
{
    if (! dentry->d_sb)
    {
        /* Kernel passed itself a dentry without a superblock. Grant access. */
        return 1;
    }

    return (dentry->d_sb->s_dev == ROOT_DEV);
}
#endif /* CONFIG_MOT_FEAT_SECURE_CLOCK || CONFIG_MOT_FEAT_SECURE_DRM */

#ifdef CONFIG_MOT_FEAT_SECURE_DRM

#define JANUS_DRM_WRITE 0x00000001
#define JANUS_DRM_READ  0x00000002
#define JANUS_DRM_RW    (JANUS_DRM_WRITE | JANUS_DRM_READ)

#define SECURE_CLOCK_READ  0x00000004
#define SECURE_CLOCK_WRITE 0x00000008
#define SECURE_CLOCK_RW    (SECURE_CLOCK_WRITE | SECURE_CLOCK_READ)

#define OMA2_DRM_READ      0x00000010
#define OMA2_DRM_WRITE     0x00000020
#define OMA2_DRM_RW        (OMA2_DRM_WRITE | OMA2_DRM_READ)

#define USER_APPS_AUTH_READ 0x000040
#define USER_APPS_AUTH_WRITE 0x000080
#define USER_APPS_AUTH_RW (USER_APPS_AUTH_READ | USER_APPS_AUTH_WRITE)

/* DRM_WRITE is the intersection of all *_WRITE flags */
#define DRM_WRITE (JANUS_DRM_WRITE | SECURE_CLOCK_WRITE | OMA2_DRM_WRITE | USER_APPS_AUTH_WRITE)

/* DRM_READ is the intersection of all *_READ flags */
#define DRM_READ (JANUS_DRM_READ | SECURE_CLOCK_READ | OMA2_DRM_READ | USER_APPS_AUTH_READ)

typedef struct mot_file_perms {
    char *filepath;
    int domain;
} mot_file_perms;

typedef struct mot_dir_perms {
    mot_file_perms perms;
    const mot_file_perms *files;
} mot_dir_perms;

/* Note: The .bak files are created temporarily and deleted after the writes to .dat files have been completed */

static const mot_file_perms etc_janus_files[] = 
                {
                    { "/etc/pds/janus/hashsdidmap.dat", JANUS_DRM_RW },
                    { "/etc/pds/janus/hashsdidmap.bak", JANUS_DRM_RW },
                    { NULL, 0 }
                };

static const mot_file_perms etc_security_files[] = 
                {
                    { "/etc/pds/security/ivtcounter.dat", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/etc/pds/security/ivtcounter.bak", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/etc/pds/security/ivtkey.dat", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/etc/pds/security/ivtkey.bak", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/etc/pds/security/keyring.dat", JANUS_DRM_RW | OMA2_DRM_RW },
                    { "/etc/pds/security/keyring.bak", JANUS_DRM_RW | OMA2_DRM_RW },
                    { "/etc/pds/security/keymaptable.dat", JANUS_DRM_RW | OMA2_DRM_RW },
                    { "/etc/pds/security/keymaptable.bak", JANUS_DRM_RW | OMA2_DRM_RW },
                    { "/etc/pds/security/janusCertTemplate.dat", JANUS_DRM_RW },
                    { "/etc/pds/security/janusCertTemplate.bak", JANUS_DRM_RW },
                    { "/etc/pds/security/janusDeviceCert.dat", JANUS_DRM_RW },
                    { "/etc/pds/security/janusDeviceCert.bak", JANUS_DRM_RW },
                    { "/etc/pds/security/ivt.dat", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/etc/pds/security/ivt.bak", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/etc/pds/security/omaDRMmotorola.dat", OMA2_DRM_RW },
                    { "/etc/pds/security/omaDRMmotorola.bak", OMA2_DRM_RW },
                    { "/etc/pds/security/omaDRMcmla.dat", OMA2_DRM_RW },
                    { "/etc/pds/security/omaDRMcmla.bak", OMA2_DRM_RW },
                    { "/etc/pds/security/omaDRMfrancetelecom.dat", OMA2_DRM_RW },
                    { "/etc/pds/security/omaDRMfrancetelecom.bak", OMA2_DRM_RW },
                    { "/etc/pds/security/ivtpdsFScounter.dat", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/etc/pds/security/ivtpdsFScounter.bak", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { NULL, 0 }
                };

static const mot_file_perms ezxlocal_tpa_files[] = 
                {
                    { "/ezxlocal/etc/tpa", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/ezxlocal/etc/tpa/bctr.dat", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/ezxlocal/etc/tpa/ivt.dat", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/ezxlocal/etc/tpa/ivt.temp", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/ezxlocal/etc/tpa/ivt.lock", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/ezxlocal/etc/tpa/keymgr.lock", JANUS_DRM_RW | OMA2_DRM_RW | USER_APPS_AUTH_RW },
                    { "/ezxlocal/etc/tpa/tpa.log", (JANUS_DRM_RW | SECURE_CLOCK_RW | OMA2_DRM_RW) },
                    { "/ezxlocal/etc/tpa/persistent", JANUS_DRM_WRITE | SECURE_CLOCK_WRITE | OMA2_DRM_WRITE | USER_APPS_AUTH_WRITE },
                    { "/ezxlocal/etc/tpa/persistent/ivtcounter.lock", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/ezxlocal/etc/tpa/persistent/ivtkey.lock", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/ezxlocal/etc/tpa/persistent/janusTimeStamp.dat", JANUS_DRM_RW },
                    { "/ezxlocal/etc/tpa/persistent/janusTimeStamp.bak", JANUS_DRM_RW },
                    { "/ezxlocal/etc/tpa/persistent/simPinStore.dat", JANUS_DRM_RW },
                    { "/ezxlocal/etc/tpa/persistent/simPinStore.bak", JANUS_DRM_RW },
                    { "/ezxlocal/etc/tpa/persistent/keyring.lock", JANUS_DRM_RW | OMA2_DRM_RW },
                    { "/ezxlocal/etc/tpa/persistent/keymaptable.lock", JANUS_DRM_RW | OMA2_DRM_RW },
                    { "/ezxlocal/etc/tpa/persistent/janusCertTemplate.lock", JANUS_DRM_RW },
                    { "/ezxlocal/etc/tpa/persistent/janusDeviceCert.lock", JANUS_DRM_RW },
                    { "/ezxlocal/etc/tpa/persistent/janusTimeStamp.lock", JANUS_DRM_RW },
                    { "/ezxlocal/etc/tpa/persistent/simPinStore.lock", JANUS_DRM_RW },
                    { "/ezxlocal/etc/tpa/persistent/ivt.lock", JANUS_DRM_RW | OMA2_DRM_RW | SECURE_CLOCK_RW | USER_APPS_AUTH_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkJanusOffset.dat", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkNitzOffset.dat", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkAlarm.dat", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkDrift.dat", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkLastRtcVal.dat", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkOMA2DRMOffset.dat", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkUserOffset.dat", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkJanusOffset.lock", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkNitzOffset.lock", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkAlarm.lock", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkDrift.lock", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkLastRtcVal.lock", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkOMA2DRMOffset.lock", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkUserOffset.lock", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkJanusOffset.bak", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkNitzOffset.bak", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkAlarm.bak", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkDrift.bak", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkLastRtcVal.bak", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkOMA2DRMOffset.bak", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/secclkUserOffset.bak", SECURE_CLOCK_RW },
                    { "/ezxlocal/etc/tpa/persistent/omaDRMmotorola.lock", OMA2_DRM_RW },
                    { "/ezxlocal/etc/tpa/persistent/omaDRMcmla.lock", OMA2_DRM_RW },
                    { "/ezxlocal/etc/tpa/persistent/omaDRMfrancetelecom.lock", OMA2_DRM_RW },
                    { NULL, 0 }
                };

static const mot_file_perms ezxlocal_oma2_files[] = 
                      {
                    { "/ezxlocal/download/appwrite/drm/oadb/data/AssetRO", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/BlackList", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/ConsentWhiteList", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/DCFMetadata", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/DomainContext", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/DomainRO", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/OCSPResponder", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/RICertificate", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/RIContext", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/RIRO", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/Replay", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/ReplayInfo", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/ReplayNoTimestamp", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/ReplaySort", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/RiDomain", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/RightsObject", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/RightsObjectXML", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/TrustAnchor", OMA2_DRM_RW },
                    { "/ezxlocal/download/appwrite/drm/oadb/data/WhiteList", OMA2_DRM_RW },
                    { NULL, 0 }
                      };

static const mot_dir_perms protected_dirs[] = 
                      {
                    { { "/etc/pds/janus", JANUS_DRM_WRITE }, etc_janus_files },
                    { { "/etc/pds/security", (JANUS_DRM_WRITE | SECURE_CLOCK_WRITE | OMA2_DRM_WRITE | USER_APPS_AUTH_WRITE) }, etc_security_files },
		    { { "/ezxlocal/etc", (JANUS_DRM_WRITE | SECURE_CLOCK_WRITE | OMA2_DRM_WRITE | USER_APPS_AUTH_WRITE) }, ezxlocal_tpa_files },
                    { { "/ezxlocal/download/appwrite/drm/oadb/data", OMA2_DRM_WRITE }, ezxlocal_oma2_files },
                    { { NULL, 0 }, NULL }
                      };

static const struct mot_file_perms* protected_file_info(struct dentry *dentry, 
                            struct vfsmount *rootmnt)
{
    const struct mot_file_perms *fp = NULL;
    char *filepath, *tmp;
    int idx;

    if (rootmnt == NULL)
    {
        return NULL;
    }

    if ((tmp = (char *)__get_free_page(GFP_KERNEL)) == NULL)
    {
        return ERR_PTR(-ENOMEM);
    }

    if (IS_ERR(filepath = abs_d_path(dentry, rootmnt, tmp, PAGE_SIZE)))
    {
        fp = (const struct mot_file_perms*)filepath;
        goto out;
    }

    if (filepath == NULL)
    {
        /* We were unable to determine the file path.  In this
           case, we have to grant permission, or system won't boot. */
        goto out;
    }

    for (idx = 0; protected_dirs[idx].files != NULL; ++idx)
    {
        char *dirname = protected_dirs[idx].perms.filepath;
        int len = strlen(dirname);

        if (strncmp(filepath, dirname, len) == 0)
        {
            const mot_file_perms *files;

            /* If the file is the directory itself, return it's permissions */

            if (strcmp(filepath, dirname) == 0)
            {
                fp = &protected_dirs[idx].perms;
                goto out;
            }

            /* Otherwise, if it's a child of the protected directory,
               find it & return its permissions structure */

            files = protected_dirs[idx].files;
            while (files->filepath != NULL)
            {
                if (strcmp(filepath, files->filepath) == 0)
                {
                    fp = files;
                    goto out;
                }

                ++files;
            }
        }
    }
    
out:
    free_page((unsigned long) tmp);

    return fp;
}
#endif /* CONFIG_MOT_FEAT_SECURE_DRM */

#if defined(CONFIG_MOT_FEAT_SECURE_CLOCK) || defined(CONFIG_MOT_FEAT_SECURE_DRM)
int mot_security_bprm_set_security (struct linux_binprm *bprm)
{
    /* Maintain the original capabilities security functions */
    int rc;
    if((rc = cap_bprm_set_security(bprm))) {
        return rc;
    }

#ifdef CONFIG_MOT_FEAT_SECURE_CLOCK
    if ((bprm->file->f_dentry) && (bprm->file->f_dentry->d_name.name) && (on_rootfs(bprm->file->f_dentry)))
    {
        /* If the executable belongs to the list of priv'd binaries 
           that are allowed to set the secure clock, give it the 
           CAP_SECURE_CLOCK capability. This is the only possible 
           way this CAP gets set. NOTE: If you add any apps to this 
           list, make sure to update MAXLEN below, or it will not 
           ever get the capability set on it! */

        const unsigned char *secure_clock_paths[] = {
            "/usr/local/bin/secclkd", /* Secure clock daemon */
            "/bin/tcmd", /* Test command binary */
            NULL 
        };

        /* MAXLEN is the length of the longest path in the
           whitelist of priv'd apps (currently: 
           (strlen("/usr/local/bin/secclkd") + 1) == 23)
           IF YOU ADD AN APPLICATION to the whitelist, this might change! */
        #define MAXLEN 23 

        int i = 0;
        unsigned char buf[MAXLEN];
        const unsigned char *path = d_path(bprm->file->f_dentry,
            bprm->file->f_vfsmnt, buf, MAXLEN);

        if (IS_ERR(path))
        {
            goto out;
        }

        while ((secure_clock_paths[i] != NULL) && 
               (*secure_clock_paths[i] != '\0'))
        {
            int maxlen = strlen(secure_clock_paths[i]);
            if (strncmp(path, secure_clock_paths[i], 
                    (maxlen > MAXLEN) ? MAXLEN : maxlen) == 0)
            {
                /* Add CAP_SECURE_CLOCK to the permitted and 
                   inheritable capability sets for the current 
                   process, so that when applying the bprm sets in
                   cap_bprm_apply_creds, the "working" set will
                   contain CAP_SECURE_CLOCK */

                cap_raise(current->cap_permitted, CAP_SECURE_CLOCK);
                cap_raise(current->cap_inheritable, CAP_SECURE_CLOCK);

                /* Now, we add the CAP_SECURE_CLOCK to the effective
                   and inheritable sets of the bprm, so that it will
                   end up in the permitted and effective sets at the
                   end of cap_bprm_apply_creds */

                cap_raise(bprm->cap_effective, CAP_SECURE_CLOCK);
                cap_raise(bprm->cap_inheritable, CAP_SECURE_CLOCK);
                break;
            }

            ++i;
        }
    }

out:
#endif /* CONFIG_MOT_FEAT_SECURE_CLOCK */

#ifdef CONFIG_MOT_FEAT_SECURE_DRM
    bprm->security = 0; /* Clear security prvis from parent process */

    if ((bprm->file->f_dentry) && (bprm->file->f_dentry->d_name.name) && (on_rootfs(bprm->file->f_dentry)))
    {
        static const mot_file_perms secure_drm_paths[] = {
            { "/bin/modem_services", JANUS_DRM_RW },
            { "/usr/local/bin/generate_ivt", (JANUS_DRM_RW | SECURE_CLOCK_RW) },
            { "/usr/bin/mediaplayer", JANUS_DRM_RW },
            { "/usr/SYSqtapp/downloadmanager/downloadmanager", JANUS_DRM_RW },
            { "/usr/SYSqtapp/mediaplayer/mediaplayer", JANUS_DRM_RW },
            { "/usr/SYSqtapp/mediafinder/mediafinder", JANUS_DRM_RW},
            { "/usr/bin/mediafinder", JANUS_DRM_RW},
            { "/usr/bin/mfmsp/mfmsp", JANUS_DRM_RW },
            { "/usr/SYSqtapp/mystuff/mystuff", JANUS_DRM_RW },
            { "/usr/SYSjava/kvm", JANUS_DRM_RW },
            { "/usr/SYSqtapp/mtp/mtpmgr", JANUS_DRM_RW },
            { "/usr/SYSqtapp/messaging/messaging", JANUS_DRM_RW },
            { "/usr/local/bin/secclkd", SECURE_CLOCK_RW },
            { "/bin/tcmd", (JANUS_DRM_RW | SECURE_CLOCK_RW | OMA2_DRM_RW) },
            { "/usr/SYSqtapp/drmfs/drmfs", OMA2_DRM_RW },
            { "/usr/SYSqtapp/drmdaemon/udadaemon", OMA2_DRM_RW },
            { "/usr/SYSqtapp/email/email", USER_APPS_AUTH_RW},
            { "/usr/SYSqtapp/motosync/motosync",  USER_APPS_AUTH_RW },
            { "/usr/SYSqtapp/motosync/airsync",  USER_APPS_AUTH_RW },
            { "/usr/SYSqtapp/provision/as_prov", USER_APPS_AUTH_RW },
            { NULL, 0 }
        };

        char *filepath, *tmp = (char *)__get_free_page(GFP_KERNEL);
        int i;

        if (tmp == NULL)
        {
            return -ENOMEM;
        }

        filepath = abs_d_path(bprm->file->f_dentry, 
                      bprm->file->f_vfsmnt, tmp, PAGE_SIZE);

        if (! IS_ERR(filepath) && filepath != NULL)
        {
            for (i = 0; secure_drm_paths[i].filepath != NULL; ++i)
            {
                if (strcmp(secure_drm_paths[i].filepath, filepath) == 0)
                {
                    bprm->security = (void *)secure_drm_paths[i].domain;

                    break;
                }
            }
        }

        free_page((unsigned long) tmp);
    }

#endif /* CONFIG_MOT_FEAT_SECURE_DRM */

    return 0;
}
#endif /* CONFIG_MOT_FEAT_SECURE_CLOCK || CONFIG_MOT_FEAT_SECURE_DRM */

#ifdef CONFIG_MOT_FEAT_SECURE_DRM
void mot_security_bprm_apply_creds (struct linux_binprm *bprm, int unsafe)
{
    /* Maintain the original capabilities security functions */
    cap_bprm_apply_creds(bprm, unsafe);
    /* Added for secure DRM, set the current security pointer */
    current->security = bprm->security;
}
#endif /* CONFIG_MOT_FEAT_SECURE_DRM */

#ifdef CONFIG_MOT_FEAT_SECURE_DRM
int mot_security_inode_permission (struct inode *inode, int mask, struct nameidata *nd)
{
    const struct mot_file_perms *fp;
    int perms;
    int domain;

    if ((!nd) || (! nd->dentry) || (! nd->mnt))
    {
        /* Kernel passed itself an empty nameidata.
           Grant permission in this case, which only happens
           during system boot. */
        return 0;
    }

    fp = protected_file_info(nd->dentry, nd->mnt);

    if (IS_ERR(fp))
    {
        return PTR_ERR(fp);
    }

    if (fp == NULL)
    {
        return 0;
    }

    domain = fp->domain;
    perms = (int)current->security;

    // Check to see if file is write-protected
    if ((mask & MAY_WRITE) && 
        ((domain & DRM_WRITE) != 0) && (((domain & perms) & DRM_WRITE) == 0))
    {
        return -EPERM;
    }

    // Check to see if file is read-protected
    if ((mask & MAY_READ) && 
        (((domain & DRM_READ) != 0) &&
         (((domain & perms) & DRM_READ) == 0)))
    {
        return -EPERM;
    }

    return 0;
}

int mot_security_inode_link (struct dentry *old_dentry, struct inode *dir,
            struct dentry *new_dentry)
{
    struct vfsmount *old_vfsmnt = old_dentry->d_sb->s_vfsmount;
    struct vfsmount *new_vfsmnt = new_dentry->d_sb->s_vfsmount;
    const struct mot_file_perms *fp1 = protected_file_info(old_dentry, old_vfsmnt);
    const struct mot_file_perms *fp2 = protected_file_info(new_dentry, new_vfsmnt);


    if ((IS_ERR(fp1)) || (IS_ERR(fp2)))
    {
        return IS_ERR(fp1) ? PTR_ERR(fp1) : PTR_ERR(fp2);
    }

    if ((fp1 != NULL) || (fp2 != NULL))
    {
        return -EPERM;
    }

    return 0;
}

int mot_security_inode_create (struct inode *dir, struct dentry *dentry, int mode)
{
    struct vfsmount *vfsmnt = dentry->d_sb->s_vfsmount;
    const struct mot_file_perms *fp = protected_file_info(dentry, vfsmnt);

    if (IS_ERR(fp))
    {
        return PTR_ERR(fp);
    }

    if (fp != NULL)
    {
        int domain = fp->domain;

        if (((domain & DRM_WRITE) != 0) &&
            (((domain & (unsigned long)current->security) & DRM_WRITE) == 0))
        {
            /* You cannot create a file unless you have 
               write access to it */
            return -EPERM;
        }
    }

    return 0;
}

int mot_security_inode_symlink (struct inode *dir,
                                struct dentry *dentry, const char *old_name)
{
   /* Check for write access to symlink inode.  The symlink target
      will be dereferenced on access, and the security will be checked then */
   return mot_security_inode_create(dir, dentry, 0);
}

int mot_security_inode_unlink (struct inode *dir, struct dentry *dentry)
{
    struct vfsmount *vfsmnt = dentry->d_sb->s_vfsmount;
    const struct mot_file_perms *fp = protected_file_info(dentry, vfsmnt);

    if (IS_ERR(fp))
    {
        return PTR_ERR(fp);
    }

    if (fp != NULL)
    {
        int domain = fp->domain;

        if (((domain & DRM_WRITE) != 0) &&
            (((domain & (unsigned long)current->security) & DRM_WRITE) == 0))
        {
            /* You cannot delete a file unless you have 
               write access to it */
            return -EPERM;
        }
    }

    return 0;
}

int mot_security_inode_rename (struct inode *old_dir,
              struct dentry *old_dentry,
              struct inode *new_dir,
              struct dentry *new_dentry)
{
    struct vfsmount *old_vfsmnt = old_dentry->d_sb->s_vfsmount;
    struct vfsmount *new_vfsmnt = new_dentry->d_sb->s_vfsmount;
    const struct mot_file_perms *fp1 = protected_file_info(old_dentry, old_vfsmnt);
    const struct mot_file_perms *fp2 = protected_file_info(new_dentry, new_vfsmnt);
    int domain1;
    int domain2;

    if ((IS_ERR(fp1)) || (IS_ERR(fp2)))
    {
        return IS_ERR(fp1) ? PTR_ERR(fp1) : PTR_ERR(fp2);
    }

    if (fp1 == NULL && fp2 == NULL)
    {
        /* Two unprotected files, so it's ok. */
        return 0;
    }

    if (fp1 != NULL && fp2 == NULL)
    {
        /* Cannot rename a protected file to unprotected */
        return -EPERM;
    }

    if (fp1 == NULL && fp2 != NULL)
    {
        if (((fp2->domain & DRM_WRITE) != 0) && ((fp2->domain & (unsigned long)current->security) & DRM_WRITE) == 0)
        {
            /* If you don't have write access to the destination file
               then you cannot rename to it */
            return -EPERM;
        }

        return 0;
    }

    /* From here on out, we know that both fp1 && fp2 are non-NULL */

    domain1 = fp1->domain;
    domain2 = fp2->domain;

    if (fp2->domain != fp1->domain)
    {
        /* Cannot use rename to change protections */
        return -EPERM;
    }

    if (((domain1 & DRM_READ) != 0) && (((domain1 & (unsigned long)current->security) & DRM_READ) == 0))
    {
        /* If you don't have read access to the source file,
           you can't rename it */
        return -EPERM;
    }

    if (((domain2 & DRM_WRITE) != 0) && ((domain2 & (unsigned long)current->security) & DRM_WRITE) == 0)
    {
        /* If you don't have write access to the destination file
           then you cannot rename to it */
        return -EPERM;
    }

    return 0;
}

int mot_security_inode_mknod (struct inode *dir, struct dentry *dentry, int mode, dev_t dev)
{
    struct vfsmount *vfsmnt = dentry->d_sb->s_vfsmount;
    const struct mot_file_perms *fp = protected_file_info(dentry, vfsmnt);

    if (IS_ERR(fp))
    {
        return PTR_ERR(fp);
    }

    if (fp != NULL)
    {
        int domain = fp->domain;

        if (((domain & DRM_WRITE) != 0) &&
            (((domain & (unsigned long)current->security) & DRM_WRITE) == 0))
        {
            /* You cannot create the device node
               unless you have write permission to the file */
            return -EPERM;
        }
    }

    return 0;
}

#ifdef CONFIG_MOT_FEAT_DRM_COREDUMP
/* 
 * Return 0 if current task does not have DRM_READ privilege 
 */
int mot_security_coredump(void)
{
	return (((unsigned long)current->security) & DRM_READ);
}
#endif /* CONFIG_MOT_FEAT_DRM_COREDUMP */
#endif /* CONFIG_MOT_FEAT_SECURE_DRM */

#ifdef CONFIG_MOT_FEAT_SECURE_MOUNT
static int mounts_locked;

struct motsec_mount {
    struct list_head list;
    char * device;
    char * mount_point;
    char * fs_type;
};

static DECLARE_RWSEM(allowed_mounts_sem);
static struct list_head allowed_mounts;

int motsec_add_allowed_mount(char * dev, char * mount, char * fs_type)
{
    struct motsec_mount *new_mount;

    if(mounts_locked){
        printk(KERN_WARNING "Cannot add allowed mount, mounts are locked\n");
        return -EPERM;
    }
    
    new_mount = kmalloc(sizeof(struct motsec_mount), GFP_KERNEL);
    if(!new_mount)
        return -ENOMEM;

    new_mount->device      = dev;
    new_mount->mount_point = mount;
    new_mount->fs_type     = fs_type;

    down_write(&allowed_mounts_sem);
    list_add_tail(&new_mount->list, &allowed_mounts);
    up_write(&allowed_mounts_sem);
        
    return 0;
}

ssize_t motsec_get_allowed_mounts(char * buff, ssize_t size) 
{
    int left, ret;
    struct motsec_mount *cur; 
    left = size;
    ret = snprintf(buff, left,
                    "Allowed Mounts:\nDevice\tMount Point\tFile System Type\n");
    left -= (ret < left) ? ret : left;
    
    down_read(&allowed_mounts_sem);
    if(allowed_mounts.next != &allowed_mounts) { 
        list_for_each_entry(cur, &allowed_mounts, list) {
            if(left && cur) {
                ret = snprintf(buff + (size - left), left,
                                    "%s\t%s\t%s\n",
                                    cur->device,
                                    cur->mount_point,
                                    cur->fs_type);
                left -= (ret < left) ? ret : left;
            }
        }
    }
    up_read(&allowed_mounts_sem);
    
    return size - left;
}

static int motsec_check_mount_allowed(char * dev, char * mount, char * fs_type)
{
    struct motsec_mount *cur;
    
    down_read(&allowed_mounts_sem);
    if(allowed_mounts.next != &allowed_mounts) { 
        list_for_each_entry(cur, &allowed_mounts, list) {
            if(cur && !(strcmp(dev, cur->device)        ||
                        strcmp(mount, cur->mount_point) ||
                        strcmp(fs_type, cur->fs_type))) {
                up_read(&allowed_mounts_sem);
                return 0;
            }
        }
    }
    up_read(&allowed_mounts_sem);
    return -EPERM;
}

void motsec_lock_mounts(void)
{
    mounts_locked = 1;
}

int motsec_mounts_locked(void)
{
    return mounts_locked;
}

static int mot_security_sb_mount (char *dev_name, struct nameidata *nd, 
                                  char *type, unsigned long flags, void *data)
{
    char * tmp;
    int rc;

    if(!mounts_locked) {
        return 0;
    }

    if ((tmp = (char *)__get_free_page(GFP_KERNEL)) == NULL) {
        return -ENOMEM;
    }

    rc = motsec_check_mount_allowed(dev_name, 
                                    abs_d_path(nd->dentry, nd->mnt, 
                                               tmp, PAGE_SIZE),
                                    type);
    
    free_page((unsigned long) tmp);
    return rc;
}
#endif /* CONFIG_MOT_FEAT_SECURE_MOUNT */

/* Define Motorola security function hooks */
static struct security_operations mot_security_ops = {
    /* Capabilities hooks preserved in this module */
    .ptrace                 = cap_ptrace,
    .capget                 = cap_capget,
    .capset_check           = cap_capset_check,
    .capset_set             = cap_capset_set,
    .capable                = cap_capable,
    .settime                = cap_settime,
    .netlink_send           = cap_netlink_send,
    .netlink_recv           = cap_netlink_recv,
    .bprm_secureexec        = cap_bprm_secureexec,
    .inode_setxattr         = cap_inode_setxattr,
    .inode_removexattr      = cap_inode_removexattr,
    .task_post_setuid       = cap_task_post_setuid,
    .task_reparent_to_init  = cap_task_reparent_to_init,
    .syslog                 = cap_syslog,
    .vm_enough_memory       = cap_vm_enough_memory,
#if defined(CONFIG_MOT_FEAT_SECURE_CLOCK) || defined(CONFIG_MOT_FEAT_SECURE_DRM)
    .bprm_set_security      = mot_security_bprm_set_security,
#else
    .bprm_set_security      = cap_bprm_set_security,
#endif /* CONFIG_MOT_FEAT_SECURE_CLOCK || CONFIG_MOT_FEAT_SECURE_DRM */ 
#ifdef CONFIG_MOT_FEAT_SECURE_DRM
    .bprm_apply_creds       = mot_security_bprm_apply_creds,
    .inode_permission       = mot_security_inode_permission,
    .inode_link             = mot_security_inode_link,
    .inode_symlink          = mot_security_inode_symlink,
    .inode_create           = mot_security_inode_create,
    .inode_unlink           = mot_security_inode_unlink,
    .inode_mkdir            = mot_security_inode_create,
    .inode_rmdir            = mot_security_inode_unlink,
    .inode_rename           = mot_security_inode_rename,
    .inode_mknod            = mot_security_inode_mknod,
#else
    .bprm_apply_creds       = cap_bprm_apply_creds,
#endif /* CONFIG_MOT_FEAT_SECURE_DRM */
#ifdef CONFIG_MOT_FEAT_SECURE_MOUNT
    .sb_mount               = mot_security_sb_mount
#endif /* CONFIG_MOT_FEAT_SECURE_MOUNT */
};

static int secondary = 0;

static int __init mot_security_init (void)
{
    if(register_security(&mot_security_ops)) {
        printk(KERN_INFO 
            "Could not register Motorola Security Module with kernel.\n");
        if(mod_reg_security(MY_NAME, &mot_security_ops)) {
            printk(KERN_INFO
                "Could not register Motorola Security Module with primary "
                "security module.\n");
            panic("Motorola Security Module could not be loaded.\n");
            return -EINVAL;
        }
        secondary = 1;
    }
    printk(KERN_INFO "Motorola Security Module initialized: %s module.\n",
                        secondary ? "Secondary" : "Primary");   

#ifdef CONFIG_MOT_FEAT_SECURE_MOUNT
    /* Initialize mount variables */
    mounts_locked = 0;
    INIT_LIST_HEAD(&allowed_mounts);
#endif /* CONFIG_MOT_FEAT_SECURE_MOUNT */
 
    return 0;
}

static void __exit mot_security_exit (void)
{
    /* Unregister LSM */
    if(secondary) {
        if(mod_unreg_security (MY_NAME, &mot_security_ops))
            printk(KERN_INFO "Could not unregister Motorola Security Module "
                "with primary module.\n");
    }    
    else if (unregister_security (&mot_security_ops)) {
            printk(KERN_INFO "Could not unregister Motorola Security Module "
                "with kernel.\n");
    }
    printk (KERN_INFO "Motorola Security Module removed.\n");
}

security_initcall(mot_security_init);
module_exit(mot_security_exit);

MODULE_AUTHOR("Motorola, Inc.");
MODULE_DESCRIPTION("Motorola Security Module");
MODULE_LICENSE("GPL");

#ifdef CONFIG_MOT_FEAT_SECURE_MOUNT
EXPORT_SYMBOL(motsec_mounts_locked);
EXPORT_SYMBOL(motsec_lock_mounts);
EXPORT_SYMBOL(motsec_add_allowed_mount);
EXPORT_SYMBOL(motsec_get_allowed_mounts);
#endif /* CONFIG_MOT_FEAT_SECURE_MOUNT */
