# Microkernel - Jogo Forca

&emsp; Este read.me contém a documentação do desenvolvimento do jogo de forca em um microkernel para a ponderada da semana 7

### Como rodar
- Pré-requisitos (Linux): `nasm`, `gcc` com suporte 32-bit (`gcc-multilib`), `ld`, `qemu-system-i386`.

```
sudo apt update
sudo apt install nasm gcc-multilib qemu-system-x86 make -y
```
- Build e execução:

```
./run.sh
```

O script compila o bootloader em assembly e o kernel em C (ELF 32-bit), faz o link com `ld` e executa no QEMU.

### Arquitetura

- Bootloader (`src/bootloader.asm`): define multiboot, inicializa a pilha, exporta rotinas `read_port`, `write_port`, `load_idt` e o stub de interrupção `keyboard_handler` que encerra com `iretd`. Chama `kmain`.
- Linker script (`src/link.ld`): posiciona o binário em 0x0010_0000 (1 MiB) e organiza as seções `.text`, `.data`, `.bss`.
- Kernel (`src/kernel.c`):
  - Saída de vídeo: escrita direta no modo texto VGA em `0xb8000` (80x25). Funções `vga_print`, `vga_putc`, `vga_newline`, `vga_clear`.
  - Interrupções: IDT configurada em C, com entrada 0x21 para teclado apontando para o stub assembly. PIC (8259) inicializado e IRQ1 desmascarada. `sti` habilita interrupções.
  - Teclado: ISR lê status 0x64 e dados 0x60, mapeia scancodes com `keyboard_map.h`. Envia EOI para o PIC.
  - Jogo: palavras de 6 letras, 6 tentativas. Cores: Roxo = letra certa e posição certa; Vermelho = letra existe em outra posição; Cinza padrão caso contrário. Lógica de marcação é baseada em contagem de letras, compatível com duplicadas.
  - Semente/aleatoriedade: seleção da palavra usa tempo do RTC (CMOS) lendo registradores 0x00/0x02/0x04 para variar entre execuções e reinícios.
  - Fluxo: após vitória, aparece "Jogar novamente? Pressione R" e o jogo reinicia ao pressionar `R`.

Arquivos principais:
- `src/bootloader.asm`: boot e utilidades de porta/IDT e stub `keyboard_handler`.
- `src/kernel.c`: IDT/PIC/ISR, VGA, lógica do jogo.
- `src/keyboard_map.h`: tabela de scancodes → caracteres.
- `src/link.ld`: linker script.
- `run.sh`: build e execução no QEMU.

### Como jogar
- O jogo escolhe uma palavra de 6 letras.
- Digite as 6 letras e pressione ENTER para validar.
- Cores na linha tentada:
  - Roxo: letra correta no lugar correto.
  - Vermelho: letra presente na palavra, mas em outro lugar.
  - Cinza (padrão): letra ausente.
- Você tem 6 tentativas.
- Se vencer, pressione `R` para jogar novamente.


