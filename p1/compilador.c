#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>  // For boolean support

/*========================================================================
  Lexical Section - Token Processing
========================================================================*/
typedef enum {
    TK_START,
    TK_BEGIN,
    TK_FINISH,
    TK_RESULT,
    TK_NAME,
    TK_NUMBER,
    TK_ASSIGN,
    TK_ADD,
    TK_SUBTRACT,
    TK_MULTIPLY,
    TK_DIVIDE,
    TK_OPEN_BRACKET,
    TK_CLOSE_BRACKET,
    TK_DELIMITER,
    TK_END_OF_FILE,
    TK_INVALID
} LexicalType;

typedef struct {
    LexicalType category;
    char text[64];
} LexicalToken;

#define TOKEN_CAPACITY 1024
LexicalToken tokenArray[TOKEN_CAPACITY];
int tokenTotal = 0;
int currentIndex = 0;

char* inputCode;
int inputPosition = 0;

void ignoreSpaces() {
    while (inputCode[inputPosition] == ' ' || inputCode[inputPosition] == '\t')
        inputPosition++;
}

int isAlpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int isNumeric(char c) {
    return (c >= '0' && c <= '9');
}

void insertToken(LexicalType category, const char *text) {
    if (tokenTotal < TOKEN_CAPACITY) {
        tokenArray[tokenTotal].category = category;
        strncpy(tokenArray[tokenTotal].text, text, sizeof(tokenArray[tokenTotal].text)-1);
        tokenArray[tokenTotal].text[sizeof(tokenArray[tokenTotal].text)-1] = '\0';
        tokenTotal++;
    }
}

void scanTokens() {
    tokenTotal = 0;
    currentIndex = 0;  // Reset parsing index
    inputPosition = 0;
    
    while (inputCode[inputPosition] != '\0') {
        if (inputCode[inputPosition] == ' ' || inputCode[inputPosition] == '\t') {
            ignoreSpaces();
            continue;
        }
        
        if (inputCode[inputPosition] == '\n' || inputCode[inputPosition] == '\r') {
            // Skip line breaks
            inputPosition++;
            continue;
        }
        
        // Check keywords
        if (strncmp(&inputCode[inputPosition], "PROGRAMA", 8) == 0 && !isAlpha(inputCode[inputPosition+8])) {
            insertToken(TK_START, "PROGRAMA");
            inputPosition += 8;
            continue;
        }
        
        if (strncmp(&inputCode[inputPosition], "INICIO", 6) == 0 && !isAlpha(inputCode[inputPosition+6])) {
            insertToken(TK_BEGIN, "INICIO");
            inputPosition += 6;
            continue;
        }
        
        if (strncmp(&inputCode[inputPosition], "FIM", 3) == 0 && !isAlpha(inputCode[inputPosition+3])) {
            insertToken(TK_FINISH, "FIM");
            inputPosition += 3;
            continue;
        }
        
        if (strncmp(&inputCode[inputPosition], "RES", 3) == 0 && !isAlpha(inputCode[inputPosition+3])) {
            insertToken(TK_RESULT, "RES");
            inputPosition += 3;
            continue;
        }
        
        // Check operators and symbols
        if (inputCode[inputPosition] == '=') {
            insertToken(TK_ASSIGN, "=");
            inputPosition++;
            continue;
        }
        
        if (inputCode[inputPosition] == '+') {
            insertToken(TK_ADD, "+");
            inputPosition++;
            continue;
        }
        
        if (inputCode[inputPosition] == '-') {
            insertToken(TK_SUBTRACT, "-");
            inputPosition++;
            continue;
        }
        
        if (inputCode[inputPosition] == '*') {
            insertToken(TK_MULTIPLY, "*");
            inputPosition++;
            continue;
        }
        
        if (inputCode[inputPosition] == '/') {
            insertToken(TK_DIVIDE, "/");
            inputPosition++;
            continue;
        }
        
        if (inputCode[inputPosition] == '(') {
            insertToken(TK_OPEN_BRACKET, "(");
            inputPosition++;
            continue;
        }
        
        if (inputCode[inputPosition] == ')') {
            insertToken(TK_CLOSE_BRACKET, ")");
            inputPosition++;
            continue;
        }
        
        if (inputCode[inputPosition] == ':') {
            insertToken(TK_DELIMITER, ":");
            inputPosition++;
            continue;
        }
        
        // Handle quoted string (program name)
        if (inputCode[inputPosition] == '\"') {
            inputPosition++; // Skip opening quote
            int startPos = inputPosition;
            while (inputCode[inputPosition] != '\"' && inputCode[inputPosition] != '\0')
                inputPosition++;
            
            int length = inputPosition - startPos;
            char buffer[64];
            strncpy(buffer, &inputCode[startPos], length);
            buffer[length] = '\0';
            insertToken(TK_NAME, buffer);
            
            if (inputCode[inputPosition] == '\"')
                inputPosition++; // Skip closing quote
            continue;
        }
        
        // Handle identifiers
        if (isAlpha(inputCode[inputPosition])) {
            int startPos = inputPosition;
            while (isAlpha(inputCode[inputPosition]) || isNumeric(inputCode[inputPosition]) || inputCode[inputPosition]=='_')
                inputPosition++;
            
            int length = inputPosition - startPos;
            char buffer[64];
            strncpy(buffer, &inputCode[startPos], length);
            buffer[length] = '\0';
            insertToken(TK_NAME, buffer);
            continue;
        }
        
        // Handle numbers
        if (isNumeric(inputCode[inputPosition])) {
            int startPos = inputPosition;
            while (isNumeric(inputCode[inputPosition]))
                inputPosition++;
            
            int length = inputPosition - startPos;
            char buffer[64];
            strncpy(buffer, &inputCode[startPos], length);
            buffer[length] = '\0';
            insertToken(TK_NUMBER, buffer);
            continue;
        }
        
        // Skip unrecognized character
        inputPosition++;
    }
    
    insertToken(TK_END_OF_FILE, "EOF");

    for (int i = 0; i < tokenTotal; i++) {
        printf("Token[%d]:  Text='%s'\n", i, tokenArray[i].text);
    }
}

