# 反汇编
riscv64-unknown-elf-objdump -d ../obj/app_print_backtrace > asm.txt
# 查看符号表
riscv64-unknown-elf-readelf -s ../obj/app_print_backtrace > readelf_symtab.txt
# 查看符号表前的所有节
riscv64-unknown-elf-readelf -S ../obj/app_print_backtrace > readelf_all_section.txt
# 以string类型查看strtab的内容
riscv64-unknown-elf-readelf -p .strtab ../obj/app_print_backtrace > readelf_strtab_string.txt
