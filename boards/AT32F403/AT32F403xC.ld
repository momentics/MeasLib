/*
    Linker script for Artery AT32F403xC
    Flash: 256KB
    RAM:   96KB
*/

MEMORY
{
    /* Flash: 226KB Code + 30KB CalData = 256KB Total */
    flash0  : org = 0x08000000, len = 226k
    flash7  : org = 0x08038800, len = 30k
    
    /* SRAM: 96KB */
    ram0    : org = 0x20000000, len = 96k
}

/* For each data/text section two region are defined, a virtual region
   and a load region (_LMA suffix).*/

REGION_ALIAS("VECTORS_FLASH", flash0);
REGION_ALIAS("VECTORS_FLASH_LMA", flash0);

REGION_ALIAS("XTORS_FLASH", flash0);
REGION_ALIAS("XTORS_FLASH_LMA", flash0);

REGION_ALIAS("TEXT_FLASH", flash0);
REGION_ALIAS("TEXT_FLASH_LMA", flash0);

REGION_ALIAS("RODATA_FLASH", flash0);
REGION_ALIAS("RODATA_FLASH_LMA", flash0);

REGION_ALIAS("VARIOUS_FLASH", flash0);
REGION_ALIAS("VARIOUS_FLASH_LMA", flash0);

REGION_ALIAS("RAM_INIT_FLASH_LMA", flash0);

REGION_ALIAS("CALDATA_FLASH", flash7);

REGION_ALIAS("MAIN_STACK_RAM", ram0);
REGION_ALIAS("PROCESS_STACK_RAM", ram0);
REGION_ALIAS("DATA_RAM", ram0);
REGION_ALIAS("DATA_RAM_LMA", flash0);
REGION_ALIAS("BSS_RAM", ram0);
REGION_ALIAS("HEAP_RAM", ram0);

INCLUDE rules.ld

SECTIONS
{
    .calsave (NOLOAD) : ALIGN(4)
    {
        *(.calsave)
    } > CALDATA_FLASH
}
