# Alvos principais
all: compilador assembler executor

# Regras para compilador
compilador: 
	gcc -o compilador compilador.c

# Regras para assembler
assembler: 
	gcc -o assembler assembler.c

# Regras para executor
executor: 
	gcc -o executor executor.c

# Limpar arquivos gerados
clean:
	rm -f compilador
	rm -f assembler
	rm -f executor
	rm -f programa.bin
	rm -f output.bin
	rm -f programa.asm

.PHONY: all compilador assembler executor clean
