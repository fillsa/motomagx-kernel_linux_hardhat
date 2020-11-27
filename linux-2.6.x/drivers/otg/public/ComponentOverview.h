/*  Version 1.1
*   Component Programming Interface Overview Header Template for Doxygen
*   Copyright © 2006 Motorola, Inc.  All Rights Reserved.
*/

/** 
\mainpage USB DRIVER API 
 
\if MOTOROLA_CONFIDENTIAL_PROPRIETARY 

    \author  Sachin T 
    
\endif

\section References 1.0 References

\section Overview 2.0 Overview
         This document describes the device nodes, device names and the IOCTLS supported by them.

\section IndustryStandard 3.0 Industry Standard
         The USB driver code does not follow any industry standard. 

\section ExternalApiDocuments 4.0 External API Documents
         "None"

\section ClassAndStructureDefinitions 5.0 C++ Class and Structure Definitions
         "None"

\section CFunctionsAndDataTypes 6.0 "C" Functions and Data Types

         \subsection CFunctions 6.1 "C" Functions
         "None" 

         \subsection DataStructures 6.2 Data Structures
         "None"

         Details of the structures are available from the source header files directly:
	 "None"

\section HeaderFilesAndLibraries 7.0 Header Files And Libraries
         -# otg-node.h 


\section PluginExtensionPointInterface 8.0 Plug-in Extension Point Interface
         "None". 

\section OtherInterfaces 9.0 Other Interfaces
         -  /proc/msc_io                   mass storage proc interface. The following are the IOCTL commands supported \n
                                           by the mass storage device. 
            -  \b OTGMSC_START  [Command Number: 'M'+1]    \e struct \e msc_info \n
                                           This IOCTL mounts the mass storage device.  
            -  \b OTGMSC_STATUS [Command Number: 'M'+2]            \e   struct \e msc_info \n
                                           This IOCTL returns the current mount status of the device 
            -  \b OTGMSC_STOP   [Command Number: 'M'+1]            \e   struct \e  msc_info \n
                                           This IOCTL unmounts the mass storage device.  
            -  \b OTGMSC_WAIT_CONNECT  [Command Number: 'M'+3]     \e   struct \e msc_info \n
                                           This IOCTL will wait for the device to be connected if it not already connected. This is a synchrous call \n 
                                           which needs to be woken and informed of the error if the drivers are unloaded.  
            -  \b OTGMSC_WAIT_DISCONNECT  [Command Number: 'M'+4]  \e struct \e msc_info \n
                                           This IOCTL will wait for the device to be disconnect if is not already disconnected. This is a synchrous call \n 
                                           which needs to be woken and informed of the error if the drivers are unloaded.
            -  \b OTGMSC_GET_UNKNOWN_CMD   [Command Number: 'M'+3] \e struct \e COMMAND_BLOCK_WRAPPER \n
                                           This IOCTL is used to get unknown commands which the user application has requested.
         -  /dev/mtp                       mtp device name. The following are the IOCTL commands supported \n
            -  \b MTP_IOC_EVENT [Command Number: 'm'+1]            \e struct \e mtp_event_data \n
                                           This IOCTL is used to send a event from the app to the host.
            -  \b MTP_IOC_SEND_ZLP [Command Number: 'm'+2]        \e void \n
                                           This IOCTL is used to send a zero length packet  
            -  \b MTP_IOC_GET_EP_SIZE_IN  [Command Number:'m'+3]  \e int \n
                                           This IOCTL is used to get the size of the endpoint buffer size for bulk in 
            -  \b MTP_IOC_GET_VENDOR_FLAG  [Command Number: 'm'+4] \e int \n
                                           This IOCTL is used to get the flag which tell whether vendor request has been serviced  
            -  \b MTP_IOC_CANCEL_IO  [Command Number: 'm'+5]      \e void \n
                                           This IOCTL is used to cancel the IO 
            -  \b MTP_IOC_DEVICE_RESET  [Command Number: 'm'+6]   \e void \n
                                           This IOCTL is used to reset the device  
         -  /proc/mtpctl                   mtp control proc interface.
         -  /dev/acm/0                     acm device name. The following are the IOCTL commands supported by acm device. \n
                                           For more details, please refer man pages.  
            -  \b TCSETS                   This IOCTL is used to set the serial port settings.
            -  \b TCFLSH                   This IOCTL discards data written to the object but not transmitted, or data received but not read, \n 
                                           depending on the value passed. 
            -  \b TCGETS                   This IOCTL is  used to get the serial port settings. 
         -  /usbl0                         network device name.
         -  /proc/network_ip               network proc interface 
         -  /proc/otg_message              otg-message proc interface. The following are the IOCTL commands supported.
            -  \b OTGADMIN_STATUS [Command Number: 'O'+3]         \e struct \e otg_admin_status_update \n
                                           This IOCTL return the USB state machine status.
            -  \b OTGADMIN_SET_FUNCTION  [Command Number: 'O'+4]    \e struct \e otg_admin_command \n
                                           This IOCTL sets OTG to the function name.
            -  \b OTGADMIN_GET_FUNCTION  [Command Number: 'O'+4]    \e struct \e otg_admin_command \n
                                           This IOCTL gets OTG function.
            -  \b OTGADMIN_ENABLE     [Command Number: 'O'+32]       \e int \n
                                           This IOCTL enables or disables the USB state machine. 
         -  /proc/usb-functions            usb-functions proc interface 


\section OtherNotes 10.0 Other Notes

\warning The following APIs are in the "Draft" state.\n
	 Disclaimer:\n 
	 APIs that are in the "Draft" state are subject to evolve with new releases. 
	 Backward compatibility is not guaranteed until moved to the "Final" state. 
	 Developers should use APIs in the "draft" state at their own risk.\n
*/
