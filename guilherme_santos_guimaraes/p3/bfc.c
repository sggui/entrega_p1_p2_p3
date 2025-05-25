#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CODE 100000

// —————————————————— AST —————————————————————
typedef struct Node Node;
struct Node {
    enum { NUMBER, BINOP } kind;
    union {
        int val; // NUMBER
        struct {
            char op;
            Node *left, *right;
        }; // BINOP
    };
};

// Declaração antecipada da função de avaliação
int evalNode(Node *n);

// ——————— Parser —————————
typedef struct {
    const char *s;
    int pos;
} Parser;

char peek(Parser *p) {
    return p->s[p->pos];
}

char consume(Parser *p) {
    char ch = peek(p);
    if (ch) p->pos++;
    return ch;
}

Node *parseExpr(Parser *p);
Node *parseTerm(Parser *p);
Node *parseFactor(Parser *p);

Node *parseExpr(Parser *p) {
    Node *node = parseTerm(p);
    while (peek(p) == '+' || peek(p) == '-') {
        char op = consume(p);
        Node *right = parseTerm(p);
        Node *n = malloc(sizeof(Node));
        n->kind = BINOP;
        n->op = op;
        n->left = node;
        n->right = right;
        node = n;
    }
    return node;
}

Node *parseTerm(Parser *p) {
    Node *node = parseFactor(p);
    while (peek(p) == '*' || peek(p) == '/') {  // Adicionado suporte para divisão
        char op = consume(p);
        Node *right = parseFactor(p);
        Node *n = malloc(sizeof(Node));
        n->kind = BINOP;
        n->op = op;
        n->left = node;
        n->right = right;
        node = n;
    }
    return node;
}

Node *parseFactor(Parser *p) {
    int negative = 0;
    if (peek(p) == '-') {
        consume(p);
        negative = 1;
    }

    if (peek(p) == '(') {
        consume(p);
        Node *node = parseExpr(p);
        if (peek(p) == ')') consume(p);
        return negative ? 
            &(Node){ .kind = BINOP, .op = '-', .left = &(Node){.kind=NUMBER, .val=0}, .right=node } : 
            node;
    }

    int start = p->pos;
    while (isdigit(peek(p))) consume(p);
    if (start == p->pos) {
        fprintf(stderr, "Erro: número esperado\n");
        exit(1);
    }
    int val = atoi(p->s + start);
    if (negative) val = -val;

    Node *n = malloc(sizeof(Node));
    n->kind = NUMBER;
    n->val = val;
    return n;
}

// ——————————————— Brainfuck Generator —————————————————
typedef struct {
    char code[MAX_CODE];
    int len;
    int pos;
} BFGen;

void emit(BFGen *g, const char *s) {
    int l = strlen(s);
    if (g->len + l >= MAX_CODE) {
        fprintf(stderr, "Código Brainfuck muito grande!\n");
        exit(1);
    }
    strcpy(g->code + g->len, s);
    g->len += l;
}

void moveTo(BFGen *g, int c) {
    while (g->pos < c) {
        emit(g, ">");
        g->pos++;
    }
    while (g->pos > c) {
        emit(g, "<");
        g->pos--;
    }
}

void zero(BFGen *g) {
    emit(g, "[-]");
}

void inc(BFGen *g, int n) {
    for (int i = 0; i < n; i++) emit(g, "+");
}

void emitLoop(BFGen *g, int c, void (*body)(BFGen *, void *), void *arg) {
    moveTo(g, c);
    emit(g, "[");
    body(g, arg);
    moveTo(g, c);
    emit(g, "]");
}

typedef struct { int src, dst; } AddArgs;
void loopAdd(BFGen *g, void *arg) {
    AddArgs *a = arg;
    emit(g, "-");
    moveTo(g, a->dst);
    emit(g, "+");
    moveTo(g, a->src);
}

void emitAdd(BFGen *g, int src, int dst) {
    AddArgs a = {src, dst};
    emitLoop(g, src, loopAdd, &a);
    moveTo(g, dst);
}

typedef struct { int src, dst; } SubArgs;
void loopSub(BFGen *g, void *arg) {
    SubArgs *a = arg;
    emit(g, "-");
    moveTo(g, a->dst);
    emit(g, "-");
    moveTo(g, a->src);
}