LexicalToken* peekNextToken() {
    if (currentIndex < tokenTotal)
        return &tokenArray[currentIndex];
    return NULL;
}

LexicalToken* consumeToken() {
    if (currentIndex < tokenTotal)
        return &tokenArray[currentIndex++];
    return NULL;
}

/*========================================================================
  Abstract Syntax Tree (AST) Construction
========================================================================*/
typedef enum { NODE_LITERAL, NODE_IDENTIFIER, NODE_OPERATION } NodeCategory;

typedef struct SyntaxNode {
    NodeCategory category;
    union {
        int value;              // for numbers
        char identifier[64];    // for variables
        struct {
            char operator;     // '+', '-', '*', '/'
            struct SyntaxNode *leftChild;
            struct SyntaxNode *rightChild;
        } operation;
    };
} SyntaxNode;

SyntaxNode* createLiteralNode(int value) {
    SyntaxNode* node = malloc(sizeof(SyntaxNode));
    node->category = NODE_LITERAL;
    node->value = value;
    return node;
}

SyntaxNode* createIdentifierNode(const char* name) {
    SyntaxNode* node = malloc(sizeof(SyntaxNode));
    node->category = NODE_IDENTIFIER;
    strncpy(node->identifier, name, sizeof(node->identifier)-1);
    node->identifier[sizeof(node->identifier)-1] = '\0';
    return node;
}

SyntaxNode* createOperationNode(char operator, SyntaxNode* leftChild, SyntaxNode* rightChild) {
    SyntaxNode* node = malloc(sizeof(SyntaxNode));
    node->category = NODE_OPERATION;
    node->operation.operator = operator;
    node->operation.leftChild = leftChild;
    node->operation.rightChild = rightChild;
    return node;
}

/* Recursive parsing functions for expressions */
SyntaxNode* parseExpression();
SyntaxNode* parseTerm();
SyntaxNode* parseFactor();

