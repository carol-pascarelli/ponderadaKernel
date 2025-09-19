#!/bin/bash
set -e

# Compila bootloader em ELF 32-bit
nasm -f elf32 src/bootloader.asm -o src/bootloader.o

# Compila kernel em ELF 32-bit
gcc -m32 -ffreestanding -c src/kernel.c -o src/kernel.o

# Linka bootloader + kernel
ld -m elf_i386 -T src/link.ld -o src/kernel.bin src/bootloader.o src/kernel.o

# Roda no QEMU
qemu-system-i386 -kernel src/kernel.bin -no-reboot
