/*
 * otg/msc/msc_scsi.h - mass storage protocol library header
 * @(#) balden@seth2.belcarratech.com|otg/functions/msc/msc-scsi.h|20051116204958|34809
 *
 *      Copyright(c) 2004-2005, Belcarra
 *
 * Adapated from work:
 *
 *      Copyright (c) 2003 Lineo Solutions, Inc.
 *
 * Written by Shunnosuke kabata
 *
 *  Copyright (c) 2005-2007, Motorola, All Rights Reserved.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 08/04/2005         Motorola         Initial distribution 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 * 05/21/2007         Motorola         Changes for ANSI Version.
 * 07/26/2007         Motorola         Changes for Open src compliance.
 * 08/24/2007         Motorola         Changes for Open src compliance.
 *
 *  This program is licensed under a BSD license with the following terms:
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 *  Neither the name of Motorola nor the names of its contributors may be
 *  used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 *
 * Documents
 *      Universal Serial Bus Mass Storage Class - Specification Overview
 *      Universal Serial Bus Mass Storage Class - Bulk-Only Transport
 *      T10/1240-D - Information technology - Reduced Block Commands
 *
 * Notes
 *
 * 1. Reduced Block Command set C.f. Table 2 RBC
 *
 *                                              Command Support
 *      Name                            OpCode  Fixed   Removable       Reference
 *      Format Unit                     04h     O       O               RBC
 *      Inquiry                         12h     M       M               SPC-2
 *      Mode Select (6)                 15h     M       M               SPC-2
 *      Mode Sense (6)                  1Ah     M       M               SPC-2
 *      Perisstent Reserve In           5Eh     O       O               SPC-2
 *      Persistent Reserve Out          5Fh     O       O               SPC-2
 *      Prevent/Allow Medium Removal    1Eh     N/A     M               SPC-2
 *      Read (10)                       28h     M       M               RBC
 *      Read Capacity                   25h     M       M               RBC
 *      Reelase (6)                     17h     O       O               SPC-2
 *      Request Sense                   03h     O       O               SPC-2
 *      Reserve (6)                     16h     O       O               SPC-2
 *      Start Stop Unit                 1Bh     M       M               RBC
 *      Synchronize Cache               35h     O       O               RBC
 *      Test Unit Ready                 00h     M       M               SPC-2
 *      Verify (10)                     2Fh     M       M               RBC
 *      Write (10)                      2Ah     M       M               RBC
 *      Write Buffer                    3Bh     M       O               SPC-2
 *
 * 2. Other commands seen?
 *                                                      Sh      MV      FS
 *      SCSI_REZERO_UNIT                     0x01       no
 *      SCSI_READ_6                          0x08                       yes
 *      SCSI_WRITE_6                         0x0a                       yes
 *      SCSI_SEND_DIAGNOSTIC                 0x1d       no              yes
 *      SCSI_READ_FORMAT_CAPACITY            0x23       yes             yes
 *      SCSI_WRITE_AND_VERIFY                0x2e       no
 *      SCSI_SEEK_10                         0x2b       no
 *      SCSI_MODE_SELECT_10                  0x55       no      yes     yes
 *      SCSI_READ_12                         0xa8       no              yes
 *      SCSI_WRITE_12                        0xaa       no              yes
 *
 *
 * 3. Status - C.f. Chapter 5 - RBC
 *
 *      Check Condition                 02h
 *
 *
 * 4. Sense Keys - C.f. Chapter 5 - RBC
 *      
 *      Not Ready                       02h
 *      Media Error                     03h
 *      Illegal Request                 05h
 *      Unit Attention                  06h
 *
 * 5. ASC/ASCQ - C.f. Chapter 5 - RBC
 *
 *      Logical Unit Not Ready                                          04h
 *      Logical Unit Not Ready, Format in Progress                      04h,04h
 *      Invalid Command Operation Code                                  20h
 *      Invalide Field in CDB                                           24h
 *      Format Command Failed                                           31h,01h
 *      Status Notification / Power Management Class Event              38h,02h
 *      Status Notification / Media Class Event                         38h,04h
 *      Status Notification / Device Busy Class Event                   38h,06h
 *      Low Power Condition Active                                      5Eh,00
 *      Power Condition Change to Active                                5Eh,41h
 *      Power Condition Change to Idle                                  5Eh,42h
 *      Power Condition Change to Standby                               5Eh,43h
 *      Power Condition Change to Sleep                                 5Eh,45h
 *      Power Condition Change to Device Control                        5Eh,47h
 *
 * 6. ASCQ - C.f. Chapter 5 - RBC
 *
 *
 *
 * 7. Sense Keys C.f. Chapter 5 - RBC
 *
 * Command                      Status                  Sense Key      ASC/ASCQ
 *
 * 5.1 Format Unit
 *   Format progress            Check Condition         Not Ready,      Logical Unit Note Ready, Format in Progress
 *   Sueccsful completion       Check Condition         Unit Attention, Status Notification / Media Class Event
 *   Failure                    Check Condition         Media error,    Format Command Failed
 *
 * 5.2 Read (10)
 *
 * 5.3 Read Capacity
 *   No Media                   Check Condition         Not Ready,      Logical Unit Not Ready
 *
 * 5.4 Start Stop Unit
 *   Power Consumption          Check Condition         Illegal Request,Low Power Condition Active
 *
 * 5.5 Synchronize Cache        Check Condition         Illegal Request,Invalid Command Operation Code
 *
 * 5.6 Write (10
 *
 * 5.7 Verify
 *
 * 5.8 Mode
 * 
 * 6.1 Inquiry
 *
 * 6.2 Mode Select (6)
 *   Cannot Save                Check Condition         Illegal Request,Invalid Field in CDB
 *
 * 6.3 Mode Sense (6)
 *
 * 6.4 Prevent Allow Medium Removal
 *
 * 6.5 Request Sense
 *
 * 6.6 Test Unit Ready
 *
 * 6.7 Write Buffer
 *
 * 7.1 Unit Attention
 *   Power Contition change     Check Condition         Unit Attention, Power Condition Change Notification
 *
 * 7.4.1 Event Status Sense
 *
 *   Power Management Event     Check Condition         Unit Attention, Event Status Notification / Power Managment
 *   Media Class Event          Check Condition         Unit Attention, Event Status Notification / Media Class
 *   Device Busy Event          Check Condition         Unit Attention, Event Status Notification / Device Busy Class
 *
 * 7.4.6 Removable Medium Device Initial Response
 *   Ready                      Check Condition         Unit Attention, Event Status Notification / Media Class Event 
 *   Power                      Check Condition         Unit Attention, Event Status Notification / Power Management Class Event
 */

