#!/bin/bash
set -e

# Compila bootloader
nasm -f elf32 src/bootloader.asm -o src/kasm.o

# Compila kernel em C
gcc -m32 -ffreestanding -c src/kernel.c -o src/kc.o

# Linka tudo
ld -m elf_i386 -T src/link.ld -o src/kernel.bin src/kasm.o src/kc.o

# Executa no QEMU
qemu-system-i386 -kernel src/kernel.bin