void emitSub(BFGen *g, int src, int dst) {
    SubArgs a = {src, dst};
    emitLoop(g, src, loopSub, &a);
    moveTo(g, dst);
}

typedef struct { int a, b, res, tmp; } MulArgs;
void loopInnerMul(BFGen *g, void *arg) {
    MulArgs *a = arg;
    emit(g, "-");
    moveTo(g, a->res);
    emit(g, "+");
    moveTo(g, a->tmp);
    emit(g, "+");
    moveTo(g, a->b);
}

void loopRestoreB(BFGen *g, void *arg) {
    MulArgs *a = arg;
    emit(g, "-");
    moveTo(g, a->b);
    emit(g, "+");
    moveTo(g, a->tmp);
}

void loopOuterMul(BFGen *g, void *arg) {
    MulArgs *a = arg;
    emit(g, "-");
    emitLoop(g, a->b, loopInnerMul, a);
    emitLoop(g, a->tmp, loopRestoreB, a);
}

void emitMul(BFGen *g, int a, int b, int res, int tmp) {
    moveTo(g, res); zero(g);
    moveTo(g, tmp); zero(g);
    emitLoop(g, a, loopOuterMul, &(MulArgs){a, b, res, tmp});
    emitLoop(g, res, loopRestoreB, &(MulArgs){a, b, res, tmp});
    moveTo(g, a);
}

// ————————— Implementação da Divisão —————————
typedef struct { int dividend, divisor, quotient, remainder, tmp1, tmp2; } DivArgs;

// Loop interno para subtrair divisor do dividendo
void loopDivSubtract(BFGen *g, void *arg) {
    DivArgs *a = arg;
    emit(g, "-");
    moveTo(g, a->dividend);
    emit(g, "-");
    moveTo(g, a->tmp1);
    emit(g, "+");
    moveTo(g, a->divisor);
}

// Loop para restaurar divisor
void loopRestoreDivisor(BFGen *g, void *arg) {
    DivArgs *a = arg;
    emit(g, "-");
    moveTo(g, a->divisor);
    emit(g, "+");
    moveTo(g, a->tmp1);
}

// Loop principal da divisão
void loopDivMain(BFGen *g, void *arg) {
    DivArgs *a = arg;
    
    // Copiar divisor para tmp2 para comparação
    moveTo(g, a->tmp2); zero(g);
    emitLoop(g, a->divisor, loopDivSubtract, a);
    emitLoop(g, a->tmp1, loopRestoreDivisor, a);
    
    // Se dividend >= divisor, subtrair e incrementar quociente  
    moveTo(g, a->dividend);
    emit(g, "[");
    moveTo(g, a->tmp2);
    emit(g, "[");
    emit(g, "-");
    moveTo(g, a->dividend);
    emit(g, "-");
    moveTo(g, a->tmp2);
    emit(g, "]");
    moveTo(g, a->dividend);
    emit(g, "[");
    moveTo(g, a->quotient);
    emit(g, "+");
    moveTo(g, a->dividend);
    emit(g, "[-]]");
    emit(g, "]");
}

void emitDiv(BFGen *g, int dividend, int divisor, int quotient, int remainder, int tmp1, int tmp2) {
    // Verificar divisão por zero
    moveTo(g, divisor);
    emit(g, "[");
    
    // Zerar resultado e temporários
    moveTo(g, quotient); zero(g);
    moveTo(g, remainder); zero(g);
    moveTo(g, tmp1); zero(g);
    moveTo(g, tmp2); zero(g);
    
    // Algoritmo de divisão por subtração sucessiva
    moveTo(g, dividend);
    emit(g, "[");
    
    // Copiar divisor para tmp2
    moveTo(g, tmp2); zero(g);
    emitLoop(g, divisor, loopAdd, &(AddArgs){divisor, tmp2});
    
    // Verificar se dividend >= divisor
    moveTo(g, dividend);
    emit(g, "[");
    moveTo(g, tmp2);
    emit(g, "[");
    emit(g, "-");
    moveTo(g, dividend);
    emit(g, "-");
    moveTo(g, tmp2);
    emit(g, "]");
    
    // Se dividend > 0, incrementar quociente
    moveTo(g, dividend);
    emit(g, "[");
    moveTo(g, quotient);
    emit(g, "+");
    moveTo(g, dividend);
    emit(g, "[-]]");
    
    // Se tmp2 > 0, dividend < divisor, então parar
    moveTo(g, tmp2);
    emit(g, "[");
    moveTo(g, remainder);
    emitAdd(g, dividend, remainder);
    moveTo(g, dividend);
    zero(g);
    moveTo(g, tmp2);
    zero(g);
    emit(g, "]");
    
    moveTo(g, dividend);
    emit(g, "]");
    
    emit(g, "]");
    
    // Se divisor era 0, setar resultado para 0
    moveTo(g, divisor);
    emit(g, "]");
    emit(g, "[");
    moveTo(g, quotient);
    zero(g);
    moveTo(g, divisor);
    emit(g, "[-]]");
    
    moveTo(g, quotient);
}

