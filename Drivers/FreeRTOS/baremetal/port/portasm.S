/*
 * portasm.S — RISC-V trap entry, context save/restore, first-task start.
 *
 * Frame layout (36 words / 144 bytes, 16-byte aligned), sp points at word 0:
 *   0  ra      1  sp      2  gp      3  tp
 *   4  t0      5  t1      6  t2      7  s0      8  s1
 *   9  a0     10  a1     11  a2     12  a3     13  a4     14  a5     15  a6     16  a7
 *  17  s2     18  s3     19  s4     20  s5     21  s6     22  s7     23  s8
 *  24  s9     25  s10    26  s11    27  t3     28  t4     29  t5     30  t6
 *  31  mepc   32  mstatus 33  mcause 34  pad    35  pad
 */
    .equ CTX_WORDS, 36
    .equ CTX_SIZE,  (CTX_WORDS * 4)

    .equ OFF_RA,      0
    .equ OFF_SP,      4
    .equ OFF_GP,      8
    .equ OFF_TP,      12
    .equ OFF_T0,      16
    .equ OFF_T1,      20
    .equ OFF_T2,      24
    .equ OFF_S0,      28
    .equ OFF_S1,      32
    .equ OFF_A0,      36
    .equ OFF_A1,      40
    .equ OFF_A2,      44
    .equ OFF_A3,      48
    .equ OFF_A4,      52
    .equ OFF_A5,      56
    .equ OFF_A6,      60
    .equ OFF_A7,      64
    .equ OFF_S2,      68
    .equ OFF_S3,      72
    .equ OFF_S4,      76
    .equ OFF_S5,      80
    .equ OFF_S6,      84
    .equ OFF_S7,      88
    .equ OFF_S8,      92
    .equ OFF_S9,      96
    .equ OFF_S10,     100
    .equ OFF_S11,     104
    .equ OFF_T3,      108
    .equ OFF_T4,      112
    .equ OFF_T5,      116
    .equ OFF_T6,      120
    .equ OFF_MEPC,    124
    .equ OFF_MSTATUS, 128
    .equ OFF_MCAUSE,  132

    .section .text
    .align 2
    .global freertos_risc_v_trap_handler
    .type   freertos_risc_v_trap_handler, @function
freertos_risc_v_trap_handler:
    addi    sp, sp, -CTX_SIZE

    /* Save t0 first, then use it to compute the interrupted sp. */
    sw      t0, OFF_T0(sp)
    addi    t0, sp, CTX_SIZE
    sw      t0, OFF_SP(sp)

    sw      ra,      OFF_RA(sp)
    sw      gp,      OFF_GP(sp)
    sw      tp,      OFF_TP(sp)
    sw      t1,      OFF_T1(sp)
    sw      t2,      OFF_T2(sp)
    sw      s0,      OFF_S0(sp)
    sw      s1,      OFF_S1(sp)
    sw      a0,      OFF_A0(sp)
    sw      a1,      OFF_A1(sp)
    sw      a2,      OFF_A2(sp)
    sw      a3,      OFF_A3(sp)
    sw      a4,      OFF_A4(sp)
    sw      a5,      OFF_A5(sp)
    sw      a6,      OFF_A6(sp)
    sw      a7,      OFF_A7(sp)
    sw      s2,      OFF_S2(sp)
    sw      s3,      OFF_S3(sp)
    sw      s4,      OFF_S4(sp)
    sw      s5,      OFF_S5(sp)
    sw      s6,      OFF_S6(sp)
    sw      s7,      OFF_S7(sp)
    sw      s8,      OFF_S8(sp)
    sw      s9,      OFF_S9(sp)
    sw      s10,     OFF_S10(sp)
    sw      s11,     OFF_S11(sp)
    sw      t3,      OFF_T3(sp)
    sw      t4,      OFF_T4(sp)
    sw      t5,      OFF_T5(sp)
    sw      t6,      OFF_T6(sp)

    csrr    t0, mepc
    sw      t0, OFF_MEPC(sp)
    csrr    t0, mstatus
    sw      t0, OFF_MSTATUS(sp)
    csrr    t0, mcause
    sw      t0, OFF_MCAUSE(sp)

    mv      a0, sp
    call    freertos_trap_handler_c

    bnez    a0, .Lload_next_task
    j       .Lrestore_context

.Lload_next_task:
    la      t0, pxCurrentTCBs
    lw      t0, 0(t0)
    lw      sp, 0(t0)

