.data
message: .asciz "%d mod 3 = %d\n"

.text
main:
    mov  r9, lr
    mov  r10, #0
loop:
    cmp  r10, #10
    bge  loop_end

    mov  r0, r10
    mov  r1, #3
    bl   mod
    mov  r1, r10
    mov  r2, r0
    ldr  r0, =message
    bl   printf

    add  r10, r10, #1
    b    loop
loop_end:
    bx   r9

mod:
    sdiv r2, r0, r1
    mul  r2, r2, r1
    sub  r0, r0, r2
    bx   lr