#ifndef _MSCSCSIPROTO_H_
#define _MSCSCSIPROTO_H_


/*
 * Class Specific Requests - C.f. MSC BO Chapter 3
 */

#define MSC_BULKONLY_RESET      0xff
#define MSC_BULKONLY_GETMAXLUN  0xfe


/*
 * Class Code
 */

#define MASS_STORAGE_CLASS                  0x08

/*
 * MSC - Specification Overview
 *
 * SubClass Codes - C.f MSC Table 2.1
 */

#define MASS_STORAGE_SUBCLASS_RBC           0x01
#define MASS_STORAGE_SUBCLASS_SFF8020I      0x02
#define MASS_STORAGE_SUBCLASS_QIC157        0x03
#define MASS_STORAGE_SUBCLASS_UFI           0x04
#define MASS_STORAGE_SUBCLASS_SFF8070I      0x05
#define MASS_STORAGE_SUBCLASS_SCSI          0x06

/*
 * Protocol - C.f MSC Table 3.1
 */

#define MASS_STORAGE_PROTO_CBI_WITH_COMP    0x00
#define MASS_STORAGE_PROTO_CBI_NO_COMP      0x01
#define MASS_STORAGE_PROTO_BULK_ONLY        0x50

/*
 * SCSI Command
*/

