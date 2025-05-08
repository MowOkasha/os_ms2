#include "memory.h"
#include "main.h"       // <<< pull in the full definition of struct Process
#include <stdlib.h>
#include <string.h>

void init_memory(Memory* mem) {
    mem->next_free = 0;
    for (int i = 0; i < MEMORY_SIZE; i++) {
        mem->words[i].name[0] = '\0';
        mem->words[i].value[0] = '\0';
    }
}

int allocate_memory(Memory* mem, struct Process* proc, int instruction_count) {
    if (mem->next_free + instruction_count + PCB_SIZE > MEMORY_SIZE)
        return -1;
    proc->pcb.memory_lower = mem->next_free;
    proc->pcb.memory_upper = mem->next_free + instruction_count + PCB_SIZE - 1;
    proc->instruction_start = mem->next_free;
    proc->pcb_start         = mem->next_free + instruction_count;
    mem->next_free          = proc->pcb.memory_upper + 1;
    return 0;
}

int load_instructions(Memory* mem, struct Process* proc, const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) return -1;
    char line[MAX_INSTRUCTIONS];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < MAX_INSTRUCTIONS) {
        strncpy(mem->words[proc->instruction_start + count].value,
                line, MAX_VALUE-1);
        count++;
    }
    fclose(f);
    return count;
}

int check_boundaries(struct Process* proc, int address) {
    return (address >= proc->pcb.memory_lower && address <= proc->pcb.memory_upper) ? 0 : -1;
}

int get_variable(Memory* mem, struct Process* proc, const char* var_name, char* value) {
    // stub: always fail
    return -1;
}

int set_variable(Memory* mem, struct Process* proc, const char* var_name, const char* value) {
    // stub: always fail
    return -1;
}