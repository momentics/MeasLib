/*
 * Linker Script for AT32F403AVGT7 (Bare Metal)
 * Flash: 1024KB (994KB Code + 30KB CalData)
 * RAM: 224KB (Max SRAM configuration)
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 */

ENTRY(Reset_Handler)

MEMORY
{
    /* 1MB Flash total. 
       - CALDATA: 30K at end (matches previous size convention)
       - FLASH: Remainder (1024K - 30K = 994K) 
       
       ORIGIN Calculation for CALDATA:
       0x08000000 + 1024*1024 - 30*1024 
       = 0x08000000 + 0x100000 - 0x7800
       = 0x08100000 - 0x7800
       = 0x080F8800
    */
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 994K
    CALDATA (rx)    : ORIGIN = 0x080F8800, LENGTH = 30K
    
    /* SRAM: 224KB (AT32F403AVGT7 specific max config) */
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 224K
}

__main_stack_end__ = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* Vector Table */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        KEEP(*(.vectors)) /* Cover ChibiOS vectors name if present */
        . = ALIGN(4);
    } > FLASH

    /* Code */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)

        KEEP (*(.init))
        KEEP (*(.fini))

        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* RO Data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
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

    /* Data */
    _textdata = LOADADDR(.data);
    
    .data :
    {
        . = ALIGN(4);
        _data = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH

    /* BSS */
    .bss :
    {
        . = ALIGN(4);
        _bss_start = .;
        __bss_start__ = _bss_start;
        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _bss_end = .;
        __bss_end__ = _bss_end;
    } > RAM

    /* Calibration Storage */
    .calsave (NOLOAD) :
    {
        . = ALIGN(4);
        *(.calsave)
        . = ALIGN(4);
    } > CALDATA
}