#define SCSI_TEST_UNIT_READY                    0x00
#define SCSI_REQUEST_SENSE                      0x03
#define SCSI_FORMAT_UNIT                        0x04
#define SCSI_INQUIRY                            0x12
#define SCSI_MODE_SELECT_6                      0x15
#define SCSI_MODE_SENSE_6                       0x1a
#define SCSI_START_STOP                         0x1b
#define SCSI_PREVENT_ALLOW_MEDIA_REMOVAL        0x1e
#define SCSI_READ_FORMAT_CAPACITY               0x23
#define SCSI_READ_CAPACITY                      0x25
#define SCSI_READ_10                            0x28
#define SCSI_WRITE_10                           0x2a
#define SCSI_VERIFY                             0x2f

#define SCSI_READ_6                             0x08
#define SCSI_WRITE_6                            0x0a
#define SCSI_RESERVE                            0x16
#define SCSI_RELEASE                            0x17
#define SCSI_SEND_DIAGNOSTIC                    0x1d
#define SCSI_SYNCHRONIZE_CACHE                  0x35
#define SCSI_MODE_SENSE_10                      0x5a

#define SCSI_REZERO_UNIT                        0x01
#define SCSI_REASSIGN_BLOCKS                    0x07
#define SCSI_COPY                               0x18
#define SCSI_RECEIVE_DIAGNOSTIC_RESULTS         0x1c
#define SCSI_WRITE_AND_VERIFY                   0x2e
#define SCSI_PREFETCH                           0x34
#define SCSI_READ_DEFECT_DATA                   0x37
#define SCSI_COMPARE                            0x39
#define SCSI_COPY_AND_VERIFY                    0x3a
#define SCSI_WRITE_BUFFER                       0x3b
#define SCSI_READ_BUFFER                        0x3c
#define SCSI_READ_LONG                          0x3e
#define SCSI_WRITE_LONG                         0x3f
#define SCSI_CHANGE_DEFINITION                  0x40
#define SCSI_WRITE_SAME                         0x41
#define SCSI_LOG_SELECT                         0x4c
#define SCSI_LOG_SENSE                          0x4d
#define SCSI_XD_WRITE                           0x50
#define SCSI_XP_WRITE                           0x51
#define SCSI_XD_READ                            0x52
#define SCSI_MODE_SELECT_10                     0x55
#define SCSI_RESERVE_10                         0x56
#define SCSI_RELEASE_10                         0x57
#define SCSI_MODE_SELECT_10                     0x55
#define SCSI_XD_WRITE_EXTENDED                  0x80
#define SCSI_REBUILD                            0x81
#define SCSI_REGENERATE                         0x82

#define SCSI_SEEK_10                            0x2b
#define SCSI_WRITE_AND_VERIFY                   0x2e
#define SCSI_WRITE_12                           0xaa
#define SCSI_READ_12                            0xa8

/*
 * Private
 */
#define SCSI_PRIVATE_PCS                        0xff

/*
 * SCSI Command Parameter
 */

#define CBW_SIGNATURE           0x43425355  /* USBC */
#define CSW_SIGNATURE           0x53425355  /* USBS */

#define PRODUCT_REVISION_LEVEL  "1.00"

/*
 * Command Block Status Values - C.f MSC BO Table 5.3
 */

#define USB_MSC_PASSED  0x00            // good
#define USB_MSC_FAILED  0x01            // bad
#define USB_MSC_PHASE_ERROR     0x02    // we want to be reset




/*
 * SCSI Sense 
 *      SenseKey
 *      AdditionalSenseCode
 *      SenseCodeQualifier
 */

#define SCSI_ERROR_CURRENT                      0x70
#define SCSI_ERROR_DEFERRED                     0x07

/*
 *  SCSI Sense Keys
 */

#define SK_NO_SENSE            0x00
#define SK_RECOVERED_ERROR     0x01
#define SK_NOT_READY           0x02
#define SK_MEDIA_ERROR         0x03
#define SK_HARDWARE_ERROR      0x04
#define SK_ILLEGAL_REQUEST     0x05
#define SK_UNIT_ATTENTION      0x06
#define SK_DATA_PROTECT        0x07
#define SK_BLANK_CHECK         0x08
#define SK_COPY_ABORTED        0x0a
#define SK_ABORTED_COMMAND     0x0b
#define SK_VOLUME_OVERFLOW     0x0d
#define SK_MISCOMPARE          0x0e

