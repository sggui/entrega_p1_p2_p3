
## Como compilar

Compile ambos com `gcc`:

```bash
gcc -o bfc bfc.c
gcc -o bfe bfe.c
```

---

## Como usar

Recebe pela entrada padrão uma expressão no formato:

```
VAR=EXPR
```

Exemplo de entrada:

```
X=3+5*2
```

## Importante
A expressão/variável não deve ter espaço, assim como listado acima.
Caso tenha, o resultado dará errado/diferente do esperado.

Saída:

Um código em **Brainfuck** que imprime:

```
X=13
```

---

### **bfe** — Interpretador de Brainfuck

Executa um código Brainfuck vindo da entrada padrão.

---

## Exemplo completo

Compilar uma expressão e executar:

```bash
echo "SOMA=4+5*2" | ./bfc | ./bfe
```

Saída:

```
SOMA=14
```

---

## Como testar separado

Gerar o código Brainfuck:

```bash
echo "X=7+8*3" | ./bfc
```

Rodar esse código manualmente no interpretador:

```bash
echo "[CÓDIGO_BRAINFUCK]" | ./bfe
```

---

## Expressões suportadas

- Operadores: `+`, `-`, `*`, `/`
- Parênteses para agrupamento: `( )`

Exemplos válidos:

- `A=5+3`
- `B=(2+4)*3`
- `C=10-2*4`
- `D=10/2+(2*-3)`

---

## Limitações

- Só trabalha com números inteiros.
- Não tem log de erros, porém, roda conforme descrito na seção de "Como usar"

---
