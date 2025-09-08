.align 4
.global _arm_kvtophys, _arbitrary_call

_arm_kvtophys:
    bti c
    mrs x2, DAIF
    msr DAIFSet, #0xF

    at s1e1r, x0
    isb sy
    mrs x1, PAR_EL1
    msr DAIF, x2

    tbnz x1, #0, L_kvtop_invalid
    bfm x1, x0, #0, #11
    and x0, x1, #0x0000ffffffffffff
    ret

L_kvtop_invalid:
    mov x0, #0
    ret

/* 10 64-bit argument kernel call primitive
extern "C" uint64_t arbitrary_call
(
    uint64_t func,
    ...
);
*/
_arbitrary_call:

    // Load the function pointer into X16
    mov x16, x0
    xpaci x16
    
    // Because this is a variadic function,
    // up to 10 arguments are placed onto
    // the stack, so we need to load them
    // correctly for the call.

    // General purpose registers
    ldp x0, x1, [sp]
    ldp x2, x3, [sp, #0x10]
    ldp x4, x5, [sp, #0x20]
    ldp x6, x7, [sp, #0x30]

    // Stack arguments
    ldp x8, x9, [sp, #0x40]
    stp x8, x9, [sp]

    // Clear X8 and X9 before the call
    mov x8, #0
    mov x9, #0

    br x16