// ARMv7-A D-cache flush
.global mmu_flush_dcache
mmu_flush_dcache:
	push	{r4-r9}
	dmb
	mrc	p15, 1, r0, c0, c0, 1		// read CLIDR
	mov	r1, r0, lsr #23
	ands	r1, r1, #7 << 1			// r1 = lines_of_cache * 2
	beq	done				// no lines -> done
	mov	r2, #0				// start at cache level 0
1:
	add	r3, r2, r2, lsr #1		// r2 = cache level * 3
	mov	r4, r0, lsr r3
	and	r4, r4, #7			// r4 = current cache level type
	cmp	r4, #2
	blt	skip				// skip if no D-cache

	mcr	p15, 2, r2, c0, c0, 0		// set cache level in CSSR
	isb					// sync register change
	mrc	p15, 1, r4, c0, c0, 0		// read new CSIDR
	and	r3, r4, #7			// r3 = length of cache lines
	add	r3, r3, #4			// r3 += line length offset
	mov	r5, #0x3ff
	ands	r5, r5, r4, lsr #3		// r5 = max number on the way size
	clz	r6, r5				// r6 = bit position of the way size increment
	mov	r7, #0x7fff
	ands	r7, r7, r4, lsr #13		// r7 = extract max number of the index size
2:
	mov	r8, r7				// r8 = working copy of max index
3:
	orr	r9, r2, r5, lsl r6		// factor way and cache number into r9
	orr	r9, r9, r8, lsl r3		// factor index number into r9
	mcr	p15, 0, r9, c7, c14, 2		// clean/invalidate by set/way
	subs	r8, r8, #1			// decrement the index
	bge	3b
	subs	r5, r5, #1			// decrement the way
	bge	2b
skip:
	add	r2, r2, #2			// increment the cache number
	cmp	r1, r2
	bgt	1b
done:
	// ARM example code doesn't do that, but both Linux and FreeBSD do:
	mov	r0, #0
	mcr	p15, 2, r0, c0, c0, 0		// set cache level in CSSR to 0
	dsb	st
	isb
	pop	{r4-r9}
	bx	lr
