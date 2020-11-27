/*
 * include/asm-arm/arch-mxc/arc_otg.h
 *
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write  to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

struct arc_usb_config {
	char *name;		/* pretty print */
	int (*platform_init) (void);	/* platform-specific init routine */
	void (*platform_uninit) (void);	/* platform-specific uninit routine */
	u32 xcvr_type;		/* PORTSC_PTS_* */
	u32 usbmode;		/* address of usbmode register */
};

#define USB_OTGREGS_BASE	(OTG_BASE_ADDR + 0x000)
#define USB_H1REGS_BASE		(OTG_BASE_ADDR + 0x200)
#define USB_H2REGS_BASE		(OTG_BASE_ADDR + 0x400)
#define USB_OTHERREGS_BASE	(OTG_BASE_ADDR + 0x600)

#define USBOTG_REG32(offset)	(*((volatile u32 *)(IO_ADDRESS(USB_OTGREGS_BASE + (offset)))))
#define USBOTG_REG16(offset)	(*((volatile u16 *)(IO_ADDRESS(USB_OTGREGS_BASE + (offset)))))

#define USBH1_REG32(offset)	(*((volatile u32 *)(IO_ADDRESS(USB_H1REGS_BASE + (offset)))))
#define USBH1_REG16(offset)	(*((volatile u16 *)(IO_ADDRESS(USB_H1REGS_BASE + (offset)))))

#define USBH2_REG32(offset)	(*((volatile u32 *)(IO_ADDRESS(USB_H2REGS_BASE + (offset)))))
#define USBH2_REG16(offset)	(*((volatile u16 *)(IO_ADDRESS(USB_H2REGS_BASE + (offset)))))

#define USBOTHER_REG(offset)	(*((volatile u32 *)(IO_ADDRESS(USB_OTHERREGS_BASE + (offset)))))

/*
 * OTG registers
 */
#define UOG_ID			USBOTG_REG32(0x00)	/* Host ID */
#define UOG_HWGENERAL		USBOTG_REG32(0x04)	/* Host General */
#define UOG_HWHOST		USBOTG_REG32(0x08)	/* Host h/w params */
#define UOG_HWTXBUF		USBOTG_REG32(0x10)	/* TX buffer h/w params */
#define UOG_HWRXBUF		USBOTG_REG32(0x14)	/* RX buffer h/w params */
#define UOG_CAPLENGTH		USBOTG_REG16(0x100)	/* Capability register length */
#define UOG_HCIVERSION		USBOTG_REG16(0x102)	/* Host Interface version */
#define UOG_HCSPARAMS		USBOTG_REG32(0x104)	/* Host control structural params */
#define UOG_HCCPARAMS		USBOTG_REG32(0x108)	/* control capability params */
#define UOG_DCIVERSION		USBOTG_REG32(0x120)	/* device interface version */
/* start EHCI registers: */
#define UOG_USBCMD		USBOTG_REG32(0x140)	/* USB command register */
#define UOG_USBSTS		USBOTG_REG32(0x144)	/* USB status register */
#define UOG_USBINTR		USBOTG_REG32(0x148)	/* interrupt enable register */
#define UOG_FRINDEX		USBOTG_REG32(0x14c)	/* USB frame index */
/*      segment                             (0x150)	   addr bits 63:32 if needed */
#define UOG_PERIODICLISTBASE	USBOTG_REG32(0x154)	/* host crtlr frame list base addr */
#define UOG_DEVICEADDR		USBOTG_REG32(0x154)	/* device crtlr device address */
#define UOG_ASYNCLISTADDR	USBOTG_REG32(0x158)	/* host ctrlr next async addr */
#define UOG_EPLISTADDR		USBOTG_REG32(0x158)	/* device ctrlr endpoint list addr */
#define UOG_BURSTSIZE		USBOTG_REG32(0x160)	/* host ctrlr embedded TT async buf status */
#define UOG_TXFILLTUNING	USBOTG_REG32(0x164)	/* TX FIFO fill tuning */
#define UOG_ULPIVIEW		USBOTG_REG32(0x170)	/* ULPI viewport (H2 only) */
#define	UOG_CFGFLAG		USBOTG_REG32(0x180)	/* configflag (supports HS) */
#define UOG_PORTSC1		USBOTG_REG32(0x184)	/* port status and control */
/* end EHCI registers: */
#define UOG_OTGSC		USBOTG_REG32(0x1a4)	/* OTG status and control */
#define UOG_USBMODE		USBOTG_REG32(0x1a8)	/* USB device mode */
#define UOG_ENDPTSETUPSTAT	USBOTG_REG32(0x1ac)	/* endpoint setup status */
#define UOG_ENDPTPRIME		USBOTG_REG32(0x1b0)	/* endpoint initialization */
#define UOG_ENDPTFLUSH		USBOTG_REG32(0x1b4)	/* endpoint de-initialize */
#define UOG_ENDPTSTAT		USBOTG_REG32(0x1b8)	/* endpoint status */
#define UOG_ENDPTCOMPLETE	USBOTG_REG32(0x1bc)	/* endpoint complete */
#define UOG_EPCTRL0		USBOTG_REG32(0x1c0)	/* endpoint control0 */
#define UOG_EPCTRL1		USBOTG_REG32(0x1c4)	/* endpoint control1 */
#define UOG_EPCTRL2		USBOTG_REG32(0x1c8)	/* endpoint control2 */
#define UOG_EPCTRL3		USBOTG_REG32(0x1cc)	/* endpoint control3 */
#define UOG_EPCTRL4		USBOTG_REG32(0x1d0)	/* endpoint control4 */
#define UOG_EPCTRL5		USBOTG_REG32(0x1d4)	/* endpoint control5 */
#define UOG_EPCTRL6		USBOTG_REG32(0x1d8)	/* endpoint control6 */
#define UOG_EPCTRL7		USBOTG_REG32(0x1dc)	/* endpoint control7 */

