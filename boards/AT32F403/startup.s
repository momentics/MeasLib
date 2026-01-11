/*
 * Bare Metal Startup for STM32F303/AT32F403 (Cortex-M4F)
 */

                .syntax unified
                .cpu    cortex-m4
                .fpu    fpv4-sp-d16
                .thumb

/*
 * Global Symbols
 */
                .global Reset_Handler
                .global __default_exit

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

                /* 2. Enable FPU (CPACR |= 0xF << 20) */
                ldr     r0, =0xE000ED88     /* SCB->CPACR */
                ldr     r1, [r0]
                orr     r1, r1, #(0xF << 20)
                str     r1, [r0]
                dsb
                isb

                /* 3. Copy Data Section (Flash -> RAM) */
                ldr     r1, =_textdata      /* Source (Flash) */
                ldr     r2, =_data          /* Dest (RAM)     */
                ldr     r3, =_edata         /* End of Dest    */
copy_loop:
                cmp     r2, r3
                bge     zero_bss
                ldr     r0, [r1], #4
                str     r0, [r2], #4
                b       copy_loop

                /* 4. Zero BSS Section */
zero_bss:
                ldr     r2, =_bss_start
                ldr     r3, =_bss_end
                mov     r0, #0
zero_loop:
                cmp     r2, r3
                bge     call_main
                str     r0, [r2], #4
                b       zero_loop

                /* 5. Call Main */
call_main:
                bl      SystemInit
                bl      main

                /* 6. Default Exit */
__default_exit:
                b       __default_exit
                .size   Reset_Handler, . - Reset_Handler
