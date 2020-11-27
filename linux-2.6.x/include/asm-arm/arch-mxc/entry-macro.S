	@ this macro disables fast irq (not implemented)
	.macro	disable_fiq
	.endm

	@ this macro checks which interrupt occured
	@ and returns its number in irqnr
	@ and returns if an interrupt occured in irqstat
	.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp
	@ Can't use IO_ADDRESS macro. Otherwise compilation error
	@ BEGIN: WFI issue Workaround
	ldr	\irqstat, =AVIC_IO_ADDRESS(AVIC_BASE_ADDR)
	ldr	\tmp, [\irqstat, #AVIC_VEC_0]
	cmp	\tmp, #12
	moveq   \tmp, #8
	str     \tmp, [\irqstat, #AVIC_VEC_0]
	@ END: WFIissue workaround
	@ Load offset & priority of the highest priority
	@ interrupt pending.
	ldr	\irqnr, [\irqstat, #0x40]	@ this is AVIC_NIVECSR
	@ Shift off the priority leaving the offset or
	@ "interrupt number"
	mov	\irqnr, \irqnr, lsr #16
	ldr	\irqstat, =1	@ dummy compare
	ldr	\base, =0xFFFF	// invalid interrupt
	cmp	\irqnr, \base
	bne	1001f
	ldr	\irqstat, =0
	1001:
	tst	\irqstat, #1	@ to make the condition code = TRUE
	.endm

	@ irq priority table (not used)
	.macro	irq_prio_table
	.endm
