/*
 * otg/otg/usbp-audio.h
 * @(#) balden@seth2.belcarratech.com|otg/otg/usbp-audio.h|20051116205002|10296
 *
 *      Copyright (c) 2004-2005 Belcarra
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
/*
 * @file otg/otg/usbp-audio.h
 * @brief Audio Class class descriptor structure definitions.
 *
 *
 * @addtogroup USBP
 */

/*!
 * @name Audio
 */
 /*! @{ */

#define CS_AUDIO_UNDEFINED              0x20
#define CS_AUDIO_DEVICE                 0x21
#define CS_AUDIO_CONFIGURATION          0x22
#define CS_AUDIO_STRING                 0x23
#define CS_AUDIO_INTERFACE              0x24
#define CS_AUDIO_ENDPOINT               0x25

#define AUDIO_HEADER                    0x01
#define AUDIO_INPUT_TERMINAL            0x02
#define AUDIO_OUTPUT_TERMINAL           0x03
#define AUDIO_MIXER_UNIT                0x04
#define AUDIO_SELECTOR_UNIT             0x05
#define AUDIO_FEATURE_UNIT              0x06
#define AUDIO_PROCESSING_UNIT           0x07
#define AUDIO_EXTENSION_UNIT            0x08
/*! @} */

#ifdef PRAGMAPACK
#pragma pack(push,1)
#endif


/*! @name c.f. CDC 5.2.3 Table 24
 * c.f. Audio Appendix A.4
 */
 /*! @{ */
#define CS_UNDEFINED                    0x20
#define CS_DEVICE                       0x21
#define CS_CONFIG                       0x22
#define CS_STRING                       0x23
#define CS_INTERFACE                    0x24
#define CS_ENDPOINT                     0x25
/*! @} */

/*!
 * @name Audio Interface Class - c.f Appendix A.1
 */
 /*! @{ */
#define AUDIO_INTERFACE_CLASS           0x01
/*! @} */

/*! @name Audio Interface Subclass - c.f Appendix A.2
 */
 /*! @{ */
#define AUDIO_SUBCLASS_UNDEFINED        0x00
#define AUDIO_AUDIOCONTROL              0x01
#define AUDIO_AUDIOSTREAMING            0x02
#define AUDIO_MIDISTREAMING             0x03
/*! @} */

/*! @name Audio Interface Proctol - c.f. Appendix A.3
 */
 /*! @{ */
#define AUDIO_PR_PROTOCOL_UNDEFINED     0x00
/*! @} */

/*! @name Audio Class-Specific AC Interface Descriptor Subtypes - c.f. A.5
 */
 /*! @{ */
#define AUDIO_AC_DESCRIPTOR_UNDEFINED   0x00
#define AUDIO_AC_HEADER                 0x01
#define AUDIO_AC_INPUT_TERMINAL         0x02
#define AUDIO_AC_OUTPUT_TERMINAL        0x03
#define AUDIO_AC_MIXER_UNIT             0x04
#define AUDIO_AC_SELECTOR_UNIT          0x05
#define AUDIO_AC_FEATURE_UNIT           0x06
#define AUDIO_AC_PROCESSING_UNIT        0x07
#define AUDIO_AC_EXTENSION_UNIT         0x08
/*! @} */

/*! @name Audio Class-Specific AS Interface Descriptor Subtypes - c.f. A.6
 */
 /*! @{ */
#define AUDIO_AS_DESCRIPTOR_UNDEFINED   0x00
#define AUDIO_AS_GENERAL                0x01
#define AUDIO_AS_FORMAT_TYPE            0x02
#define AUDIO_AS_FORMAT_UNSPECFIC       0x03
/*! @} */

/*! @name Audio Class Processing Unit Processing Types - c.f. A.7
 */
 /*! @{ */
#define AUDIO_PROCESS_UNDEFINED         0x00
#define AUDIO_UP_DOWN_MUIX_PROCESS      0x01
#define AUDIO_DOLBY_PROLOGIC_PROCESS    0x02
#define AUDIO_3D_STEREO_EXTENDER_PROCESS 0x03
#define AUDIO_REVERBERATION_PROCESS     0x04
#define AUDIO_CHORUS_PROCESS            0x05
#define AUDIO_DYN_RANGE_COMP_PROCESS    0x06
/*! @} */

/*! @name Audio Class-Specific Endpoint Descriptor Subtypes - c.f. A.8
 */
 /*! @{ */
#define AUDIO_DESCRIPTOR_UNDEFINED      0x00
#define AUDIO_EP_GENERAL                0x01
/*! @} */

/*! @name Audio Class-Specific Request Codes - c.f. A.9
 */
 /*! @{ */
