
.data
message: .asciz "lr = %d\n"

.text
main:
	push	{lr}
	ldr	r0, =message
	ldr	r1, [sp]
	bl	printf
	pop	{lr}
	bx	lr
