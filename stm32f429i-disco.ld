/* Linker script for STM32F429ZIT6 (2MB Flash, 256KB RAM) */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
    RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 192K
    CCMRAM (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

/* Include the common STM32 linker script from LibOpenCM3 */
INCLUDE libopencm3/lib/cortex-m-generic.ld