/*
 * 5. ASC/ASCQ - C.f. Chapter 5 - RBC
 */

#define SK(SenseKey,ASC,ASCQ) ((SenseKey<<16) | (ASC << 8) | (ASCQ))

#define SCSI_SENSEKEY_NO_SENSE                          SK(SK_NO_SENSE,         0x00,0x00)      // 0x000000

#define SCSI_FAILURE_PREDICTION_THRESHOLD_EXCEEDED      SK(SK_RECOVERED_ERROR,  0x5d,0x00)      // 0x015d00

#define SCSI_SENSEKEY_LOGICAL_UNIT_NOT_READY            SK(SK_NOT_READY,        0x04,0x00)      // 0x020400
#define SCSI_SENSEKEY_FORMAT_IN_PROGRESS                SK(SK_NOT_READY,        0x04,0x04)      // 0x020404
#define SCSI_SENSEKEY_MEDIA_NOT_PRESENT                 SK(SK_NOT_READY,        0x3a,0x00)      // 0x023a00

#define SCSI_SENSEKEY_WRITE_ERROR                       SK(SK_MEDIA_ERROR,      0x0c,0x02)      // 0x030c02
#define SCSI_SENSEKEY_UNRECOVERED_READ_ERROR            SK(SK_MEDIA_ERROR,      0x11,0x00)      // 0x031100
#define SCSI_FORMAT_COMMAND_FAILED                      SK(SK_MEDIA_ERROR,      0x31,0x01)      // 0x033101

#define SCSI_SENSEKEY_COMMUNICATION_FAILURE             SK(SK_HARDWARE_ERROR,   0x08,0x00)       // 0x040800

#define SCSI_SENSEKEY_INVALID_COMMAND                   SK(SK_ILLEGAL_REQUEST,  0x20,0x00)      // 0x052000
#define SCSI_SENSEKEY_BLOCK_ADDRESS_OUT_OF_RANGE        SK(SK_ILLEGAL_REQUEST,  0x21,0x00)      // 0x052100
#define SCSI_SENSEKEY_INVALID_FIELD_IN_CDB              SK(SK_ILLEGAL_REQUEST,  0x24,0x00)      // 0x052400
#define SCSI_SENSEKEY_LOGICAL_UNIT_NOT_SUPPORTED        SK(SK_ILLEGAL_REQUEST,  0x25,0x00)      // 0x052500
#define SCSI_SENSEKEY_SAVING_PARAMETERS_NOT_SUPPORTED   SK(SK_ILLEGAL_REQUEST,  0x39,0x00)      // 0x053900
#define SCSI_MEDIA_REMOVAL_PREVENTED                    SK(SK_ILLEGAL_REQUEST,  0x53,0x02)      // 0x055302

#define SCSI_SENSEKEY_NOT_READY_TO_READY_CHANGE         SK(SK_UNIT_ATTENTION,   0x28,0x00)      // 0x062800
#define SCSI_SENSEKEY_RESET_OCCURRED                    SK(SK_UNIT_ATTENTION,   0x29,0x00)      // 0x062900

#define SCSI_SENSEKEY_STATUS_NOTIFICATION_POWER_CLASS   SK(SK_UNIT_ATTENTION,   0x38,0x02)      // 0x063802
#define SCSI_SENSEKEY_STATUS_NOTIFICATION_MEDIA_CLASS   SK(SK_UNIT_ATTENTION,   0x38,0x04)      // 0x063804
#define SCSI_SENSEKEY_STATUS_NOTIFICATION_DEVICE_BUSY   SK(SK_UNIT_ATTENTION,   0x38,0x06)      // 0x063806

