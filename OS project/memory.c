#include "memory.h"
#include "main.h"
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

/*
 * Find a variable in this proc’s memory segment.
 * Returns 0+copies into `value` on success, -1 if not found.
 */
int get_variable(Memory* mem, Process* proc, const char* var_name, char* value) {
    for (int i = proc->pcb.memory_lower; i <= proc->pcb.memory_upper; i++) {
        if (strcmp(mem->words[i].name, var_name) == 0) {
            strncpy(value, mem->words[i].value, MAX_VALUE-1);
            value[MAX_VALUE-1] = '\0';
            return 0;
        }
    }
    return -1;
}

/*
 * Store (or overwrite) a variable in this proc’s memory segment.
 * Returns 0 on success, -1 if there was no free slot.
 */
int set_variable(Memory* mem, Process* proc, const char* var_name, const char* value) {
    for (int i = proc->pcb.memory_lower; i <= proc->pcb.memory_upper; i++) {
        // either overwrite existing or pick the first empty slot
        if (mem->words[i].name[0]=='\0' || strcmp(mem->words[i].name, var_name)==0) {
            strncpy(mem->words[i].name,  var_name, MAX_NAME-1);
            mem->words[i].name[MAX_NAME-1] = '\0';
            strncpy(mem->words[i].value, value, MAX_VALUE-1);
            mem->words[i].value[MAX_VALUE-1] = '\0';
            return 0;
        }
    }
    return -1;
}