#include "keyboard_map.h"

#define MAX_TENTATIVAS 6
#define PALAVRA_TAM 5
#define NUM_PALAVRAS 10

const char *lista_palavras[NUM_PALAVRAS] = {
    "ARVOR", "CASAS", "LIVRO", "FELIZ", "PLANO",
    "GRITO", "FORTE", "NORTE", "CARRO", "LINHA"
};

char palavra_correta[PALAVRA_TAM+1];
char tentativa[PALAVRA_TAM+1];
int tentativas = 0;
int letra_atual = 0;
int ganhou = 0;

#define LINES 25
#define COLUMNS 80
#define BYTES 2
#define SCREENSIZE LINES * COLUMNS * BYTES

unsigned int current_loc = 0;
char *vidptr = (char*)0xb8000;

extern void keyboard_handler(void);
extern void load_idt(unsigned long *idt_ptr);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);

/* ---------------------- Funções de vídeo ---------------------- */

// Adicionar no topo do kernel.c
void keyboard_handler_main(void) {
    unsigned char status;
    char keycode;

    // Lida com interrupção do teclado
    write_port(0x20, 0x20); // envia EOI para PIC

    status = read_port(0x64);
    if(status & 0x01) {
        keycode = read_port(0x60);
        if(keycode == 0x0E) { // Backspace
            apagarLetra();
            return;
        }

        if(keycode == 0x1C) { // Enter
            verificarPalavra();
            return;
        }

        char letra = keyboard_map[(unsigned char)keycode];
        if(letra >= 'a' && letra <= 'z') {
            letra = letra - 'a' + 'A';
            inserirLetra(letra);
        }
    }
}

void kprint(const char *str){
    for(int i=0; str[i]; i++){
        vidptr[current_loc++] = str[i];
        vidptr[current_loc++] = 0x07;
    }
}

void kprint_newline(){
    current_loc += BYTES * COLUMNS - (current_loc % (BYTES * COLUMNS));
}

void clear_screen(){
    for(int i=0; i<SCREENSIZE; i+=2){
        vidptr[i] = ' ';
        vidptr[i+1] = 0x07;
    }
    current_loc = 0;
}

/* ---------------------- Forca ---------------------- */

void escolher_palavra(){
    unsigned char semente = read_port(0x64);
    int indice = semente % NUM_PALAVRAS;
    for(int i=0;i<PALAVRA_TAM;i++) palavra_correta[i] = lista_palavras[indice][i];
    palavra_correta[PALAVRA_TAM] = '\0';
}

void inserirLetra(char c){
    if(letra_atual < PALAVRA_TAM){
        tentativa[letra_atual++] = c;
        char str[2] = {c, '\0'};
        kprint(str);
    }
}

void apagarLetra(){
    if(letra_atual>0){
        letra_atual--;
        tentativa[letra_atual]='\0';
        current_loc-=2;
        vidptr[current_loc]=' ';
        vidptr[current_loc+1]=0x07;
    }
}

void verificarPalavra(){
    if(letra_atual!=PALAVRA_TAM) return;
    int acertos=0;
    unsigned int line_size = BYTES*COLUMNS;
    current_loc = (current_loc / line_size) * line_size;

    for(int i=0;i<PALAVRA_TAM;i++){
        if(tentativa[i]==palavra_correta[i]){
            vidptr[current_loc+i*2] = tentativa[i];
            vidptr[current_loc+i*2+1] = 0x0A; // verde
            acertos++;
        } else {
            vidptr[current_loc+i*2+1] = 0x07; // cinza
        }
    }

    if(acertos==PALAVRA_TAM) ganhou=1;

    kprint_newline();
    tentativas++;
    letra_atual=0;
}

void mensagem_ganhou(){
    kprint_newline();
    kprint("VOCE VENCEU!");
    kprint_newline();
}

/* ---------------------- Inicialização ---------------------- */

void inicializar(){
    escolher_palavra();
    clear_screen();
    kprint("BEM-VINDO AO JOGO DA FORCA!");
    kprint_newline();
    kprint("Digite uma palavra de 5 letras e ENTER");
    kprint_newline();
}

void kmain(void){
    inicializar();
    while(!ganhou);
    mensagem_ganhou();
}
