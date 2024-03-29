/*
 * Raspberry PI Remote Serial Protocol.
 * Copyright 2012 Jamie Iles, jamie@jamieiles.com.
 * Licensed under GPLv2.
 */
#include "assembly.h"

.arm

#ifdef JAILHOUSE
// Jailhouse loader program IRQ handler decrements LR, so we don't need to fix it up here.
VECTOR(jhirq)
	ex_handler	#0, gdbstub_io_handler, #CPSR_MODE_SVC
// SVC insn points LR at the next insn (i.e. the one after our breakpoint), so we need to fix it up here.
VECTOR(jhsvc)
	ex_handler	#4, gdbstub_smc_handler, #CPSR_MODE_SVC
#else

.pushsection .text.tzvectors
.align	12
_monitor_vecs:
	b	mon_panic		@ System reset
	b	mon_panic		@ Undefined instruction
	b	_vec_smc_tz		@ Software interrupt
	b	mon_panic		@ Prefetch abort
	b	mon_panic		@ Data abort
	b	mon_panic		@ Reserved vector
	b	mon_panic		@ IRQ
	b	_vec_fiq_tz		@ FIQ
.popsection

VECTOR(smc_tz)
	ex_handler	#4, gdbstub_smc_handler, #CPSR_MODE_MON
VECTOR(fiq_tz)
	ex_handler	#4, gdbstub_io_handler, #CPSR_MODE_MON

/*
 * Setup monitor mode.  Must be called from supervisor mode.
 *
 * r0: stack for monitor mode.
 * clobbers: r0
 */
.globl init_monitor
init_monitor:
	cps	#CPSR_MODE_MON
	mov	sp, r0
	cps	#CPSR_MODE_SVC
	ldr	r0, monitor_vec_addr
	mcr	p15, 0, r0, c12, c0, 1
	mov	r0, #4
	mcr	p15, 0, r0, c1, c1, 0 @ FIQ's go to the monitor mode.
	bx	lr

monitor_vec_addr:
	.word	_monitor_vecs
#endif