#define SCSI_SENSEKEY_LOW_POWER_CONDITION_ACTIVE        SK(SK_UNIT_ATTENTION,   0x5e,0x00)      // 0x065e00
#define SCSI_SENSEKEY_POWER_CONDITION_CHANGE_TO_ACTIVE  SK(SK_UNIT_ATTENTION,   0x5e,0x41)      // 0x065e41
#define SCSI_SENSEKEY_POWER_CONDITION_CHANGE_TO_IDLE    SK(SK_UNIT_ATTENTION,   0x5e,0x42)      // 0x065e42
#define SCSI_SENSEKEY_POWER_CONDITION_CHANGE_TO_STANDBY SK(SK_UNIT_ATTENTION,   0x5e,0x43)      // 0x065e43
#define SCSI_SENSEKEY_POWER_CONDITION_CHANGE_TO_SLEEP   SK(SK_UNIT_ATTENTION,   0x5e,0x45)      // 0x065e45
#define SCSI_SENSEKEY_POWER_CONDITION_CHANGE_TO_DEVICE  SK(SK_UNIT_ATTENTION,   0x5e,0x47)      // 0x065e47


#define SCSI_SENSEKEY_WRITE_PROTECTED                   SK(SK_DATA_PROTECT,     0x27,0x00)      // 0x072700


/*
 * Mode Page Code and Page Control
 */

#define SCSI_MODESENSE_PAGE_CONTROL_CURRENT           0x0
#define SCSI_MODESENSE_PAGE_CONTROL_CHANGEABLE        0x1
#define SCSI_MODESENSE_PAGE_CONTROL_DEFAULT           0x2
#define SCSI_MODESENSE_PAGE_CONTROL_SAVED             0x3

#define SCSI_MODESENSE_PAGE_CODE_UNIT_ATTENTION            0x00
#define SCSI_MODESENSE_PAGE_CODE_ERROR_RECOVERY            0x01
#define SCSI_MODESENSE_PAGE_CODE_DISCONNNECT_RECONNECT     0x02
#define SCSI_MODESENSE_PAGE_CODE_FORMAT                    0x03
#define SCSI_MODESENSE_PAGE_CODE_RIGID_DRIVE_GEOMETRY      0x04
#define SCSI_MODESENSE_PAGE_CODE_FLEXIBLE_DISK_PAGE        0x05
#define SCSI_MODESENSE_PAGE_CODE_VERIFY_ERROR_RECOVERY     0x07
#define SCSI_MODESENSE_PAGE_CODE_CACHING                   0x08
#define SCSI_MODESENSE_PAGE_CODE_CONTROL_MODE              0x0a
#define SCSI_MODESENSE_PAGE_CODE_NOTCH_AND_PARTITION       0x0c
#define SCSI_MODESENSE_PAGE_CODE_POWER_CONDITION           0x0d
#define SCSI_MODESENSE_PAGE_CODE_XOR                       0x10
#define SCSI_MODESENSE_PAGE_CODE_CONTROL_MODE_ALIAS        0x1a
#define SCSI_MODESENSE_PAGE_CODE_REMOVABLE_BLOCK_ACCESS    0x1b
#define SCSI_MODESENSE_PAGE_CODE_INFORMATION_EXCEPTIONS    0x1c
#define SCSI_MODESENSE_PAGE_CODE_ALL_SUPPORTED             0x3f

#define SCSI_MODESENSE_MEDIUM_TYPE_DEFAULT                 0x00
#define SCSI_MODESENSE_MEDIUM_TYPE_FLEXIBLE_SGL_SIDED      0x01
#define SCSI_MODESENSE_MEDIUM_TYPE_FLEXIBLE_DBL_SIDED      0x02
      /* See Table 153 of SCSI-2 Spec for more medium types */

/*
 * Command Block Wrapper / Command Status Wrapper
 */

/*
 * Command Block Wrapper
 */
typedef struct{
        unsigned long   dCBWSignature;
        unsigned long   dCBWTag;
        unsigned long   dCBWDataTransferLength;
        u8   bmCBWFlags;
        u8   bCBWLUN:4,
             Reserved:4;
        u8   bCBWCBLength:5,
             Reserved2:3;
        u8   CBWCB[16];
} __attribute__((packed)) COMMAND_BLOCK_WRAPPER;

/*
 * Command Status Wrapper
 */