/*
 * Host 1 registers
 */
#define UH1_ID			USBH1_REG32(0x00)	/* Host ID */
#define UH1_HWGENERAL		USBH1_REG32(0x04)	/* Host General */
#define UH1_HWHOST		USBH1_REG32(0x08)	/* Host h/w params */
#define UH1_HWTXBUF		USBH1_REG32(0x10)	/* TX buffer h/w params */
#define UH1_HWRXBUF		USBH1_REG32(0x14)	/* RX buffer h/w params */
#define UH1_CAPLENGTH		USBH1_REG16(0x100)	/* Capability register length */
#define UH1_HCIVERSION		USBH1_REG16(0x102)	/* Host Interface version */
#define UH1_HCSPARAMS		USBH1_REG32(0x104)	/* Host control structural params */
#define UH1_HCCPARAMS		USBH1_REG32(0x108)	/* control capability params */
/* start EHCI registers: */
#define UH1_USBCMD		USBH1_REG32(0x140)	/* USB command register */
#define UH1_USBSTS		USBH1_REG32(0x144)	/* USB status register */
#define UH1_USBINTR		USBH1_REG32(0x148)	/* interrupt enable register */
#define UH1_FRINDEX		USBH1_REG32(0x14c)	/* USB frame index */
/*      segment                            (0x150)	   addr bits 63:32 if needed */
#define UH1_PERIODICLISTBASE	USBH1_REG32(0x154)	/* host crtlr frame list base addr */
#define UH1_ASYNCLISTADDR	USBH1_REG32(0x158)	/* host ctrlr nest async addr */
#define UH1_BURSTSIZE		USBH1_REG32(0x160)	/* host ctrlr embedded TT async buf status */
#define UH1_TXFILLTUNING	USBH1_REG32(0x164)	/* TX FIFO fill tuning */
/*      configured_flag                    (0x180)	   configflag (supports HS) */
#define UH1_PORTSC1		USBH1_REG32(0x184)	/* port status and control */
/* end EHCI registers: */
#define UH1_USBMODE		USBH1_REG32(0x1a8)	/* USB device mode */

