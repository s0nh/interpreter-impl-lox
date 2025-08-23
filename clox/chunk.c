#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->lineCnt = 0;
    chunk->lineCap = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

int getLine(Chunk* chunk, int index) {
    int count = index + 1;
    for (int i=0; i<chunk->lineCnt; i++) {
        if(chunk->lines[i].count >= count) {
            return chunk->lines[i].line;
        }

        count -= chunk->lines[i].count;
    }
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code,
            oldCapacity, chunk->capacity);
    }

    if (chunk->lineCap < chunk->lineCnt + 1) {
        int oldCapacity = chunk->lineCap;
        chunk->lineCap = GROW_CAPACITY(oldCapacity);
        chunk->lines = GROW_ARRAY(Line, chunk->lines, 
            oldCapacity, chunk->lineCap);
    }

    chunk->code[chunk->count] = byte;

    if (chunk->lineCnt == 0 || line != chunk->lines[chunk->lineCnt-1].line) {
        (chunk->lines[chunk->lineCnt]).line = line;
        (chunk->lines[chunk->lineCnt]).count = 0;
        chunk->lineCnt++;
    }
    
    (chunk->lines[chunk->lineCnt-1]).count++;
    chunk->count++;
}

int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}