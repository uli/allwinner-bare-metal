.global _reset
_reset:
cpsid if

MRC p15,0,r0,c1,c0,2    // Read CP Access register
ORR r0,r0,#0x00f00000   // Enable full access to NEON/VFP (Coprocessors 10 and 11)
MCR p15,0,r0,c1,c0,2    // Write CP Access register
ISB
MOV r0,#0x40000000      // Switch on the VFP and NEON hardware
VMSR FPEXC,r0            // Set EN bit in FPEXC

MRC p15, 0, r0, c0, c0, 5
and r0, r0, #3
#ifdef JAILHOUSE
CMP r0, #2
#else
CMP r0, #0
#endif
BNE start_secondary

#ifdef JAILHOUSE
mov sp, #0xf000
#else
mov sp, #0xc000
#endif

bl init_bss
movw r0, #0x4541
movt r0, #0xc0de
ldr r1, 1f
str r0, [r1]

b startup

1: .word __stack_chk_guard

.globl _ivt
_ivt:
ldr pc, _reset_h
ldr pc, _undefined_instruction_vector_h
ldr pc, _software_interrupt_vector_h
ldr pc, _prefetch_abort_vector_h
ldr pc, _data_abort_vector_h
ldr pc, _unused_handler_h
ldr pc, _interrupt_vector_h
ldr pc, _fast_interrupt_vector_h
_reset_h:                           .word   _reset
_undefined_instruction_vector_h:    .word   except_undef
_software_interrupt_vector_h:       .word   except_swi
_prefetch_abort_vector_h:           .word   except_prefetch_abort
_data_abort_vector_h:               .word   except_data_abort
_unused_handler_h:                  .word   _reset
_interrupt_vector_h:                .word   interrupt
_fast_interrupt_vector_h:           .word   except_fiq

// initialize IRQ stack pointer
.globl init_sp_irq
init_sp_irq:
	mov	   r2, r0
	mrs        r0, CPSR
	bic        r1, r0, #31
	orr        r1, r1, #18
	msr        CPSR_c, r1
	mov	   sp, r2
	msr        CPSR_c, r0
	bx         lr

.globl _smp_secondary_sp
_smp_secondary_sp: .word 0

start_secondary:
	ldr sp, _smp_secondary_sp
	b _smp_startup_secondary

.globl smp_get_core_id
smp_get_core_id:
	MRC p15, 0, r0, c0, c0, 5
	and r0, r0, #3
	bx lr