.Lrestore_context:
    lw      t0, OFF_MEPC(sp)
    csrw    mepc, t0
    lw      t0, OFF_MSTATUS(sp)
    csrw    mstatus, t0

    lw      ra,  OFF_RA(sp)
    lw      gp,  OFF_GP(sp)
    lw      tp,  OFF_TP(sp)
    lw      t1,  OFF_T1(sp)
    lw      t2,  OFF_T2(sp)
    lw      s0,  OFF_S0(sp)
    lw      s1,  OFF_S1(sp)
    lw      a0,  OFF_A0(sp)
    lw      a1,  OFF_A1(sp)
    lw      a2,  OFF_A2(sp)
    lw      a3,  OFF_A3(sp)
    lw      a4,  OFF_A4(sp)
    lw      a5,  OFF_A5(sp)
    lw      a6,  OFF_A6(sp)
    lw      a7,  OFF_A7(sp)
    lw      s2,  OFF_S2(sp)
    lw      s3,  OFF_S3(sp)
    lw      s4,  OFF_S4(sp)
    lw      s5,  OFF_S5(sp)
    lw      s6,  OFF_S6(sp)
    lw      s7,  OFF_S7(sp)
    lw      s8,  OFF_S8(sp)
    lw      s9,  OFF_S9(sp)
    lw      s10, OFF_S10(sp)
    lw      s11, OFF_S11(sp)
    lw      t3,  OFF_T3(sp)
    lw      t4,  OFF_T4(sp)
    lw      t5,  OFF_T5(sp)
    lw      t6,  OFF_T6(sp)

    /* Restore the original stack pointer last. */
    lw      sp, OFF_SP(sp)
    mret

/* ------------------------------------------------------------------------- */
    .align 2
    .global vPortStartFirstTask
    .type   vPortStartFirstTask, @function
vPortStartFirstTask:
    la      t0, pxCurrentTCBs
    lw      t0, 0(t0)
    lw      sp, 0(t0)
    j       .Lrestore_context

/* ------------------------------------------------------------------------- */
/* void vPortYield(void) — direct, call-based context switch. */
    .align 2
    .global vPortYield
    .type   vPortYield, @function
vPortYield:
    addi    sp, sp, -CTX_SIZE

    sw      t0, OFF_T0(sp)
    addi    t0, sp, CTX_SIZE
    sw      t0, OFF_SP(sp)

    sw      ra,      OFF_RA(sp)
    sw      gp,      OFF_GP(sp)
    sw      tp,      OFF_TP(sp)
    sw      t1,      OFF_T1(sp)
    sw      t2,      OFF_T2(sp)
    sw      s0,      OFF_S0(sp)
    sw      s1,      OFF_S1(sp)
    sw      a0,      OFF_A0(sp)
    sw      a1,      OFF_A1(sp)
    sw      a2,      OFF_A2(sp)
    sw      a3,      OFF_A3(sp)
    sw      a4,      OFF_A4(sp)
    sw      a5,      OFF_A5(sp)
    sw      a6,      OFF_A6(sp)
    sw      a7,      OFF_A7(sp)
    sw      s2,      OFF_S2(sp)
    sw      s3,      OFF_S3(sp)
    sw      s4,      OFF_S4(sp)
    sw      s5,      OFF_S5(sp)
    sw      s6,      OFF_S6(sp)
    sw      s7,      OFF_S7(sp)
    sw      s8,      OFF_S8(sp)
    sw      s9,      OFF_S9(sp)
    sw      s10,     OFF_S10(sp)
    sw      s11,     OFF_S11(sp)
    sw      t3,      OFF_T3(sp)
    sw      t4,      OFF_T4(sp)
    sw      t5,      OFF_T5(sp)
    sw      t6,      OFF_T6(sp)

    /* Return address becomes the resume PC. */
    sw      ra, OFF_MEPC(sp)
    sw      zero, OFF_MCAUSE(sp)

    /* mret returns with the interrupt enable level the caller had. */
    csrr    t0, mstatus
    andi    t0, t0, 8
    beqz    t0, 1f
    li      t0, 0x1880          /* MPP=3, MPIE=1 */
    j       2f
1:  li      t0, 0x1800          /* MPP=3, MPIE=0 */
2:  sw      t0, OFF_MSTATUS(sp)

    la      t0, pxCurrentTCBs
    lw      t0, 0(t0)
    sw      sp, 0(t0)

    call    vTaskSwitchContext

    la      t0, pxCurrentTCBs
    lw      t0, 0(t0)
    lw      sp, 0(t0)
    j       .Lrestore_context
