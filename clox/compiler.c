#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,    // =
    PREC_OR,            // or
    PREC_AND,           // and
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_TERM,          // + -
    PREC_FACTOR,        // * /
    PREC_UNARY,         // ! -
    PREC_CALL,          // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

Parser parser;
Chunk* compilingChunk;

static Chunk* currentChunk() {
    return compilingChunk;
}

static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, "at end");
    } else if (token->type == TOKEN_ERROR) {
        // 코드 없음
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

// 파서의 이전 토큰 에러처리
static void error(const char* message) {
    errorAt(&parser.previous, message);
}

// 파서의 현재 토큰 에러처리
static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

// 토큰 소비 함수
static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        // 에러 토큰은 start에 에러 상수 문자열 주소를 저장함.
        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

// 바이트코드 생성 함수
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants is one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void endCompiler() {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void binary() {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch(operatorType) {
        case TOKEN_PLUS:        emitByte(OP_ADD); break;
        case TOKEN_MINUS:       emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR:        emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH:       emitByte(OP_DIVIDE); break;
        default: return; // 실행되지 않는 코드
    }
}

static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}

static void unary() {
    TokenType operatorType = parser.previous.type;

    // 피연산자를 컴파일한다.
    parsePrecedence(PREC_UNARY);

    // 연산자 명령어를 내보낸다.
    switch (operatorType) {
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // 실행되지 않는 코드.
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]      = {grouping , NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN]     = {NULL , NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE]      = {NULL , NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE]     = {NULL , NULL, PREC_NONE},
    [TOKEN_COMMA]           = {NULL , NULL, PREC_NONE},
    [TOKEN_DOT]             = {NULL , NULL, PREC_NONE},
    [TOKEN_MINUS]           = {unary , binary, PREC_TERM},
    [TOKEN_PLUS]            = {NULL , binary, PREC_TERM},
    [TOKEN_SEMICOLON]       = {NULL , NULL, PREC_NONE},
    [TOKEN_SLASH]           = {NULL , binary, PREC_FACTOR},
    [TOKEN_STAR]            = {NULL , binary, PREC_FACTOR},
    [TOKEN_BANG]            = {NULL , NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL]      = {NULL , NULL, PREC_NONE},
    [TOKEN_EQUAL]           = {NULL , NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL]     = {NULL , NULL, PREC_NONE},
    [TOKEN_GREATER]         = {NULL , NULL, PREC_NONE},
    [TOKEN_GREATER_EQUAL]   = {NULL , NULL, PREC_NONE},
    [TOKEN_LESS]            = {NULL , NULL, PREC_NONE},
    [TOKEN_LESS_EQUAL]      = {NULL , NULL, PREC_NONE},
    [TOKEN_IDENTIFIER]      = {NULL , NULL, PREC_NONE},
    [TOKEN_STRING]          = {NULL , NULL, PREC_NONE},
    [TOKEN_NUMBER]          = {number , NULL, PREC_NONE},
    [TOKEN_AND]             = {NULL , NULL, PREC_NONE},
    [TOKEN_CLASS]           = {NULL , NULL, PREC_NONE},
    [TOKEN_ELSE]            = {NULL , NULL, PREC_NONE},
    [TOKEN_FALSE]           = {NULL , NULL, PREC_NONE},
    [TOKEN_FOR]             = {NULL , NULL, PREC_NONE},
    [TOKEN_FUN]             = {NULL , NULL, PREC_NONE},
    [TOKEN_IF]              = {NULL , NULL, PREC_NONE},
    [TOKEN_NIL]             = {NULL , NULL, PREC_NONE},
    [TOKEN_OR]              = {NULL , NULL, PREC_NONE},
    [TOKEN_PRINT]           = {NULL , NULL, PREC_NONE},
    [TOKEN_RETURN]          = {NULL , NULL, PREC_NONE},
    [TOKEN_SUPER]           = {NULL , NULL, PREC_NONE},
    [TOKEN_THIS]            = {NULL , NULL, PREC_NONE},
    [TOKEN_TRUE]            = {NULL , NULL, PREC_NONE},
    [TOKEN_VAR]             = {NULL , NULL, PREC_NONE},
    [TOKEN_WHILE]           = {NULL , NULL, PREC_NONE},
    [TOKEN_ERROR]           = {NULL , NULL, PREC_NONE},
    [TOKEN_EOF]             = {NULL , NULL, PREC_NONE},
};

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    prefixRule();

    /* precedence 는 이항 연산자의 우선순위임.
       즉, while문은 parser.current가 이항 연산자일 때만 실행됨.
       while문 조건문에서 <=의 좌변은 이전 연산자의 우선순위,
       우변은 다음 연산자의 우선순위임.
       postfix로 스택에 넣기 때문에, 우선순위가 높은 연산자가 나올때까지
       constant를 스택에 push하고 재귀로 다시 돌아오면서 연산자 우선순위에
       맞게 스택에 순서대로 push함.
       */
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

// 
static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;
    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;
}