/*
 * Host 2 registers
 */
#define UH2_ID			USBH2_REG32(0x00)	/* Host ID */
#define UH2_HWGENERAL		USBH2_REG32(0x04)	/* Host General */
#define UH2_HWHOST		USBH2_REG32(0x08)	/* Host h/w params */
#define UH2_HWTXBUF		USBH2_REG32(0x10)	/* TX buffer h/w params */
#define UH2_HWRXBUF		USBH2_REG32(0x14)	/* RX buffer h/w params */
#define UH2_CAPLENGTH		USBH2_REG16(0x100)	/* Capability register length */
#define UH2_HCIVERSION		USBH2_REG16(0x102)	/* Host Interface version */
#define UH2_HCSPARAMS		USBH2_REG32(0x104)	/* Host control structural params */
#define UH2_HCCPARAMS		USBH2_REG32(0x108)	/* control capability params */
/* start EHCI registers: */
#define UH2_USBCMD		USBH2_REG32(0x140)	/* USB command register */
#define UH2_USBSTS		USBH2_REG32(0x144)	/* USB status register */
#define UH2_USBINTR		USBH2_REG32(0x148)	/* interrupt enable register */
#define UH2_FRINDEX		USBH2_REG32(0x14c)	/* USB frame index */
/*      segment                            (0x150)	   addr bits 63:32 if needed */
#define UH2_PERIODICLISTBASE	USBH2_REG32(0x154)	/* host crtlr frame list base addr */
#define UH2_ASYNCLISTADDR	USBH2_REG32(0x158)	/* host ctrlr nest async addr */
#define UH2_BURSTSIZE		USBH2_REG32(0x160)	/* host ctrlr embedded TT async buf status */
#define UH2_TXFILLTUNING	USBH2_REG32(0x164)	/* TX FIFO fill tuning */
#define UH2_ULPIVIEW		USBH2_REG32(0x170)	/* ULPI viewport (H2 only) */
/*      configured_flag                    (0x180)	   configflag (supports HS) */
#define UH2_PORTSC1		USBH2_REG32(0x184)	/* port status and control */
/* end EHCI registers */
#define UH2_USBMODE		USBH2_REG32(0x1a8)	/* USB device mode */

/*
 * other regs (not part of ARC core)
 */
#define USBCTRL			USBOTHER_REG(0x00)	/* USB Control register */
#define USB_OTG_MIRROR		USBOTHER_REG(0x04)	/* USB OTG mirror register */

/*
 * register bits
 */

/* x_PORTSCx */
#define PORTSC_PTS_MASK		(3 << 30)	/* parallel xcvr select mask */
#define PORTSC_PTS_UTMI		(0 << 30)	/* UTMI/UTMI+ */
#define PORTSC_PTS_PHILIPS	(1 << 30)	/* Philips classic */
#define PORTSC_PTS_ULPI		(2 << 30)	/* ULPI */
#define PORTSC_PTS_SERIAL	(3 << 30)	/* serial */
#define PORTSC_STS		(1 << 29)	/* serial xcvr select */
#define PORTSC_LS_MASK		(3 << 10)	/* Line State mask */
#define PORTSC_LS_SE0		(0 << 10)	/* SE0     */
#define PORTSC_LS_K_STATE	(1 << 10)	/* K-state */
#define PORTSC_LS_J_STATE	(2 << 10)	/* J-state */

/* x_USBMODE */
#define USBMODE_SDIS		(1 << 4)	/* stream disable mode */
#define USBMODE_SLOM		(1 << 3)	/* setup lockout mode */
#define USBMODE_ES		(1 << 2)	/* (big) endian select */
#define USBMODE_CM_MASK		(3 << 0)	/* controller mode mask */
#define USBMODE_CM_HOST		(3 << 0)	/* host */
#define USBMODE_CM_DEVICE	(2 << 0)	/* device */
#define USBMODE_CM_reserved	(1 << 0)	/* reserved */
#define USBMODE_CM_IDLE		(0 << 0)	/* idle */

