# �����
riscv64-unknown-elf-objdump -d ../obj/app_errorline > asm.txt
# �鿴���ű�ǰ�����н�
riscv64-unknown-elf-readelf -S ../obj/app_errorline > readelf_all_section.txt