// —————————— Node Generation —————————
void genNode(Node *n, BFGen *g, int cell) {
    if (n->kind == NUMBER) {
        moveTo(g, cell);
        zero(g);
        if (n->val >= 0) {
            inc(g, n->val);
        } else {
            // Para números negativos, usar complemento
            inc(g, 256 + n->val);
        }
    } else {
        if (n->op == '+') {
            genNode(n->left, g, cell);
            genNode(n->right, g, cell + 1);
            emitAdd(g, cell + 1, cell);
        } else if (n->op == '-') {
            genNode(n->left, g, cell);
            genNode(n->right, g, cell + 1);
            emitSub(g, cell + 1, cell);
        } else if (n->op == '*') {
            genNode(n->left, g, cell);
            genNode(n->right, g, cell + 1);
            emitMul(g, cell, cell + 1, cell + 2, cell + 3);
            // Mover resultado para cell
            moveTo(g, cell); zero(g);
            emitAdd(g, cell + 2, cell);
        } else if (n->op == '/') {  // Novo caso para divisão
            genNode(n->left, g, cell);      // dividendo
            genNode(n->right, g, cell + 1); // divisor
            emitDiv(g, cell, cell + 1, cell + 2, cell + 3, cell + 4, cell + 5);
            // Mover resultado (quociente) para cell
            moveTo(g, cell); zero(g);
            emitAdd(g, cell + 2, cell);
        }
    }
}

// ————————— Avaliação da AST ———————————
int evalNode(Node *n) {
    if (n->kind == NUMBER) return n->val;
    int l = evalNode(n->left);
    int r = evalNode(n->right);
    switch (n->op) {
        case '+': return l + r;
        case '-': return l - r;
        case '*': return l * r;
        case '/': return r != 0 ? l / r : 0;  // Adicionado caso para divisão
    }
    return 0;
}

// —————————————— Main —————————————————————
int main() {
    char line[1024];
    if (!fgets(line, sizeof(line), stdin)) {
        fprintf(stderr, "Erro de leitura\n");
        return 1;
    }

    char *eq = strchr(line, '=');
    if (!eq) {
        fprintf(stderr, "Uso: VAR=EXPR\n");
        return 1;
    }
    *eq = '\0';
    char *varName = line;
    char *expr = eq + 1;
    while (isspace(*expr)) expr++;

    Parser p = {expr, 0};
    Node *ast = parseExpr(&p);

    BFGen g = {{0}, 0, 0};

    // Gerar nome da variável (UTF-8 byte a byte)
    for (unsigned char *c = (unsigned char *)varName; *c; c++) {
        moveTo(&g, 10);
        zero(&g);
        inc(&g, *c);
        emit(&g, ".");
    }
    moveTo(&g, 10);
    zero(&g);
    inc(&g, '=');
    emit(&g, ".");

    // Gerar expressão
    genNode(ast, &g, 0);

    // Gerar resultado numérico como string
    char buf[32];
    sprintf(buf, "%d", evalNode(ast));
    for (unsigned char *c = (unsigned char *)buf; *c; c++) {
        moveTo(&g, 10);
        zero(&g);
        inc(&g, *c);
        emit(&g, ".");
    }

    g.code[g.len] = '\0';
    printf("%s\n", g.code);

    return 0;
}
