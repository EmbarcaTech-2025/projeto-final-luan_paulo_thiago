# Módulo SD Card com Raspberry Pi Pico (W) usando Pico SDK

Este repositório demonstra como utilizar um **módulo SD Card** com a **Raspberry Pi Pico (ou Pico W)** utilizando o **Pico SDK** e a biblioteca **FatFs** para leitura e escrita em arquivos.

---

##  Visão Geral

O projeto contém dois arquivos principais:

- **`sd_card_.c`** → Código principal que inicializa o SD Card, cria/escreve em um arquivo `.txt` e depois lê o conteúdo de volta, enviando pela porta serial.
- **`hw_config.c`** → Arquivo de configuração de hardware, responsável por mapear os pinos SPI utilizados para comunicação com o cartão SD.
(lib\no-OS-FatFS-SD-SPI-RPi-Pico\FatFs_SPI\sd_driver\hw_config.c)

---

##  Requisitos

- **Raspberry Pi Pico ou Pico W**
- **Módulo MicroSD (com interface SPI)**
- **Pico SDK** configurado no ambiente de desenvolvimento
- **Biblioteca FatFs** (repositório indicado abaixo)

---

##  Conexões de Hardware

Este exemplo assume a seguinte configuração de pinos:

| Função | SPI0 | GPIO | Pico Pin | SD Card | Descrição                   |
|--------|------|------|----------|---------|-----------------------------|
| MISO   | RX   | 16   | 21       | DataOut | Master In, Slave Out        |
| MOSI   | TX   | 19   | 25       | DataIn  | Master Out, Slave In        |
| SCK    | SCK  | 18   | 24       | CLK     | Clock do barramento SPI     |
| CS     | CSn  | 17   | 22       | CS      | Chip Select do cartão       |
| GND    | -    | -    | GND      | GND     | Terra                       |
| 3V3    | -    | -    | 3V3      | 3.3V    | Alimentação                 |

---

##  Funcionamento do Código (`sd_card_.c`)

O programa segue os seguintes passos:

1. **Inicializa a porta serial** (`stdio_init_all`) para comunicação com o PC.
2. **Inicializa o driver do SD Card** com `sd_init_driver()`.
3. **Monta o sistema de arquivos** usando `f_mount`.
4. **Cria e abre um arquivo** (`test02.txt`) no modo escrita (`FA_WRITE | FA_CREATE_ALWAYS`).
5. **Escreve dados no arquivo** usando `f_printf`.
6. **Fecha o arquivo** com `f_close`.
7. **Reabre o arquivo em modo leitura** (`FA_READ`).
8. **Lê e imprime o conteúdo** linha por linha via serial (`f_gets`).
9. **Fecha novamente o arquivo**.
10. **Desmonta o sistema de arquivos** com `f_unmount`.
11. Entra em um **loop infinito**, exibindo mensagem de sucesso.

---

##  Configuração do SPI (`hw_config.c`)

O arquivo `hw_config.c` define a configuração do barramento SPI e do cartão SD:

- Estrutura `spi_t` → Define qual SPI usar (nesse caso `spi0`), pinos **MISO/MOSI/SCK**, e baud rate.
- Estrutura `sd_card_t` → Define cada cartão SD, associando-o ao SPI, pino **CS**, e pino de detecção de cartão.
- Funções auxiliares (`sd_get_by_num`, `spi_get_by_num`) permitem acessar os dispositivos configurados.

Exemplo da configuração usada:
```c
static spi_t spis[] = {
    {
        .hw_inst = spi0,
        .miso_gpio = 16,
        .mosi_gpio = 19,
        .sck_gpio  = 18,
        .baud_rate = 12500 * 1000
    }
};

static sd_card_t sd_cards[] = {
    {
        .pcName = "0:",
        .spi = &spis[0],
        .ss_gpio = 17,
        .use_card_detect = true,
        .card_detect_gpio = 22,
        .card_detected_true = 1
    }
};


Vídeo de referência: https://www.youtube.com/watch?v=u-vmsIr-s7w&t=391s

Repositório Biblioteca SD Card: https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico

