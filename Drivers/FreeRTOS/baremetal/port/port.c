/*
 * port.c — minimal FreeRTOS RISC-V port for the ESP32-C3 (bare metal)
 *
 * Tick source: SYSTIMER unit0 in period mode, target0, 1 kHz.
 * Trap entry:  mtvec direct mode -> freertos_risc_v_trap_handler (portasm.S).
 *
 * No ESP-IDF interrupt allocator, no HAL, no CLINT: the C3 has none.
 */
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"

#include "soc/soc.h"
#include "soc/reg_base.h"
#include "soc/systimer_reg.h"
#include "soc/system_reg.h"
#include "soc/interrupt_core0_reg.h"

/* SYSTIMER runs at 16 MHz on the C3 (fixed 16 ticks/us). */
#define SYSTIMER_TICKS_PER_MS       16000U
#define SYSTIMER_TARGET0_CPU_INT    6U

/* Critical nesting counter.  Single core only. */
static UBaseType_t uxCriticalNesting = 0;
static volatile UBaseType_t uxSwitchPending = 0;

/* Provided by portasm.S */
extern void vPortStartFirstTask( void );
extern void freertos_risc_v_trap_handler( void );
extern void prvTaskExitError( void );

/* Keep the trap vector reachable from C (used by startup as a safety net). */
void vPortInstallTrapVector( void )
{
    __asm__ volatile( "csrw mtvec, %0" :: "r"( freertos_risc_v_trap_handler ) : "memory" );
}

void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
    uxCriticalNesting++;
}

void vPortExitCritical( void )
{
    configASSERT( uxCriticalNesting > 0 );
    uxCriticalNesting--;
    if( uxCriticalNesting == 0 )
    {
        portENABLE_INTERRUPTS();
    }
}

void vPortYieldFromISR( void )
{
    uxSwitchPending = 1;
}

/* Called from the trap handler with the saved frame pointer.  Returns 1 if
 * the trap return path must load a new task stack. */
uint32_t freertos_trap_handler_c( uint32_t *frame )
{
    uint32_t mcause = frame[ 33 ];
    uint32_t switch_required = 0;

    if( ( mcause & 0x80000000UL ) != 0 )
    {
        /* Interrupt.  Only SYSTIMER target0 is enabled. */
        REG_WRITE( SYSTIMER_INT_CLR_REG, SYSTIMER_TARGET0_INT_CLR );
        REG_WRITE( INTERRUPT_CORE0_CPU_INT_CLEAR_REG, ( 1U << SYSTIMER_TARGET0_CPU_INT ) );

        if( xTaskIncrementTick() != pdFALSE )
        {
            switch_required = 1;
        }
    }

    if( uxSwitchPending != 0 )
    {
        uxSwitchPending = 0;
        switch_required = 1;
    }

    return switch_required;
}

static void vPortSetupTimerInterrupt( void )
{
    /* Ungate SYSTIMER and release its reset. */
    REG_WRITE( SYSTEM_PERIP_CLK_EN0_REG,
               REG_READ( SYSTEM_PERIP_CLK_EN0_REG ) | SYSTEM_SYSTIMER_CLK_EN );
    REG_WRITE( SYSTEM_PERIP_RST_EN0_REG,
               REG_READ( SYSTEM_PERIP_RST_EN0_REG ) & ~SYSTEM_SYSTIMER_RST );

    /* Register-file clock on, unit0 running. */
    REG_WRITE( SYSTIMER_CONF_REG,
               REG_READ( SYSTIMER_CONF_REG ) |
               ( 1U << 31 ) | SYSTIMER_TIMER_UNIT0_WORK_EN );

    /* Period mode: target0 fires every SYSTIMER_TICKS_PER_MS ticks. */
    REG_WRITE( SYSTIMER_TARGET0_CONF_REG,
               ( SYSTIMER_TICKS_PER_MS & SYSTIMER_TARGET0_PERIOD_M ) |
               SYSTIMER_TARGET0_PERIOD_MODE |
               ( 0U << SYSTIMER_TARGET0_TIMER_UNIT_SEL_S ) );

    /* Clear any stale condition, then enable the peripheral interrupt. */
    REG_WRITE( SYSTIMER_INT_CLR_REG, SYSTIMER_TARGET0_INT_CLR );
    REG_WRITE( SYSTIMER_INT_ENA_REG, SYSTIMER_TARGET0_INT_ENA );

    /* Route target0 to CPU interrupt 6, level triggered, priority 1. */
    REG_WRITE( INTERRUPT_CORE0_SYSTIMER_TARGET0_INT_MAP_REG, SYSTIMER_TARGET0_CPU_INT );
    REG_WRITE( INTERRUPT_CORE0_CPU_INT_TYPE_REG,
               REG_READ( INTERRUPT_CORE0_CPU_INT_TYPE_REG ) &
               ~( 1U << SYSTIMER_TARGET0_CPU_INT ) );
    REG_WRITE( INTERRUPT_CORE0_CPU_INT_PRI_6_REG, 1U );
    REG_WRITE( INTERRUPT_CORE0_CPU_INT_THRESH_REG, 0U );
    REG_WRITE( INTERRUPT_CORE0_CPU_INT_CLEAR_REG, ( 1U << SYSTIMER_TARGET0_CPU_INT ) );
    REG_WRITE( INTERRUPT_CORE0_CPU_INT_ENABLE_REG,
               REG_READ( INTERRUPT_CORE0_CPU_INT_ENABLE_REG ) |
               ( 1U << SYSTIMER_TARGET0_CPU_INT ) );

    /* Target0 work enable (period mode generates the alarm itself). */
    REG_WRITE( SYSTIMER_CONF_REG,
               REG_READ( SYSTIMER_CONF_REG ) | SYSTIMER_TARGET0_WORK_EN );
}

BaseType_t xPortStartScheduler( void )
{
    uxCriticalNesting = 0;
    uxSwitchPending = 0;

    vPortInstallTrapVector();
    vPortSetupTimerInterrupt();

    /* Never returns: restores pxCurrentTCB's initial frame and mret's into it. */
    vPortStartFirstTask();

    return pdFALSE;
}

void vPortEndScheduler( void )
{
    for( ;; )
    {
        __asm__ volatile( "nop" );
    }
}

/* Idle hook installed by the ESP-IDF kernel core. */
void esp_vApplicationIdleHook( void )
{
}

void prvTaskExitError( void )
{
    portDISABLE_INTERRUPTS();
    for( ;; )
    {
        __asm__ volatile( "nop" );
    }
}

/* The kernel's single-core code asks for this through portGET_CORE_ID(), but
 * some IDF additions call the function form. */
BaseType_t xPortGetCoreID( void )
{
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Initial stack frame for a new task.  Matches the layout in portasm.S.      */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    uint32_t * frame = ( uint32_t * ) ( pxTopOfStack - 36 );
    uint32_t i;

    for( i = 0; i < 36; i++ )
    {
        frame[ i ] = 0;
    }

    frame[ 0 ]  = ( uint32_t ) prvTaskExitError; /* ra: task must not return */
    frame[ 1 ]  = ( uint32_t ) pxTopOfStack;     /* initial sp               */
    frame[ 9 ]  = ( uint32_t ) pvParameters;     /* a0: task argument        */
    frame[ 31 ] = ( uint32_t ) pxCode;           /* mepc: task entry         */
    frame[ 32 ] = 0x00001880U;                   /* MPP=3, MPIE=1, MIE=0     */

    return ( StackType_t * ) frame;
}