typedef struct{
        unsigned long   dCSWSignature;
        unsigned long   dCSWTag;
        unsigned long   dCSWDataResidue;
        u8   bCSWStatus;
} __attribute__((packed)) COMMAND_STATUS_WRAPPER;

/*
 * SCSI Command
 */

/*
 * INQUIRY
 */

typedef struct{
        u8   OperationCode;
        u8   EnableVPD:1,
             Reserved1:4,
             LogicalUnitNumber:3;
        u8   PageCode;
        u8   Reserved2;
        u8   AllocationLength;
        u8   Reserved3;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
        u8   Reserved8;
        u8   Reserved9;
} __attribute__((packed)) SCSI_INQUIRY_COMMAND;

typedef struct{
        u8   PeripheralDeviceType:5,
             PeripheralQaulifier:3;
        u8   Reserved2:7,
             RMB:1;
        u8   ANSIVersion; //:3,
//             ECMAVersion:3,
//             ISOVersion:2;
        u8   ResponseDataFormat:4,
             Reserved3:4;
        u8   AdditionalLength;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
        u8   VendorInformation[8];
        u8   ProductIdentification[16];
        u8   ProductRevisionLevel[4];
} __attribute__((packed)) SCSI_INQUIRY_DATA;

/*
 * READ FORMAT CAPACITY
 */

typedef struct{
        u8   OperationCode;
        u8   Reserved1:5,
             LogicalUnitNumber:3;
        u8   Reserved2;
        u8   Reserved3;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
        u16  AllocationLength;
        u8   Reserved7;
        u8   Reserved8;
        u8   Reserved9;
} __attribute__((packed)) SCSI_READ_FORMAT_CAPACITY_COMMAND;

typedef struct{
        struct{
                u8   Reserved1;
                u8   Reserved2;
                u8   Reserved3;
                u8   CapacityListLength;
        } __attribute__((packed)) CapacityListHeader;

        struct{
                u32  NumberofBlocks;
                u8   DescriptorCode:2,
                     Reserved1:6;
                u8   BlockLength[3];
        } __attribute__((packed)) CurrentMaximumCapacityDescriptor;

} __attribute__((packed)) SCSI_READ_FORMAT_CAPACITY_DATA;

/*
 * READ FORMAT CAPACITY
 */

typedef struct{
        u8   OperationCode;
        u8   RelAdr:1, 
             Reserved1:4, 
             LogicalUnitNumber:3;
        u32  LogicalBlockAddress;
        u8   Reserved2;
        u8   Reserved3;
        u8   PMI:1, 
             Reserved4:7;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
} __attribute__((packed)) SCSI_READ_CAPACITY_COMMAND;

typedef struct{
        u32   LastLogicalBlockAddress;
        u32   BlockLengthInBytes;
} __attribute__((packed)) SCSI_READ_CAPACITY_DATA;

/*
 * REQUEST SENSE
 */

typedef struct{
        u8   OperationCode;
        u8   Reserved1:5, 
             LogicalUnitNumber:3;
        u8   Reserved2;
        u8   Reserved3;
        u8   AllocationLength;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
        u8   Reserved8;
        u8   Reserved9;
        u8   Reserved10;
} __attribute__((packed)) SCSI_REQUEST_SENSE_COMMAND;

typedef struct{
        u8   ErrorCode:7, 
             Valid:1;
        u8   Reserved1;
        u8   SenseKey:4, 
             Reserved2:4;
        u32  Information;
        u8   AdditionalSenseLength;
        u8   Reserved3[4];
        u8   AdditionalSenseCode;
        u8   AdditionalSenseCodeQualifier;
        u8   Reserved4;
        u8   Reserved5[3];
} __attribute__((packed)) SCSI_REQUEST_SENSE_DATA;

/*
 * READ(10)
 */

typedef struct{
        u8   OperationCode;
        u8   RelAdr:1, 
             Reserved1:2, 
             FUA:1, 
             DPO:1, 
             LogicalUnitNumber:3;
        u32  LogicalBlockAddress;
        u8   Reserved2;
        u16  TransferLength;
        u8   Reserved3;
        u8   Reserved4;
        u8   Reserved5;
} __attribute__((packed)) SCSI_READ_10_COMMAND;



