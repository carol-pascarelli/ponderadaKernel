#define VGA_ADDRESS 0xB8000
#define VGA_COLS 80
#define VGA_ROWS 25

#include "keyboard_map.h"

unsigned short* vga_buffer = (unsigned short*) VGA_ADDRESS;
int cursor_row = 0;
int cursor_col = 0;

void clear_screen() {
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++) {
        vga_buffer[i] = (0x0F << 8) | ' ';
    }
    cursor_row = 0;
    cursor_col = 0;
}

void print_char(char c) {
    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;
    } else {
        vga_buffer[cursor_row * VGA_COLS + cursor_col] = (0x0F << 8) | c;
        cursor_col++;
        if (cursor_col >= VGA_COLS) {
            cursor_col = 0;
            cursor_row++;
        }
    }
}

void print_string(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}

// Função chamada pelo bootloader
void kernel_main() {
    clear_screen();
    print_string("Bem-vindo ao Microkernel!\n");
    print_string("Digite algo no teclado (ainda nao implementado).\n");

    // Loop infinito para o kernel não terminar
    while(1) {
        // Aqui você pode colocar futuras funções, teclado, jogo, etc
    }
}
