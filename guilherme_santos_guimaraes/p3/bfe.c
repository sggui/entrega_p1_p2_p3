#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAPE_SIZE 30000
#define CODE_SIZE 65536

int main() {
    char tape[TAPE_SIZE] = {0};
    char code[CODE_SIZE];
    int jumps[CODE_SIZE] = {0};
    int stack[CODE_SIZE];
    int sp = 0;

    // Ler código da entrada padrão
    int code_len = fread(code, 1, CODE_SIZE, stdin);

    // Construir mapa de saltos
    for (int i = 0; i < code_len; i++) {
        if (code[i] == '[') {
            stack[sp++] = i;
        } else if (code[i] == ']') {
            if (sp == 0) {
                fprintf(stderr, "Erro: ']' sem par em %d\n", i);
                exit(1);
            }
            int j = stack[--sp];
            jumps[j] = i;
            jumps[i] = j;
        }
    }
    if (sp != 0) {
        fprintf(stderr, "Erro: '[' sem par\n");
        exit(1);
    }

    // Interpretar código
    int ptr = 0;
    for (int ip = 0; ip < code_len; ip++) {
        switch (code[ip]) {
            case '>':
                ptr = (ptr + 1) % TAPE_SIZE;
                break;
            case '<':
                ptr = (ptr - 1 + TAPE_SIZE) % TAPE_SIZE;
                break;
            case '+':
                tape[ptr]++;
                break;
            case '-':
                tape[ptr]--;
                break;
            case '.':
                putchar(tape[ptr]);
                break;
            case '[':
                if (tape[ptr] == 0) {
                    ip = jumps[ip];
                }
                break;
            case ']':
                if (tape[ptr] != 0) {
                    ip = jumps[ip];
                }
                break;
        }
    }

    putchar('\n');
    return 0;
}

