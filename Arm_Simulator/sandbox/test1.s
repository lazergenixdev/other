.data
message: .asciz "%i\n"

.text
main:
	push    {lr}
	mov     r5, #0
loop:
	cmp     r5, #10
	bgt     end
	ldr     r0, =message
	mov     r1, r5
	bl      printf
	add     r5, r5, #1
	b       loop
end: 
	pop     {lr}
	bx      lr
