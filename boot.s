_reset:
cpsid if

MRC p15,0,r0,c1,c0,2    // Read CP Access register
ORR r0,r0,#0x00f00000   // Enable full access to NEON/VFP (Coprocessors 10 and 11)
MCR p15,0,r0,c1,c0,2    // Write CP Access register
ISB
MOV r0,#0x40000000      // Switch on the VFP and NEON hardware
VMSR FPEXC,r0            // Set EN bit in FPEXC

mov sp, #0x4000
b startup

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
