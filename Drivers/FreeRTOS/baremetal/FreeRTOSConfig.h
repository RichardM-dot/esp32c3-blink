/*
 * FreeRTOSConfig.h — bare-metal ESP32-C3 (no ESP-IDF build system)
 *
 * This is a deliberately small configuration: single core, preemptive,
 * 1 kHz tick, heap_4.  It must stay independent of sdkconfig/Kconfig.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      ( 160000000UL )
#define configTICK_RATE_HZ                      ( 1000 )
#define configMAX_PRIORITIES                    ( 8 )
#define configMINIMAL_STACK_SIZE                ( 256 )
#define configTOTAL_HEAP_SIZE                   ( 64 * 1024 )
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 0
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_QUEUE_SETS                    0
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configAPPLICATION_ALLOCATED_HEAP        0
#define configSTACK_DEPTH_TYPE                  uint32_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0
#define configGENERATE_RUN_TIME_STATS           0
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configRECORD_STACK_HIGH_ADDRESS         1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0

/* Single core kernel (this is the ESP-IDF SMP-capable source tree). */
#define configNUMBER_OF_CORES                   1
#define configNUM_CORES                         configNUMBER_OF_CORES
#define configUSE_CORE_AFFINITY                 0
#define configUSE_PASSIVE_IDLE_HOOK             0
#define configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H 1
#define configISR_STACK_SIZE                    ( 1024 )

#define configMAX_SYSCALL_INTERRUPT_PRIORITY    0
#define configKERNEL_INTERRUPT_PRIORITY         1

#define configASSERT( x ) \
    do { if( !( x ) ) { for( ;; ) { __asm__ volatile( "nop" ); } } } while( 0 )

/* API inclusion */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1

/* Trace hooks used by this kernel tree but not defaulted by FreeRTOS.h. */
#define traceQUEUE_GIVE_FROM_ISR( pxQueue )             do { } while( 0 )
#define traceQUEUE_GIVE_FROM_ISR_FAILED( pxQueue )      do { } while( 0 )
#define traceQUEUE_SEMAPHORE_RECEIVE( pxQueue )         do { } while( 0 )

#endif /* FREERTOS_CONFIG_H */
