#include "keyboard_map.h"

#define MAX_TENTATIVAS 6
#define PALAVRA_TAM 6
#define NUM_PALAVRAS 10

const char *lista_palavras[NUM_PALAVRAS] = {
    "ENIGMA", "INSANO", "CIFRAR", "ORBITA", "QUADRO",
    "VORTEX", "MATRIZ", "MORADA", "ESCUDO", "PIXELS"
};

char palavra_correta[PALAVRA_TAM+1];
char tentativa[PALAVRA_TAM+1];
int tentativas = 0;
int letra_atual = 0;
int ganhou = 0;
int game_over = 0;
int waiting_restart = 0;

// Tamanho da tela
#define LINES 25
#define COLUMNS 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE (BYTES_FOR_EACH_ELEMENT * COLUMNS * LINES)
// Portas do teclado
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C

// Ponteiro para vídeo
char *vidptr = (char*)0xb8000;
unsigned int current_loc = 0;

// Estrutura IDT
struct IDT_entry {
    unsigned short offset_lowerbits;
    unsigned short selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];

// Protótipos
void kmain(void);
void idt_init(void);
void kb_init(void);
void keyboard_handler_main(void);
void inicializar(void);
void inserirLetra(char c);
void apagarLetra(void);
void verificarPalavra(void);
void kprint(const char *str);
void kprint_newline(void);
void kprint_char_color(char c, unsigned char color);
void clear_screen(void);
void mensagem_ganhou(void);
void mensagem_perdeu(void);
void reiniciar_jogo(void);

// Funções externas do bootloader
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

