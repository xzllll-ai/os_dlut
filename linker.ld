OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
  . = 0x80000000;

  .text : ALIGN(4K) {
    PROVIDE(_kernel_start = .);
    *(.text.entry)
    *(.text .text.*)
  }

  .rodata : ALIGN(4K) {
    *(.srodata .srodata.*)
    *(.rodata .rodata.*)
  }

  .data : ALIGN(4K) {
    PROVIDE(__global_pointer$ = . + 0x800);
    *(.sdata .sdata.*)
    *(.data .data.*)
  }

  .bss : ALIGN(4K) {
    PROVIDE(_bss_start = .);
    *(.sbss .sbss.*)
    *(.bss .bss.*)
    *(COMMON)
    PROVIDE(_bss_end = .);
  }

  . = ALIGN(4K);
  PROVIDE(_kernel_end = .);

  /DISCARD/ : {
    *(.note .note.*)
    *(.eh_frame .eh_frame.*)
    *(.comment)
  }
}
