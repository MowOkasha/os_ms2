#ifndef MEMORY_H
#define MEMORY_H

#include <stdio.h>

#define MEMORY_SIZE      60
#define MAX_NAME         50
#define MAX_VALUE       256
#define MAX_INSTRUCTIONS 50
#define PCB_SIZE         6       // <<< add this

/* forward reference */
struct Process;

typedef struct {
    char name[MAX_NAME];
    char value[MAX_VALUE];
} MemoryWord;

typedef struct {
    MemoryWord words[MEMORY_SIZE];
    int        next_free;
} Memory;

/* memory APIs */
void init_memory(Memory* mem);
int  allocate_memory(Memory* mem, struct Process* proc, int instruction_count);
int  check_boundaries(struct Process* proc, int address);
int  load_instructions(Memory* mem, struct Process* proc, const char* filename);
int  get_variable(Memory* mem, struct Process* proc, const char* var_name, char* value);
int  set_variable(Memory* mem, struct Process* proc, const char* var_name, const char* value);

#endif // MEMORY_H