#ifndef clox_debug_h
#define clox_debug_h

void disassembleChunk(Chunk* chunk, const char* name);
int disassembleInstruction(Chunk* chunk, int offset);

#endif