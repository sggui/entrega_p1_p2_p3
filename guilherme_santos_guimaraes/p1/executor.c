#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MEMORYSIZE 516
#define LINESIZE 16
#define HEADERSIZE 4

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo_bin>\n", argv[0]);
        return 1;
    }

    uint8_t ac = 0, pc = 0;
    bool z = false, n = false;

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Não foi possível abrir o arquivo binário");
        return 1;
    }

    uint8_t bytes[MEMORYSIZE] = {0};
    uint8_t header[HEADERSIZE];

    fread(header, 1, HEADERSIZE, file);
    const uint8_t expected[] = {0x03, 0x4E, 0x44, 0x52};

    if (memcmp(header, expected, HEADERSIZE) != 0) {
        printf("Cabeçalho inválido!\n");
        fclose(file);
        return 1;
    }

    fread(bytes + HEADERSIZE, 1, MEMORYSIZE - HEADERSIZE, file);
    fclose(file);

    while (bytes[pc] != 0xF0) {
        z = (ac == 0);
        n = (ac & 0x80) != 0;
        uint16_t addr = bytes[pc + 2] * 2 + HEADERSIZE;

        switch (bytes[pc]) {
            case 0x00: break;                       // NOP
            case 0x10: bytes[addr] = ac; break;     // STA
            case 0x20: ac = bytes[addr]; break;     // LDA
            case 0x30: ac += bytes[addr]; break;    // ADD
            case 0x31: ac -= bytes[addr]; break;    // SUB
            case 0x40: ac |= bytes[addr]; break;    // OR
            case 0x50: ac &= bytes[addr]; break;    // AND
            case 0x60: ac = ~ac; pc += 2; continue; // NOT
            case 0x80: pc = addr; continue;         // JMP
            case 0x90: if (n) { pc = addr; continue; } break; // JMN
            case 0xA0: if (z) { pc = addr; continue; } break; // JMZ
            case 0xF0: break; // HLT
        }

        pc += 4;
    }

    // Localiza resultado na memória
    int found = 0;
    int resultAddress = -1;
    for (int i = HEADERSIZE; i < MEMORYSIZE; i += 2) {
        if (bytes[i] == ac) {
            resultAddress = i;
            found = 1;
            break;
        }
    }

    if (found) {
        uint8_t raw = bytes[resultAddress];
        int8_t signed_val = (int8_t)raw;

        printf("Conta final (hexa) = 0x%02X\n", raw);
        printf("Conta final (decimal) = %d\n", signed_val);
    } else {
        printf("Conta final (hexa) = ERRO\n");
    }

    return 0;
}