// -----------------------------------------------------------------
// Inicialização da IDT
void idt_init(void)
{
    unsigned long keyboard_address;
    unsigned long idt_address;
    unsigned long idt_ptr[2];

    // A IDT deve apontar para o stub em assembly que finaliza com IRETD,
    // e não diretamente para a função C.
    extern void keyboard_handler(void);
    keyboard_address = (unsigned long)keyboard_handler;
    IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
    IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[0x21].zero = 0;
    IDT[0x21].type_attr = INTERRUPT_GATE;
    IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

    // Inicializa PIC
    write_port(0x20, 0x11);
    write_port(0xA0, 0x11);
    write_port(0x21, 0x20);
    write_port(0xA1, 0x28);
    write_port(0x21, 0x00);
    write_port(0xA1, 0x00);
    write_port(0x21, 0x01);
    write_port(0xA1, 0x01);

    // Mascara todas as IRQs
    write_port(0x21, 0xFF);
    write_port(0xA1, 0xFF);

    // Carrega IDT
    idt_address = (unsigned long)IDT;
    idt_ptr[0] = (sizeof(struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
    idt_ptr[1] = idt_address >> 16;
    load_idt(idt_ptr);
}

// Inicializa teclado (desmascara apenas IRQ1)
void kb_init(void) {
    write_port(0x21, 0xFD);
}

// -----------------------------------------------------------------
// Funções de saída na tela
void kprint(const char *str) {
    unsigned int i = 0;
    while (str[i] != '\0') {
        vidptr[current_loc++] = str[i++];
        vidptr[current_loc++] = 0x07;
    }
}

void kprint_char_color(char c, unsigned char color) {
    vidptr[current_loc++] = c;
    vidptr[current_loc++] = color;
}

void kprint_newline(void) {
    unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS;
    current_loc = current_loc + (line_size - current_loc % line_size);
}

void clear_screen(void) {
    for(unsigned int i=0; i<SCREENSIZE; i+=2) {
        vidptr[i] = ' ';
        vidptr[i+1] = 0x07;
    }
    current_loc = 0;
}

// -----------------------------------------------------------------
// Funções do jogo
static unsigned char cmos_read(unsigned char reg) {
    write_port(0x70, reg);
    return (unsigned char)read_port(0x71);
}

static int bcd_to_bin(unsigned char v) {
    return (v & 0x0F) + ((v >> 4) * 10);
}

void escolher_palavra() {
    // Usa tempo do RTC (CMOS) para variar a palavra
    int sec = bcd_to_bin(cmos_read(0x00));
    int min = bcd_to_bin(cmos_read(0x02));
    int hour = bcd_to_bin(cmos_read(0x04));
    int seed = sec + min * 60 + hour * 3600;
    int indice = seed % NUM_PALAVRAS;
    for(int i=0;i<PALAVRA_TAM;i++)
        palavra_correta[i] = lista_palavras[indice][i];
    palavra_correta[PALAVRA_TAM] = '\0';
}

void inicializar() {
    escolher_palavra();
    clear_screen();
    kprint("Bem-vindo ao Forca Kernel!");
    kprint_newline();
    kprint("Digite uma palavra de 6 letras e pressione ENTER");
    kprint_newline();
    kprint("Cores: ROXO = letra certa, lugar certo");
    kprint_newline();
    kprint("       VERMELHO = letra certa, lugar errado");
    kprint_newline();
}

void mensagem_ganhou() {
    kprint_newline();
    kprint("VOCE VENCEU!!");
}

void mensagem_perdeu() {
    kprint_newline();
    kprint("VOCE PERDEU. PALAVRA: ");
    for(int i=0;i<PALAVRA_TAM;i++) {
        char s[2] = { palavra_correta[i], '\0' };
        kprint(s);
    }
}

void inserirLetra(char c) {
    if(game_over) return;
    if(letra_atual < PALAVRA_TAM) {
        tentativa[letra_atual++] = c;
        char str[2] = {c,'\0'};
        kprint(str);
    }
}

void apagarLetra() {
    if(letra_atual > 0) {
        letra_atual--;
        tentativa[letra_atual] = '\0';
        current_loc -= 2;
        vidptr[current_loc] = ' ';
        vidptr[current_loc+1] = 0x07;
    }
}

void verificarPalavra() {
    if(game_over) return;
    if(letra_atual != PALAVRA_TAM) return;

    int acertos = 0;
    unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS;
    current_loc = (current_loc / line_size) * line_size;

    int usadas_corretas[PALAVRA_TAM] = {0};
    int usadas_tentativa[PALAVRA_TAM] = {0};

    // Roxo (era Verde)
    for(int i=0;i<PALAVRA_TAM;i++) {
        if(tentativa[i]==palavra_correta[i]) {
            kprint_char_color(tentativa[i], 0x0D);
            usadas_corretas[i]=1;
            usadas_tentativa[i]=1;
            acertos++;
        } else {
            kprint_char_color(tentativa[i], 0x07);
        }
    }

    // Vermelho (era Amarelo)
    for(int i=0;i<PALAVRA_TAM;i++) {
        if(usadas_tentativa[i]) continue;
        for(int j=0;j<PALAVRA_TAM;j++) {
            if(!usadas_corretas[j] && tentativa[i]==palavra_correta[j]) {
                unsigned int pos = current_loc - (PALAVRA_TAM-i)*2;
                vidptr[pos]=tentativa[i];
                vidptr[pos+1]=0x0C;
                usadas_corretas[j]=1;
                usadas_tentativa[i]=1;
                break;
            }
        }
    }

    if(acertos==PALAVRA_TAM) {
        ganhou=1;
        game_over=1;
        kprint_newline();
        mensagem_ganhou();
        kprint_newline();
        kprint("Jogar novamente? Pressione R");
        kprint_newline();
        waiting_restart = 1;
    }

    kprint_newline();
    tentativas++;
    letra_atual=0;

    if(!ganhou && tentativas >= MAX_TENTATIVAS) {
        game_over = 1;
        kprint_newline();
        mensagem_perdeu();
        kprint_newline();
        kprint("Reinicie a maquina para jogar novamente.");
        kprint_newline();
    }
}

// -----------------------------------------------------------------
// Interrupção do teclado (chamada pelo bootloader)
void keyboard_handler_main(void) {
    unsigned char status;
    char keycode;

    write_port(0x20,0x20); // EOI PIC

    if(game_over && !waiting_restart) return;

    status = read_port(0x64);
    if(status & 0x01) {
        keycode = read_port(0x60);
        if(waiting_restart) {
            char letra_r = keyboard_map[(unsigned char)keycode];
            if(letra_r == 'r' || letra_r == 'R') {
                reiniciar_jogo();
            }
            return;
        }
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

void reiniciar_jogo() {
    tentativas = 0;
    letra_atual = 0;
    ganhou = 0;
    game_over = 0;
    waiting_restart = 0;
    tentativa[0] = '\0';
    escolher_palavra();
    clear_screen();
    kprint("Bem-vindo ao Forca Kernel!");
    kprint_newline();
    kprint("Digite uma palavra de 6 letras e pressione ENTER");
    kprint_newline();
    kprint("Cores: ROXO = letra certa, lugar certo");
    kprint_newline();
    kprint("       VERMELHO = letra certa, lugar errado");
    kprint_newline();
}

// -----------------------------------------------------------------
// Loop principal do kernel
void kmain(void) {
    clear_screen();
    idt_init();
    kb_init();
    asm volatile("sti"); // Habilita interrupções

    inicializar();

    while(1) {
        asm volatile("hlt");
    }
}