SyntaxNode* parseExpression() {
    SyntaxNode* node = parseTerm();
    LexicalToken* token;
    
    while ((token = peekNextToken()) && (token->category == TK_ADD || token->category == TK_SUBTRACT)) {
        token = consumeToken(); // consume the operator
        SyntaxNode* rightNode = parseTerm();
        node = createOperationNode(token->text[0], node, rightNode);
    }
    return node;
}

SyntaxNode* parseTerm() {
    SyntaxNode* node = parseFactor();
    LexicalToken* token;
    
    while ((token = peekNextToken()) && (token->category == TK_MULTIPLY || token->category == TK_DIVIDE)) {
        token = consumeToken(); // consume the operator
        SyntaxNode* rightNode = parseFactor();
        node = createOperationNode(token->text[0], node, rightNode);
    }
    return node;
}

SyntaxNode* parseFactor() {
    LexicalToken* token = peekNextToken();
    if (!token) return NULL;
    
    if (token->category == TK_OPEN_BRACKET) {
        consumeToken(); // consume '('
        SyntaxNode* node = parseExpression();
        consumeToken(); // consume ')'
        return node;
    }
    
    if (token->category == TK_NUMBER) {
        token = consumeToken();
        return createLiteralNode(atoi(token->text));
    }
    
    if (token->category == TK_NAME) {
        token = consumeToken();
        return createIdentifierNode(token->text);
    }
    
    return NULL;
}

/*========================================================================
  Statement Representation
========================================================================*/
typedef struct Command {
    char variable[64];
    SyntaxNode* expression;
    struct Command* next;
} Command;

Command* commandList = NULL;
Command* lastCommand = NULL;

void parseAssignmentStmt() {
    LexicalToken* token = consumeToken(); // expect identifier
    if (!token || token->category != TK_NAME) {
        printf("Warning: Esperado identificador\n");
        return;
    }
    
    char varName[64];
    strncpy(varName, token->text, sizeof(varName));
    
    LexicalToken* equals = consumeToken(); // expect '='
    if (!equals || equals->category != TK_ASSIGN) {
        printf("Esperado '='\n");
        return;
    }
    
    SyntaxNode* expr = parseExpression();
    
    Command* cmd = malloc(sizeof(Command));
    strncpy(cmd->variable, varName, sizeof(cmd->variable));
    cmd->expression = expr;
    cmd->next = NULL;
    
    if (commandList == NULL) {
        commandList = cmd;
        lastCommand = cmd;
    } else {
        lastCommand->next = cmd;
        lastCommand = cmd;
    }
    
}

/* Program structure representation */
typedef struct {
    char title[64];
    Command* commands;
    SyntaxNode* output;
} CompilationUnit;

CompilationUnit program;

void freeNode(SyntaxNode* node) {
    if (!node) return;
    if (node->category == NODE_OPERATION) {
        freeNode(node->operation.leftChild);
        freeNode(node->operation.rightChild);
    }
    free(node);
}

void freeCommands(Command* cmd) {
    while (cmd) {
        Command* next = cmd->next;
        freeNode(cmd->expression);
        free(cmd);
        cmd = next;
    }
}

/*========================================================================
  Program Parsing (according to grammar)
  
  Format:
    PROGRAMA "Name" :
    INICIO
      <assignments>
      RES = <expression>
    FIM
========================================================================*/
void parseProgram() {
    LexicalToken* token = consumeToken(); // Should be PROGRAMA
    if (!token || token->category != TK_START) { 
        printf("Erro: Esperado PROGRAMA no cabeçalho (consultar gramatica.pdf)\n"); 
        exit(1); 
    }
    
    token = consumeToken(); // Program name (in quotes)
    if (!token || token->category != TK_NAME) { 
        printf("Erro: Esperado NOME DO PROGRAMA no cabeçalho (consultar gramatica.pdf)\n"); 
        exit(1); 
    }
    strncpy(program.title, token->text, sizeof(program.title));
    
    token = consumeToken(); // Should be ":"
    if (!token || token->category != TK_DELIMITER) { 
        exit(1); 
    }
    
    token = consumeToken(); // Should be INICIO
    if (!token || token->category != TK_BEGIN) { 
        printf("Erro: Esperado INICIO no código (consultar gramatica.pdf)\n"); 
        exit(1); 
    }
    
    // Process assignments until we find RES
    while (1) {
        token = peekNextToken();
        if (!token) break;
        if (token->category == TK_RESULT)
            break;
        parseAssignmentStmt();
    }
    
    token = consumeToken(); // Should be RES
    if (!token || token->category != TK_RESULT) { 
        printf("Erro: Esperado RES para armazenar o retorno do programa (consultar gramatica.pdf)\n"); 
        exit(1); 
    }
    
    token = consumeToken(); // Should be '='
    if (!token || token->category != TK_ASSIGN) { 
        printf("Erro: Esperado '=' após RES (consultar gramatica.pdf)\n"); 
        exit(1); 
    }
    
    program.output = parseExpression();
    
    token = consumeToken(); // Should be FIM
    if (!token || token->category != TK_FINISH) { 
        printf("Erro: Esperado FIM para finalizar o programa (consultar gramatica.pdf)\n"); 
        exit(1); 
    }
}