/*
 * MODE SENSE
 */
typedef struct{
        u8   OperationCode;
        u8   Reserved1:3, 
             DBD:1, 
             Reserved2:1, 
             LogicalUnitNumber:3; 
        u8   PageCode:6, 
             PageControl:2;
        u8   Reserved3;
        u8   AllocationLength;
        u8   Control;
} __attribute__((packed)) SCSI_MODE_SENSE_6_COMMAND;


typedef struct{
        u8   OperationCode;
        u8   Reserved1:3, 
             DBD:1, 
             Reserved2:1, 
             LogicalUnitNumber:3; 
        u8   PageCode:6, 
             PageControl:2;
        u8   Reserved3;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
        u16  AllocationLength;
        u8   Control;
} __attribute__((packed)) SCSI_MODE_SENSE_10_COMMAND;


/*  Mode sense command will return a structure looking like this:
       PARAMETER HEADER
       BLOCK DESCRIPTORS
       PAGES
    
       SCSI-2 8.3.3 Table 90
*/

typedef struct{
        u8   ModeDataLength;   /* Length in bytes of following data not including itself */
        u8   MediumTypeCode;
        u8   Reserved1:4,      /* This byte is the direct access device-specific parameters */
             DPOFUA:1,         /* set indicated device supports DPO and FUA bits */
             Reserved2:2,
             WriteProtect:1;   /* set indicates the device is write protected. */
        u8   BlockDescriptorLength;  /* Number of block descritpors multiplied by 8 */
} __attribute__((packed)) MODE_6_PARAMETER_HEADER;


typedef struct{
        u16  ModeDataLength;   /* Length in bytes of following data not including itself */   
        u8   MediumTypeCode;
        u8   Reserved1:4,      /* This byte is the direct access device-specific parameters */
             DPOFUA:1,         /* set indicated device supports DPO and FUA bits */
             Reserved2:2,
             WriteProtect:1;   /* set indicates the device is write protected. */
        u8   Reserved4;
        u8   Reserved5;
        u16  BlockDescriptorLength;
} __attribute__((packed)) MODE_10_PARAMETER_HEADER;



typedef struct{
        u8   PageCode:6,
             Reserved1:1,
             PS:1;             /* Set when target can save to a nonvolatile location */
        u8   PageLength;
        u8   DCR:1,            /* Disable Correction */
             DTE:1,            /* Disable Transfer on Error */
             PER:1,            /* Post Error Recovery: set indicates target reports recovered errors */
             EER:1,            /* Enable Early Recovery: set indicates exppedient error recovery first */
             RC:1,             /* Read Continous: set indicates full amount of data requested will be given, even if erroneous of fabricated */
             TB:1,             /* Transfer Block: set indiciates a unrecovered data block shall be transferred to initiator before CHECK CONDITION */
             ARRE:1,           /* Automatic Read Reallocation Enabled */
             AWRE:1;           /* Automatic Write Reallocation Enabled */
        u8   ReadRetryCount;   /* Number of times target will retry read */
        u8   CorrectionSpan;   /* Size (in bits) of largest data burst for error correction to be attempted.  0 indicates default/not supproted */
        u8   HeadOffsetCount;  /* Two-complement value indicating incremental offset position from track center to which the heads shall be moved */
        u8   DataStrobeOffsetCount; 
        u8   Reserved7;
        u8   WriteRetryCount;  /* Number of times target will retry write */
        u8   Reserved9;
        u16  RecoveryTimeLimit; /* Time (in ms) target will use for data recovery procedures */
} __attribute__((packed)) READ_WRITE_ERROR_RECOVERY_PAGE;

typedef struct{
        u8   PageCode:6,
             Reserved1:1,
             PS:1;
        u8   PageLength;
        u16  TransferRate;
        u8   NumberofHeads;
        u8   SectorsperTrack;
        u16  DataBytesperSector;
        u16  NumberofCylinders;
        u8   Reserved2[9];
        u8   MotorOnDelay;
        u8   MotorOffDelay;
        u8   Reserved3[7];
        u16  MediumRotationRate;
        u8   Reserved4;
        u8   Reserved5;
} __attribute__((packed)) FLEXIBLE_DISK_PAGE;