/* USBCTRL */
#define UCTRL_OWIR		(1 << 31)	/* OTG wakeup intr request received */
#define UCTRL_OSIC_MASK		(3 << 29)	/* OTG  Serial Interface Config: */
#define UCTRL_OSIC_DU6		(0 << 29)	/* Differential/unidirectional 6 wire */
#define UCTRL_OSIC_DB4		(1 << 29)	/* Differential/bidirectional  4 wire */
#define UCTRL_OSIC_SU6		(2 << 29)	/* single-ended/unidirectional 6 wire */
#define UCTRL_OSIC_SB3		(3 << 29)	/* single-ended/bidirectional  3 wire */

#define UCTRL_OUIE		(1 << 28)	/* OTG ULPI intr enable */
#define UCTRL_OWIE		(1 << 27)	/* OTG wakeup intr enable */
#define UCTRL_OBPVAL_RXDP	(1 << 26)	/* OTG RxDp status in bypass mode */
#define UCTRL_OBPVAL_RXDM	(1 << 25)	/* OTG RxDm status in bypass mode */
#define UCTRL_OPM		(1 << 24)	/* OTG power mask */
#define UCTRL_H2WIR		(1 << 23)	/* HOST2 wakeup intr request received */
#define UCTRL_H2SIC_MASK	(3 << 21)	/* HOST2 Serial Interface Config: */
#define UCTRL_H2SIC_DU6		(0 << 21)	/* Differential/unidirectional 6 wire */
#define UCTRL_H2SIC_DB4		(1 << 21)	/* Differential/bidirectional  4 wire */
#define UCTRL_H2SIC_SU6		(2 << 21)	/* single-ended/unidirectional 6 wire */
#define UCTRL_H2SIC_SB3		(3 << 21)	/* single-ended/bidirectional  3 wire */

#define UCTRL_H2UIE		(1 << 20)	/* HOST2 ULPI intr enable */
#define UCTRL_H2WIE		(1 << 19)	/* HOST2 wakeup intr enable */
#define UCTRL_H2PM		(1 << 16)	/* HOST2 power mask */

#define UCTRL_H1WIR		(1 << 15)	/* HOST1 wakeup intr request received */
#define UCTRL_H1SIC_MASK	(3 << 13)	/* HOST1 Serial Interface Config: */
#define UCTRL_H1SIC_DU6		(0 << 13)	/* Differential/unidirectional 6 wire */
#define UCTRL_H1SIC_DB4		(1 << 13)	/* Differential/bidirectional  4 wire */
#define UCTRL_H1SIC_SU6		(2 << 13)	/* single-ended/unidirectional 6 wire */
#define UCTRL_H1SIC_SB3		(3 << 13)	/* single-ended/bidirectional  3 wire */

#define UCTRL_H1WIE		(1 << 11)	/* HOST1 wakeup intr enable */
#define UCTRL_H1BPVAL_RXDP	(1 << 10)	/* HOST1 RxDp status in bypass mode */
#define UCTRL_H1BPVAL_RXDM	(1 <<  9)	/* HOST1 RxDm status in bypass mode */
#define UCTRL_H1PM		(1 <<  8)	/* HOST1 power mask */

#define UCTRL_H2DT		(1 <<  5)	/* HOST2 TLL disabled */
#define UCTRL_H1DT		(1 <<  4)	/* HOST1 TLL disabled */
#define UCTRL_BPE		(1 <<  0)	/* bypass mode enable */

/* USBCMD */
#define UCMD_RESET		(1 << 1)	/* controller reset */

/* OTG_MIRROR */
#define OTGM_SESEND		(1 << 4)	/* B device session end */
#define OTGM_VBUSVAL		(1 << 3)	/* Vbus valid */
#define OTGM_BSESVLD		(1 << 2)	/* B session Valid */
#define OTGM_ASESVLD		(1 << 1)	/* A session Valid */
#define OTGM_IDIDG		(1 << 0)	/* OTG ID pin status */
						/* 1=high: Operate as B-device */
						/* 0=low : Operate as A-device */
