OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
  . = 0x10000;
  .text : ALIGN(4K) {
    *(.text .text.*)
  }
  .rodata : ALIGN(4K) {
    *(.rodata .rodata.*)
  }
  .data : ALIGN(4K) {
    *(.sdata .sdata.*)
    *(.data .data.*)
  }
  .bss : ALIGN(4K) {
    *(.sbss .sbss.*)
    *(.bss .bss.*)
    *(COMMON)
  }
}
