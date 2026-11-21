/*
 * portmacro.h — minimal FreeRTOS RISC-V (RV32IMC) port for ESP32-C3.
 *
 * The official ESP-IDF RISC-V port in this tree is the SMP variant that
 * depends on the IDF systimer HAL and interrupt allocator.  For a bare-metal
 * build we use this small port instead: mtvec direct mode, SYSTIMER target0
 * as the tick source, and a plain call-based vPortYield() (no ecall).
 */
#ifndef PORTMACRO_H
#define PORTMACRO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Types */
#define portSTACK_TYPE              uint32_t
#define portBASE_TYPE               long
#define portUBASE_TYPE              unsigned long
#define portPOINTER_SIZE_TYPE       uint32_t

typedef portSTACK_TYPE   StackType_t;
typedef long             BaseType_t;
typedef unsigned long    UBaseType_t;
typedef uint32_t         TickType_t;

#define portMAX_DELAY               ( ( TickType_t ) 0xffffffffUL )
#define portTICK_TYPE_IS_ATOMIC     1

/* Architecture */
#define portBYTE_ALIGNMENT          16
#define portNUM_PROCESSORS          1
#define portSTACK_GROWTH            ( -1 )
#define portHAS_STACK_OVERFLOW_CHECKING 0
#define portCRITICAL_NESTING_IN_TCB 0
#define portTICK_PERIOD_MS          ( ( TickType_t ) ( 1000 / configTICK_RATE_HZ ) )

/* The ESP-IDF kernel structs always contain a portMUX_TYPE even on UP. */
typedef struct
{
    uint32_t owner;
    uint32_t count;
} portMUX_TYPE;

#define portMUX_INITIALIZER_UNLOCKED    { 0, 0 }
#define portMUX_INITIALIZE( mux )       do { ( mux )->owner = 0; ( mux )->count = 0; } while( 0 )
#define portVALID_LIST_MEM( p )         ( ( p ) != NULL )
#define portVALID_TCB_MEM( p )          ( ( p ) != NULL )

/* Critical sections */
void vPortEnterCritical( void );
void vPortExitCritical( void );

/* The ESP-IDF kernel passes a per-object lock argument (unused on UP). */
#define portENTER_CRITICAL( ... )       vPortEnterCritical()
#define portEXIT_CRITICAL( ... )        vPortExitCritical()
#define portENTER_CRITICAL_ISR( ... )   do { } while( 0 )
#define portEXIT_CRITICAL_ISR( ... )    do { } while( 0 )

#define portDISABLE_INTERRUPTS()    __asm__ volatile( "csrci mstatus, 8" ::: "memory" )
#define portENABLE_INTERRUPTS()     __asm__ volatile( "csrsi mstatus, 8" ::: "memory" )

#define portSET_INTERRUPT_MASK_FROM_ISR()           ( 0 )
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( x )      ( ( void ) ( x ) )
#define portASSERT_IF_INTERRUPT_PRIORITY_INVALID()

/* Context switching */
void vPortYield( void );
void vPortYieldFromISR( void );

#define portYIELD()                 vPortYield()
#define portYIELD_FROM_ISR( x )     vPortYieldFromISR()
#define portEND_SWITCHING_ISR( x )  vPortYieldFromISR()

#define portGET_CORE_ID()           0
#define portYIELD_CORE( x )         ( ( void ) ( x ) )

#define portNOP()                   __asm__ volatile( "nop" )
#define portMEMORY_BARRIER()        __asm__ volatile( "" ::: "memory" )

#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )  void vFunction( void * pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )        void vFunction( void * pvParameters )

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */
