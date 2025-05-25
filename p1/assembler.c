#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#define MEM_SIZE 512
#define HEADER_SIZE 4
#define VAR_START 0x100
#define MAX_TAGS 256
#define MAX_LINE 256

// OpCodes
#define OP_NOP  0x00
#define OP_STA  0x10
#define OP_LDA  0x20
#define OP_ADD  0x30
#define OP_SUB  0x31
#define OP_OR   0x40
#define OP_AND  0x50
#define OP_NOT  0x60
#define OP_JMP  0x80
#define OP_JMN  0x90
#define OP_JMZ  0xA0
#define OP_HLT  0xF0

typedef struct {
    char name[32];
    int address;
    bool defined;
} Symbol;

Symbol symbol_table[MAX_TAGS];
int symbol_count = 0;
uint8_t memory[MEM_SIZE] = {0};
int pc = HEADER_SIZE;
int var_ptr = VAR_START;

void add_symbol(const char* name, int address, bool defined) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0)
            return; // Já existe
    }
    strncpy(symbol_table[symbol_count].name, name, sizeof(symbol_table[symbol_count].name));
    symbol_table[symbol_count].address = address;
    symbol_table[symbol_count].defined = defined;
    symbol_count++;
}

int get_symbol_address(const char* name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0)
            return symbol_table[i].address;
    }

    // Se não existe, cria nova entrada
    if (var_ptr % 2 != 0) var_ptr++;  // alinhamento
    int addr = var_ptr;
    add_symbol(name, addr, false);
    var_ptr += 2;
    return addr;
}

uint8_t get_opcode(const char* mnemonic) {
    if (strcasecmp(mnemonic, "NOP") == 0) return OP_NOP;
    if (strcasecmp(mnemonic, "STA") == 0) return OP_STA;
    if (strcasecmp(mnemonic, "LDA") == 0) return OP_LDA;
    if (strcasecmp(mnemonic, "ADD") == 0) return OP_ADD;
    if (strcasecmp(mnemonic, "SUB") == 0) return OP_SUB;
    if (strcasecmp(mnemonic, "OR")  == 0) return OP_OR;
    if (strcasecmp(mnemonic, "AND") == 0) return OP_AND;
    if (strcasecmp(mnemonic, "NOT") == 0) return OP_NOT;
    if (strcasecmp(mnemonic, "JMP") == 0) return OP_JMP;
    if (strcasecmp(mnemonic, "JMN") == 0) return OP_JMN;
    if (strcasecmp(mnemonic, "JMZ") == 0) return OP_JMZ;
    if (strcasecmp(mnemonic, "HLT") == 0) return OP_HLT;
    return 0xFF;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s programa.asm programa.bin\n", argv[0]);
        return 1;
    }

    FILE* src = fopen(argv[1], "r");
    if (!src) {
        perror("Erro ao abrir arquivo ASM");
        return 1;
    }

    FILE* out = fopen(argv[2], "wb");
    if (!out) {
        perror("Erro ao criar arquivo BIN");
        fclose(src);
        return 1;
    }

    // Cabeçalho NDR
    memory[0] = 0x03;
    memory[1] = 'N';
    memory[2] = 'D';
    memory[3] = 'R';

    char line[MAX_LINE];
    bool in_data = false, in_code = false;

    while (fgets(line, sizeof(line), src)) {
        char* token = strtok(line, " \t\r\n");
        if (!token || token[0] == ';') continue;

        if (strcasecmp(token, ".DATA") == 0) {
            in_data = true;
            in_code = false;
            continue;
        } else if (strcasecmp(token, ".CODE") == 0) {
            in_code = true;
            in_data = false;
            continue;
        } else if (strcasecmp(token, ".ORG") == 0) {
            strtok(NULL, " \t\r\n"); // Ignora valor
            continue;
        }

        if (in_data) {
            char* name = token;
            strtok(NULL, " \t\r\n"); // DB
            char* val = strtok(NULL, " \t\r\n");
            int value = (val && strcmp(val, "?") != 0) ? atoi(val) : 0;
            if (var_ptr % 2 != 0) var_ptr++; // alinhamento
            add_symbol(name, var_ptr, true);
            memory[var_ptr++] = value;
            memory[var_ptr++] = 0x00;
        } else if (in_code) {
            if (strchr(token, ':')) continue; // Ignora labels

            uint8_t opcode = get_opcode(token);
            if (opcode == 0xFF) continue;

            if (opcode == OP_HLT || opcode == OP_NOP || opcode == OP_NOT) {
                memory[pc++] = opcode;
                memory[pc++] = 0x00;
                memory[pc++] = 0x00;
                memory[pc++] = 0x00;
            } else {
                char* arg = strtok(NULL, " \t\r\n");
                int addr = get_symbol_address(arg);
                uint8_t addr_word = (addr - HEADER_SIZE) / 2;

                memory[pc++] = opcode;
                memory[pc++] = 0x00;
                memory[pc++] = addr_word;
                memory[pc++] = 0x00;
            }
        }
    }

    // Garante que termina com HLT
    if (memory[pc - 4] != OP_HLT) {
        memory[pc++] = OP_HLT;
        memory[pc++] = 0x00;
        memory[pc++] = 0x00;
        memory[pc++] = 0x00;
    }

    fwrite(memory, 1, MEM_SIZE, out);
    fclose(src);
    fclose(out);
    printf("(successful) Binário gerado com sucesso!\n");
    return 0;
}