/*========================================================================
  Symbol Table for Assembly Generation
========================================================================*/
typedef struct {
    char identifier[64];
    int data;
    bool initialized;
} Symbol;

#define SYMTABLE_SIZE 256
Symbol symbolTable[SYMTABLE_SIZE];
int symbolCount = 0;

int addSymbol(const char* identifier) {
    // Check if symbol already exists
    for (int i = 0; i < symbolCount; i++) {
        if (strcmp(symbolTable[i].identifier, identifier) == 0)
            return i;
    }
    
    // Add new symbol
    strncpy(symbolTable[symbolCount].identifier, identifier, sizeof(symbolTable[symbolCount].identifier)-1);
    symbolTable[symbolCount].identifier[sizeof(symbolTable[symbolCount].identifier)-1] = '\0';
    symbolTable[symbolCount].data = 0;
    symbolTable[symbolCount].initialized = false;
    symbolCount++;
    return symbolCount - 1;
}

/* Update symbol value if expression is a literal */
void updateSymbolValue(const char* identifier, int value) {
    for (int i = 0; i < symbolCount; i++) {
        if (strcmp(symbolTable[i].identifier, identifier) == 0) {
            symbolTable[i].data = value;
            symbolTable[i].initialized = true;
            return;
        }
    }
    
    // Symbol not found, add it first
    addSymbol(identifier);
    updateSymbolValue(identifier, value);
}

// Register numeric constants in symbol table
void registerConstant(int value) {
    char constName[64];
    sprintf(constName, "CONST_%d", value);
    
    // Check if constant already exists
    for (int i = 0; i < symbolCount; i++) {
        if (strcmp(symbolTable[i].identifier, constName) == 0)
            return; // Already registered
    }
    
    // Add new constant
    addSymbol(constName);
    updateSymbolValue(constName, value);
}

int tempVarCount = 0;
char tempVarName[64];

void createTempVar(char* buffer) {
    sprintf(buffer, "TEMP_%d", tempVarCount++);
    addSymbol(buffer);
}

FILE* asmOutput;

// Function declarations to avoid compilation errors
void generateExprCode(SyntaxNode* node);
void generateAssignmentCode(Command* cmd);
void generateAssemblyCode();

