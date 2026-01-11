/*
 * Bare Metal Startup for STM32F072 (Cortex-M0)
 */

                .syntax unified
                .cpu    cortex-m0
                .fpu    softvfp
                .thumb

/*
 * Global Symbols
 */
                .global Reset_Handler
                .global __default_exit

/*
 * External Linker Symbols
 */
                .word   __main_stack_end__
                .word   _textdata
                .word   _data
                .word   _edata
                .word   _bss_start
                .word   _bss_end

/*
 * Reset Handler
 */
                .section .text.Reset_Handler
                .weak   Reset_Handler
                .type   Reset_Handler, %function
Reset_Handler:
                /* 1. Initialize Main Stack Pointer */
                ldr     r0, =__main_stack_end__
                mov     sp, r0

                /* 2. Copy Data Section (Flash -> RAM) */
                ldr     r1, =_textdata      /* Source (Flash) */
                ldr     r2, =_data          /* Dest (RAM)     */
                ldr     r3, =_edata         /* End of Dest    */
copy_loop:
                cmp     r2, r3              /* Check if end reached */
                bge     zero_bss
                ldr     r0, [r1]
                str     r0, [r2]
                adds    r1, #4
                adds    r2, #4
                b       copy_loop

                /* 3. Zero BSS Section */
zero_bss:
                ldr     r2, =_bss_start
                ldr     r3, =_bss_end
                movs    r0, #0
zero_loop:
                cmp     r2, r3
                bge     call_main
                str     r0, [r2]
                adds    r2, #4
                b       zero_loop

                /* 4. Call Main */
call_main:
                bl      main

                /* 5. Default Exit (Infinite Loop) */
__default_exit:
                b       __default_exit
                .size   Reset_Handler, . - Reset_Handler