#define AUDIO_REQUEST_CODE_UNDEFINED    0x00
#define AUDIO_SET_CUR                   0x01
#define AUDIO_GET_CUR                   0x81
#define AUDIO_SET_MIN                   0x02
#define AUDIO_GET_MIN                   0x82
#define AUDIO_SET_MAX                   0x03
#define AUDIO_GET_MAX                   0x83
#define AUDIO_SET_RES                   0x04
#define AUDIO_GET_RES                   0x84
#define AUDIO_SET_MEM                   0x05
#define AUDIO_GET_MEM                   0x85
#define AUDIO_GET_STAT                  0xff
/*! @} */

/*!
 * @name Audio Status Word Format - c.f. Table 3.1
 */
 /*! @{ */

#define AUDIO_STATUS_INTERRUPT_PENDING                          1<<7
#define AUDIO_STATUS_MEMORY_CONTENT_CHANGED                     1<<6
#define AUDIO_STATUS_ORIGINATOR_AUDIO_CONTROL_INTERFACE         0
#define AUDIO_STATUS_ORIGINATOR_AUDIO_STREAMING_INTERFACE       1
#define AUDIO_STATUS_ORIGINATOR_AUDIO_STREAMING_ENDPOINT        2
#define AUDIO_STATUS_ORIGINATOR_AUDIOCONTROL_INTERFACE          0

struct usbd_audio_status_word {
        u8 bStatusType;
        u8 bOriginator;
};

struct usbd_audio_ac_interface_header_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u16 bcdADC;
        u16 wTotalLength;
        u8 binCollection;
        u8 bainterfaceNr[1];
};

struct usbd_audio_input_terminal_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bTerminalID;
        u16 wTerminalType;
        u8 bAssocTerminal;
        u8 bNrChannels;
        u16 wChannelConfig;
        u8 iChannelNames;
        u8 iTerminal;
};

struct usbd_audio_input_terminal_description {
        struct usbd_audio_input_terminal_descriptor *audio_input_terminal_descriptor;
        u16 wTerminalType;
        u16 wChannelConfig;
        char * iChannelNames;
        char * iTerminal;
};

struct usbd_audio_output_terminal_descriptor {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bTerminalID;
        u16 wTerminalType;
        u8 bAssocTerminal;
        u8 bSourceID;
        u8 iTerminal;
};

struct usbd_audio_output_terminal_description {
        struct usbd_audio_output_terminal_descriptor *audio_output_terminal_descriptor;
        u16 wTerminalType;
        char * iTerminal;
};

struct usbd_audio_mixer_unit_descriptor_a {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bUniteID;
        u8 bNrinPins;
};

struct usbd_audio_mixer_unit_descriptor_b {
        u8 baSourceID;
        u8 bNrChannels;
        u16 wChannelConfig;
        u8 iChannelNames;
        u8 bmControls;
        u8 iMixer;
};

struct usbd_audio_mixer_unit_description {
        struct usbd_audio_mixer_unit_descriptor *audio_mixer_unit_descriptor;
        u16 wChannelConfig;
        char * iMixer;
};


struct usbd_audio_as_general_interface_descriptor {
        u8 bLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bTerminalLink;
        u8 bDelay;
        u16 wFormatTag;
};

struct usbd_audio_as_general_interface_description {
        struct usbd_audio_as_general_interface_descriptor *audio_as_general_interface_descriptor;
        u16 wFormatTag;
};

struct usbd_audio_as_format_type_descriptor {
        u8 bLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bFormatType;
        u8 bNrChannels;
        u8 bSubFrameSize;
        u8 bBitResolution;
        u8 bSamFreqType;
        u8 iSamFreq[3];
};

struct usbd_audio_as_format_type_description {
        struct usbd_audio_as_format_type_descriptor *audio_as_format_type_descriptor;
        u16 wFormatTag;
};




struct usbd_audio_descriptor {
        union {
                struct usbd_audio_ac_interface_header_descriptor header;
                struct usbd_audio_input_terminal_descriptor input;
                struct usbd_audio_output_terminal_descriptor output;
        } descriptor;
};



struct usbd_ac_endpoint_descriptor {
        u8 bLength;
        u8 bDescriptorType;
        u8 bEndpointAddress;
        u8 bmAttributes;
        u16 wMaxPacketSize;
        u8 bInterval;
        u8 bRefresh;
        u8 bSynchAddress;
};


struct usbd_as_iso_endpoint_descriptor {
        u8 bLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype;
        u8 bmAttributes;
        u8 bLockDelayUnits;
        u16 wLockDelay;
};
/*! @}*/


#ifdef PRAGMAPACK
#pragma pack(pop)
#endif