// Improved function for code generation from expressions
void generateExprCode(SyntaxNode* node) {
    if (node->category == NODE_LITERAL) {
        // Ensure constant exists in symbol table
        char constName[64];
        sprintf(constName, "CONST_%d", node->value);
        registerConstant(node->value);
        fprintf(asmOutput, "LDA %s\n", constName);
    } 
    else if (node->category == NODE_IDENTIFIER) {
        addSymbol(node->identifier);
        fprintf(asmOutput, "LDA %s\n", node->identifier);
    } 
    else if (node->category == NODE_OPERATION) {
        char op = node->operation.operator;
        
        if (op == '+') {
            // Handle addition cases
            if (node->operation.leftChild->category == NODE_IDENTIFIER && 
                node->operation.rightChild->category == NODE_IDENTIFIER) {
                // Simple case: id + id
                fprintf(asmOutput, "LDA %s\n", node->operation.leftChild->identifier);
                fprintf(asmOutput, "ADD %s\n", node->operation.rightChild->identifier);
            } 
            else if (node->operation.leftChild->category == NODE_IDENTIFIER && 
                     node->operation.rightChild->category == NODE_LITERAL) {
                // Case: id + number
                char constName[64];
                sprintf(constName, "CONST_%d", node->operation.rightChild->value);
                registerConstant(node->operation.rightChild->value);
                fprintf(asmOutput, "LDA %s\n", node->operation.leftChild->identifier);
                fprintf(asmOutput, "ADD %s\n", constName);
            } 
            else if (node->operation.leftChild->category == NODE_LITERAL && 
                     node->operation.rightChild->category == NODE_IDENTIFIER) {
                // Case: number + id
                char constName[64];
                sprintf(constName, "CONST_%d", node->operation.leftChild->value);
                registerConstant(node->operation.leftChild->value);
                fprintf(asmOutput, "LDA %s\n", constName);
                fprintf(asmOutput, "ADD %s\n", node->operation.rightChild->identifier);
            } 
            else if (node->operation.leftChild->category == NODE_LITERAL && 
                     node->operation.rightChild->category == NODE_LITERAL) {
                // Case: number + number (optimize at compile time)
                int result = node->operation.leftChild->value + node->operation.rightChild->value;
                char constName[64];
                sprintf(constName, "CONST_%d", result);
                registerConstant(result);
                fprintf(asmOutput, "LDA %s\n", constName);
            } 
            else {
                // Complex case with subexpressions
                generateExprCode(node->operation.leftChild);  // Load left side
                
                // If right side is simple, add directly
                if (node->operation.rightChild->category == NODE_IDENTIFIER) {
                    fprintf(asmOutput, "ADD %s\n", node->operation.rightChild->identifier);
                } 
                else if (node->operation.rightChild->category == NODE_LITERAL) {
                    char constName[64];
                    sprintf(constName, "CONST_%d", node->operation.rightChild->value);
                    registerConstant(node->operation.rightChild->value);
                    fprintf(asmOutput, "ADD %s\n", constName);
                } 
                else {
                    // Right side is complex, store left result
                    createTempVar(tempVarName);
                    fprintf(asmOutput, "STA %s\n", tempVarName);
                    
                    // Evaluate right side
                    generateExprCode(node->operation.rightChild);
                    
                    // Add left result
                    fprintf(asmOutput, "ADD %s\n", tempVarName);
                }
            }
        } 
        else if (op == '-') {
            // Handle subtraction cases
            if (node->operation.leftChild->category == NODE_IDENTIFIER && 
                node->operation.rightChild->category == NODE_IDENTIFIER) {
                // Simple case: id - id
                fprintf(asmOutput, "LDA %s\n", node->operation.leftChild->identifier);
                fprintf(asmOutput, "SUB %s\n", node->operation.rightChild->identifier);
            } 
            else if (node->operation.leftChild->category == NODE_IDENTIFIER && 
                     node->operation.rightChild->category == NODE_LITERAL) {
                // Case: id - number
                char constName[64];
                sprintf(constName, "CONST_%d", node->operation.rightChild->value);
                registerConstant(node->operation.rightChild->value);
                fprintf(asmOutput, "LDA %s\n", node->operation.leftChild->identifier);
                fprintf(asmOutput, "SUB %s\n", constName);
            } 
            else if (node->operation.leftChild->category == NODE_LITERAL && 
                     node->operation.rightChild->category == NODE_IDENTIFIER) {
                // Case: number - id
                char constName[64];
                sprintf(constName, "CONST_%d", node->operation.leftChild->value);
                registerConstant(node->operation.leftChild->value);
                fprintf(asmOutput, "LDA %s\n", constName);
                fprintf(asmOutput, "SUB %s\n", node->operation.rightChild->identifier);
            } 
            else if (node->operation.leftChild->category == NODE_LITERAL && 
                     node->operation.rightChild->category == NODE_LITERAL) {
                // Case: number - number (optimize at compile time)
                int result = node->operation.leftChild->value - node->operation.rightChild->value;
                char constName[64];
                sprintf(constName, "CONST_%d", result);
                registerConstant(result);
                fprintf(asmOutput, "LDA %s\n", constName);
            } 
            else {
                // Complex case with subexpressions
                generateExprCode(node->operation.leftChild);  // Load left side
                
                // If right side is simple, subtract directly
                if (node->operation.rightChild->category == NODE_IDENTIFIER) {
                    fprintf(asmOutput, "SUB %s\n", node->operation.rightChild->identifier);
                } 
                else if (node->operation.rightChild->category == NODE_LITERAL) {
                    char constName[64];
                    sprintf(constName, "CONST_%d", node->operation.rightChild->value);
                    registerConstant(node->operation.rightChild->value);
                    fprintf(asmOutput, "SUB %s\n", constName);
                } 
                else {
                    // Right side is complex, store left result
                    createTempVar(tempVarName);
                    fprintf(asmOutput, "STA %s\n", tempVarName);
                    
                    // Evaluate right side
                    generateExprCode(node->operation.rightChild);
                    
                    // Store right result
                    char rightTemp[64];
                    createTempVar(rightTemp);
                    fprintf(asmOutput, "STA %s\n", rightTemp);
                    
                    // Load left and subtract right
                    fprintf(asmOutput, "LDA %s\n", tempVarName);
                    fprintf(asmOutput, "SUB %s\n", rightTemp);
                }
            }
        } 
        else if (op == '*') {
            // Handle multiplication
            char resultTemp[64];
            createTempVar(resultTemp); // For storing the result
            fprintf(asmOutput, "LDA CONST_0\n"); // Start with zero
            fprintf(asmOutput, "STA %s\n", resultTemp);

            if (node->operation.rightChild->category == NODE_LITERAL) {
                // Multiplier is a literal number
                int multiplier = node->operation.rightChild->value;
                for (int i = 0; i < multiplier; i++) {
                    generateExprCode(node->operation.leftChild); // Load left operand
                    fprintf(asmOutput, "ADD %s\n", resultTemp);
                    fprintf(asmOutput, "STA %s\n", resultTemp);
                }
            } 
            else if (node->operation.rightChild->category == NODE_IDENTIFIER) {
                // Multiplier is a variable
                // Check if variable has a known value
                int multiplier = -1;
                for (int i = 0; i < symbolCount; i++) {
                    if (strcmp(symbolTable[i].identifier, node->operation.rightChild->identifier) == 0 && 
                        symbolTable[i].initialized) {
                        multiplier = symbolTable[i].data;
                        break;
                    }
                }
                
                if (multiplier >= 0) {
                    // Use the known value
                    for (int i = 0; i < multiplier; i++) {
                        generateExprCode(node->operation.leftChild); // Load left operand
                        fprintf(asmOutput, "ADD %s\n", resultTemp);
                        fprintf(asmOutput, "STA %s\n", resultTemp);
                    }
                } 
                else {
                    // Unknown value, display warning
                    fprintf(asmOutput, "; Warning: Multiplication with dynamic variable %s\n", 
                            node->operation.rightChild->identifier);
                    fprintf(asmOutput, "LDA CONST_0\n");
                    printf("Warning: Multiplication with unknown value in %s\n", 
                           node->operation.rightChild->identifier);
                }
            }
            
            fprintf(asmOutput, "LDA %s\n", resultTemp); // Load final result
        } 
        else if (op == '/') {
            if (node->operation.leftChild->category == NODE_LITERAL && 
                node->operation.rightChild->category == NODE_LITERAL) {
                
                // Check for division by zero
                if (node->operation.rightChild->value == 0) {
                    fprintf(stderr, "Erro: Divisão por 0 detectada\n");
                    fprintf(asmOutput, "; Erro: Divisão por 0\n");
                    fprintf(asmOutput, "LDA CONST_0\n");
                    return;
                }
                
                // Optimize constant division
                int result = node->operation.leftChild->value / node->operation.rightChild->value;
                char constName[64];
                sprintf(constName, "CONST_%d", result);
                registerConstant(result);
                fprintf(asmOutput, "LDA %s\n", constName);
            } 
            else {
                // General division algorithm
                char quotient[64], dividend[64], divisor[64];
                createTempVar(quotient);   // For result
                createTempVar(dividend);   // For dividend (working copy)
                createTempVar(divisor);    // For divisor
        
                // Initialize quotient with 0
                fprintf(asmOutput, "LDA CONST_0\n");
                fprintf(asmOutput, "STA %s\n", quotient);
        
                // Load dividend
                if (node->operation.leftChild->category == NODE_LITERAL) {
                    char constName[64];
                    sprintf(constName, "CONST_%d", node->operation.leftChild->value);
                    registerConstant(node->operation.leftChild->value);
                    fprintf(asmOutput, "LDA %s\n", constName);
                } 
                else if (node->operation.leftChild->category == NODE_IDENTIFIER) {
                    fprintf(asmOutput, "LDA %s\n", node->operation.leftChild->identifier);
                } 
                else {
                    generateExprCode(node->operation.leftChild);
                }
                fprintf(asmOutput, "STA %s\n", dividend);
        
                // Load divisor
                if (node->operation.rightChild->category == NODE_LITERAL) {
                    if (node->operation.rightChild->value == 0) {
                        fprintf(stderr, "Erro: Divisão por 0 detectada\n");
                        fprintf(asmOutput, "; Erro: Division by zero\n");
                        fprintf(asmOutput, "LDA CONST_0\n");
                        return;
                    }
                    char constName[64];
                    sprintf(constName, "CONST_%d", node->operation.rightChild->value);
                    registerConstant(node->operation.rightChild->value);
                    fprintf(asmOutput, "LDA %s\n", constName);
                } 
                else if (node->operation.rightChild->category == NODE_IDENTIFIER) {
                    fprintf(asmOutput, "LDA %s\n", node->operation.rightChild->identifier);
                } 
                else {
                    generateExprCode(node->operation.rightChild);
                }
                fprintf(asmOutput, "STA %s\n", divisor);
        
                // Ensure CONST_1 exists
                registerConstant(1);
        
                // Create unique labels for division loop
                static int divLabelCount = 0;
                char divLoop[64], divEnd[64];
                sprintf(divLoop, "DIV_LOOP_%d", divLabelCount);
                sprintf(divEnd, "DIV_DONE_%d", divLabelCount++);
        
                // Division loop implementation
                fprintf(asmOutput, "%s:\n", divLoop);
                fprintf(asmOutput, "LDA %s\n", dividend);
                fprintf(asmOutput, "SUB %s\n", divisor);
                fprintf(asmOutput, "JMN %s\n", divEnd); // Exit if negative
                fprintf(asmOutput, "STA %s\n", dividend); // Update dividend
                fprintf(asmOutput, "LDA %s\n", quotient);
                fprintf(asmOutput, "ADD CONST_1\n");
                fprintf(asmOutput, "STA %s\n", quotient);
                fprintf(asmOutput, "JMP %s\n", divLoop);
                fprintf(asmOutput, "%s:\n", divEnd);
        
                // Load final quotient
                fprintf(asmOutput, "LDA %s\n", quotient);
            }
        }
    }
}

