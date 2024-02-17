# 反汇编
riscv64-unknown-elf-objdump -d ../obj/app_errorline > asm.txt
# 查看符号表前的所有节
riscv64-unknown-elf-readelf -S ../obj/app_errorline > readelf_all_section.txt
# 查看ehdr
riscv64-unknown-elf-readelf -h ../obj/app_errorline > readelf_ehdr.txt
riscv64-unknown-elf-readelf -h ./app_ls > ../readelf_ehdr_app_ls.txt