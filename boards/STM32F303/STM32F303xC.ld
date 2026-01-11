/*
 * Linker Script for STM32F303xC (Bare Metal)
 * Flash: 256KB (142KB Code + 114KB CalData)
 * RAM: 40KB
 * CCM: 8KB
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 142K
    CALDATA (rx)    : ORIGIN = 0x0801C800, LENGTH = 114K
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 40K
    CCMRAM (xrw)    : ORIGIN = 0x10000000, LENGTH = 8K
}

__main_stack_end__ = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* Vector Table */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        KEEP(*(.vectors))
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

    /* CCM RAM */
    .ccm (NOLOAD) :
    {
        . = ALIGN(4);
        *(.ccm)
        *(.ccm.*)
        . = ALIGN(4);
    } > CCMRAM

    /* Calibration Storage */
    .calsave (NOLOAD) :
    {
        . = ALIGN(4);
        *(.calsave)
        . = ALIGN(4);
    } > CALDATA
}