void generateAssignmentCode(Command* cmd) {
    // If assignment is a literal number, update symbol value directly
    if (cmd->expression && cmd->expression->category == NODE_LITERAL) {
        updateSymbolValue(cmd->variable, cmd->expression->value);
        // Generate direct assignment code
        char constName[64];
        sprintf(constName, "CONST_%d", cmd->expression->value);
        registerConstant(cmd->expression->value);
        fprintf(asmOutput, "LDA %s\n", constName);
        fprintf(asmOutput, "STA %s\n", cmd->variable);
    } else {
        // For complex expressions, generate normal code
        generateExprCode(cmd->expression);
        addSymbol(cmd->variable);
        fprintf(asmOutput, "STA %s\n", cmd->variable);
    }
}

void generateAssemblyCode() {
    fprintf(asmOutput, "; Assembly code generated by compiler\n");
    fprintf(asmOutput, "; Program: %s\n\n", program.title);
    
    fprintf(asmOutput, ".DATA\n");
    /* Fixed header */
    fprintf(asmOutput, "UNITY DB 1\n");
    fprintf(asmOutput, "CONST_0 DB 0\n");
    fprintf(asmOutput, "CONST_1 DB 1\n");
    fprintf(asmOutput, "NEGATIVE DB 255\n");
    fprintf(asmOutput, "RESULT DB ?\n");
    
    /* Pre-processing to ensure all constants are defined */
    Command* cmd = commandList;
    while (cmd) {
        if (cmd->expression && cmd->expression->category == NODE_LITERAL) {
            registerConstant(cmd->expression->value);
        }
        cmd = cmd->next;
    }
    
    /* Print symbols from table */
    for (int i = 0; i < symbolCount; i++) {
        if (strcmp(symbolTable[i].identifier, "UNITY") == 0 ||
            strcmp(symbolTable[i].identifier, "CONST_0") == 0 ||
            strcmp(symbolTable[i].identifier, "CONST_1") == 0 ||
            strcmp(symbolTable[i].identifier, "NEGATIVE") == 0 ||
            strcmp(symbolTable[i].identifier, "RESULT") == 0)
            continue;
        
        if (strncmp(symbolTable[i].identifier, "TEMP_", 5) == 0) {
            fprintf(asmOutput, "%s DB ?\n", symbolTable[i].identifier);
        } else if (strncmp(symbolTable[i].identifier, "CONST_", 6) == 0) {
            fprintf(asmOutput, "%s DB %d\n", symbolTable[i].identifier, symbolTable[i].data);
        } else if (!symbolTable[i].initialized) {
            fprintf(asmOutput, "%s DB ?\n", symbolTable[i].identifier);
        } else {
            fprintf(asmOutput, "%s DB %d\n", symbolTable[i].identifier, symbolTable[i].data);
        }
    }
    
    fprintf(asmOutput, "\n.CODE\n");
    fprintf(asmOutput, ".ORG 0\n");
    
    /* Generate instructions for assignments */
    cmd = commandList;
    while (cmd) {
        fprintf(asmOutput, "; Assignment: %s = ...\n", cmd->variable);
        generateAssignmentCode(cmd);
        cmd = cmd->next;
    }
    
    /* Generate code for final expression */
    fprintf(asmOutput, "; Result expression\n");
    generateExprCode(program.output);
    fprintf(asmOutput, "STA RESULT\n");
    fprintf(asmOutput, "HALT\n");
    
    printf("Código assembly gerado!\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s sourcefile.lpn\n", argv[0]);
        return 1;
    }

    FILE* inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        perror("Erro para abrir .lpn");
        return 1;
    }

    // Determine file size for buffer allocation
    fseek(inputFile, 0, SEEK_END);
    long fileSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);

    // Allocate memory for file content
    inputCode = (char*)malloc(fileSize + 1);
    if (!inputCode) {
        perror("Erro na alocação de memória");
        fclose(inputFile);
        return 1;
    }

    // Read file into buffer
    size_t bytesRead = fread(inputCode, 1, fileSize, inputFile);
    inputCode[bytesRead] = '\0';
    fclose(inputFile);

    // Create output .asm file
    char outputFilename[256];
    strncpy(outputFilename, argv[1], sizeof(outputFilename)-5);
    outputFilename[sizeof(outputFilename)-5] = '\0';
    char* dot = strrchr(outputFilename, '.');
    if (dot) *dot = '\0';
    strcat(outputFilename, ".asm");
    
    asmOutput = fopen(outputFilename, "w");
    if (!asmOutput) {
        perror("Erro para criar .asm");
        free(inputCode);
        return 1;
    }

    // Initialize structures
    commandList = NULL;
    lastCommand = NULL;
    symbolCount = 0;
    tempVarCount = 0;

    // Run lexical analysis
    scanTokens();
    
    // Run syntax analysis
    parseProgram();
    
    // Generate assembly code
    generateAssemblyCode();
    
    // Free memory
    freeCommands(commandList);
    freeNode(program.output);
    free(inputCode);
    fclose(asmOutput);
    
    printf("(successful) Arquivo gerado sem erros de compilação. Arquivo: %s\n", outputFilename);
    return 0;
}

