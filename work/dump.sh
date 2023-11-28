riscv64-unknown-elf-objdump -d ../obj/app_print_backtrace > asm.txt
riscv64-unknown-elf-objdump -D ../obj/app_print_backtrace > asmfull.txt
riscv64-unknown-elf-objdump -t ../obj/app_print_backtrace > sym.txt