typedef struct{
        u8   PageCode:6,
             Reserved1:1,
             PS:1;
        u8   PageLength;
        u8   Reserved2:6,
             SRFP:1,
             SFLP:1;
        u8   TLUN:3,
             Reserved3:3,
             SML:1,
             NCD:1;
        u8   Reserved4[8];
} __attribute__((packed)) REMOVABLE_BLOCK_ACCESS_CAPABILITIES_PAGE;

typedef struct{
        u8   PageCode:6,
             Reserved1:1,
             PS:1;
        u8   PageLength;
        u8   Reserved2;
        u8   InactivityTimeMultiplier:4,
             Reserved3:4;
        u8   SWPP:1,
             DISP:1,
             Reserved4:6;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
} __attribute__((packed)) TIMER_AND_PROTECT_PAGE;

typedef struct{
        READ_WRITE_ERROR_RECOVERY_PAGE              ReadWriteErrorRecoveryPage;
        FLEXIBLE_DISK_PAGE                          FlexibleDiskPage;
        REMOVABLE_BLOCK_ACCESS_CAPABILITIES_PAGE    RemovableBlockAccessCapabilitiesPage;
        TIMER_AND_PROTECT_PAGE                      TimerAndProtectPage;
} __attribute__((packed)) MODE_ALL_PAGES;






/*
 * TEST UNIT READY
 */

typedef struct{
        u8   OperationCode;
        u8   Reserved1:5,
             LogicalUnitNumber:3;
        u8   Reserved2;
        u8   Reserved3;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
        u8   Reserved8;
        u8   Reserved9;
        u8   Reserved10;
        u8   Reserved11;
} __attribute__((packed)) SCSI_TEST_UNIT_READY_COMMAND;

/*
 * PREVENT-ALLOW MEDIA REMOVAL
 */

typedef struct{
        u8   OperationCode;
        u8   Reserved1:5,
             LogicalUnitNumber:3;
        u8   Reserved2;
        u8   Reserved3;
        u8   Prevent:2,
             Reserved4:6;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
        u8   Reserved8;
        u8   Reserved9;
        u8   Reserved10;
        u8   Reserved11;
} __attribute__((packed)) SCSI_PREVENT_ALLOW_MEDIA_REMOVAL_COMMAND;

/*
 * START-STOP UNIT
 */

typedef struct{
        u8   OperationCode;
        u8   IMMED:1,
             Reserved1:4,
             LogicalUnitNumber:3;
        u8   Reserved2;
        u8   Reserved3;
        u8   Start:1,
             LoEj:1,
             Reserved4:2,
             PowerConditions:4;
        u8   Reserved5;
        u8   Reserved6;
        u8   Reserved7;
        u8   Reserved8;
        u8   Reserved9;
        u8   Reserved10;
        u8   Reserved11;
} __attribute__((packed)) SCSI_START_STOP_COMMAND;


/*
 * WRITE(10)
 */

typedef struct{
        u8   OperationCode;
        u8   RelAdr:1,
             Reserved1:2,
             FUA:1,
             DPO:1,
             LogicalUnitNumber:3;
        u32  LogicalBlockAddress;
        u8   Reserved2;
        u16  TransferLength;
        u8   Reserved3;
        u8   Reserved4;
        u8   Reserved5;
} __attribute__((packed)) SCSI_WRITE_10_COMMAND;

/*
 * VERIFY
 */

typedef struct{
        u8   OperationCode;
        u8   RelAdr:1,
             ByteChk:1,
             Reserved1:1,
             Reserved2:1,
             DPO:1,
             LogicalUnitNumber:3;
        u32  LogicalBlockAddress;
        u8   Reserved3;
        u16  VerificationLength;
        u8   Reserved4;
        u8   Reserved5;
        u8   Reserved6;
} __attribute__((packed)) SCSI_VERIFY_COMMAND;

#endif  /* _MSCSCSIPROTO_H_ */

