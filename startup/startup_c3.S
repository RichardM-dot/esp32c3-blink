/* startup_c3.S — RISC-V bare-metal startup with FreeRTOS trap vector */
    .section .init.text, "ax"
    .global _start
    .type _start, @function
_start:
    la      sp, _stack_top

    /* Install the FreeRTOS trap handler in direct mode. */
    la      t0, freertos_risc_v_trap_handler
    csrw    mtvec, t0

    /* Make sure interrupts are masked until the scheduler starts. */
    csrci   mstatus, 8

    la      t0, _bss_start
    la      t1, _bss_end
1:  bgeu    t0, t1, 2f
    sw      zero, 0(t0)
    addi    t0, t0, 4
    j       1b
2:  call    main
3:  j       3b
