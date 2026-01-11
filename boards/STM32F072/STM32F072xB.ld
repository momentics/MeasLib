/*
 * Linker Script for STM32F072xB (Bare Metal)
 * Flash: 128KB (98KB Code + 30KB CalData)
 * RAM: 16KB
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 98K
    CALDATA (rx)    : ORIGIN = 0x08018800, LENGTH = 30K
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 16K
}

/* stack end = end of RAM */
__main_stack_end__ = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* Vector Table */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector)) /* Startup code */
        KEEP(*(.vectors))    /* ChibiOS vectors.c uses .vectors section? Check vectors.c */
        . = ALIGN(4);
    } > FLASH

    /* Code and Read-Only Data */
    .text :
    {
        . = ALIGN(4);
        *(.text)           /* .text sections (code) */
        *(.text*)          /* .text* sections (code) */
        *(.glue_7)         /* glue arm to thumb code */
        *(.glue_7t)        /* glue thumb to arm code */
        *(.eh_frame)

        KEEP (*(.init))
        KEEP (*(.fini))

        . = ALIGN(4);
        _etext = .;        /* define a global symbols at end of code */
    } > FLASH

    /* Constant Data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)         /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
        . = ALIGN(4);
    } > FLASH

    .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
    .ARM : {
    __exidx_start = .;
      *(.ARM.exidx*)
      __exidx_end = .;
    } >FLASH

    .preinit_array     :
    {
      PROVIDE_HIDDEN (__preinit_array_start = .);
      KEEP (*(.preinit_array*))
      PROVIDE_HIDDEN (__preinit_array_end = .);
    } >FLASH
    .init_array :
    {
      PROVIDE_HIDDEN (__init_array_start = .);
      KEEP (*(SORT(.init_array.*)))
      KEEP (*(.init_array*))
      PROVIDE_HIDDEN (__init_array_end = .);
    } >FLASH
    .fini_array :
    {
      PROVIDE_HIDDEN (__fini_array_start = .);
      KEEP (*(SORT(.fini_array.*)))
      KEEP (*(.fini_array*))
      PROVIDE_HIDDEN (__fini_array_end = .);
    } >FLASH

    /* Data (Initialized) */
    _textdata = LOADADDR(.data);
    
    .data :
    {
        . = ALIGN(4);
        _data = .;         /* create a global symbol at data start */
        *(.data)           /* .data sections */
        *(.data*)          /* .data* sections */
        . = ALIGN(4);
        _edata = .;        /* define a global symbol at data end */
    } > RAM AT> FLASH

    /* BSS (Uninitialized) */
    .bss :
    {
        . = ALIGN(4);
        _bss_start = .;    /* define a global symbol at bss start */
        __bss_start__ = _bss_start;
        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _bss_end = .;      /* define a global symbol at bss end */
        __bss_end__ = _bss_end;
    } > RAM

    /* Calibration Storage */
    .calsave (NOLOAD) :
    {
        . = ALIGN(4);
        *(.calsave)
        . = ALIGN(4);
    } > CALDATA

    /* User Heap and Stack checks could be added here */
}
