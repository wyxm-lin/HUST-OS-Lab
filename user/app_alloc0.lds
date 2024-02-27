OUTPUT_ARCH( "riscv" )

ENTRY(main)

SECTIONS
{
  . = 0x100b0;
  . = ALIGN(0x10);
  .text : { *(.text) }
  . = ALIGN(16);
  .data : { *(.data) }
  . = ALIGN(16);
  .bss : { *(.bss) }
}