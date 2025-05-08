#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "memory.h"
#include "main.h"
#include "gui.h"

#define ALG_FCFS  0
#define ALG_RR    1
#define ALG_MLFQ  2

// Initialize mutex controller
void init_mutex(MutexController* mc) {
  for(int i = 0; i < MAX_MUTEXES; i++) {
    mc->locked[i] = 0;
    init_queue(&mc->blocked_queues[i]);
  }
  init_queue(&mc->general_blocked);
}

// Build process
Process* build_process(Memory* mem, int pid, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return NULL;
    }
    int instruction_count = 0;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, file)) {
        instruction_count++;
    }
    fclose(file);

    Process* proc = (Process*)malloc(sizeof(Process));
    proc->pcb.process_id = pid;
    proc->pcb.state = READY;
    proc->pcb.priority = 1;
    proc->pcb.program_counter = 0;
    proc->quantum_used = 0;
    proc->remaining_time = 0;

    if (allocate_memory(mem, proc, instruction_count) != 0) {
        free(proc);
        return NULL;
    }

    int loaded = load_instructions(mem, proc, filename);
    if (loaded < 0) {
        for (int i = proc->pcb.memory_lower; i <= proc->pcb.memory_upper; i++) {
            mem->words[i].name[0] = '\0';
            mem->words[i].value[0] = '\0';
        }
        free(proc);
        return NULL;
    }
    proc->instruction_count = loaded;
    snprintf(mem->words[proc->pcb_start + 1].value, MAX_VALUE, "Ready");
    return proc;
}

// Free process
void free_process(Memory* mem, Process* proc) {
    for (int i = proc->pcb.memory_lower; i <= proc->pcb.memory_upper; i++) {
        mem->words[i].name[0] = '\0';
        mem->words[i].value[0] = '\0';
    }
    free(proc);
}

// Initialize simulation engine
void init_simulation(SimulationEngine* engine, Memory* memory, MutexController* mutex_ctrl, 
                    Process** processes, int process_count, int algorithm, int quantum) {
    engine->memory = memory;
    engine->mutex_ctrl = mutex_ctrl;
    engine->processes = processes;
    engine->process_count = process_count;
    engine->algorithm = algorithm;
    engine->quantum = quantum;
    engine->clock_cycle = 0;
    engine->completed_processes = 0;

    for (int i = 0; i < 4; i++) {
        init_queue(&engine->ready_queues[i]);
    }
    for (int i = 0; i < process_count; i++) {
        enqueue(&engine->ready_queues[algorithm == 2 ? processes[i]->pcb.priority - 1 : 0], processes[i]);
    }
}

// Select next process
Process* select_next_process(SimulationEngine* engine) {
    Process* proc = NULL;
    int queue_index = 0;

    if (engine->algorithm == 2) {
        for (int i = 0; i < 4; i++) {
            if (engine->ready_queues[i].front) {
                proc = dequeue(&engine->ready_queues[i], 0);
                queue_index = i;
                break;
            }
        }
    } else {
        proc = dequeue(&engine->ready_queues[0], 0);
    }

    if (proc) {
        proc->pcb.state = RUNNING;
        snprintf(engine->memory->words[proc->pcb_start + 1].value, MAX_VALUE, "Running");
        proc->quantum_used = 0;
    }
    return proc;
}

int execute_cycle(SimulationEngine* engine, Process* current_proc) {
    printf("Cycle %d: Starting with PID %d\n",
           engine->clock_cycle,
           current_proc ? current_proc->pcb.process_id : -1);
    if (!current_proc) {
        printf("No process to execute\n");
        log_cycle(engine, NULL, NULL, -1);
        return 0;
    }

    // 1) determine quantum
    int max_quantum;
    if (engine->algorithm == ALG_FCFS) {
        // FCFS: drain the rest of the process
        max_quantum = current_proc->instruction_count
                     - current_proc->pcb.program_counter;
    }
    else if (engine->algorithm == ALG_MLFQ) {
        // MLFQ: quantum = 2^(priority-1)
        max_quantum = 1 << (current_proc->pcb.priority - 1);
    }
    else {
        // RR
        max_quantum = engine->quantum;
    }

    int instructions_executed = 0;
    int result = 1;
    while (instructions_executed < max_quantum &&
           result == 1 &&
           current_proc->pcb.program_counter < current_proc->instruction_count) {
        int instr_addr = current_proc->instruction_start + current_proc->pcb.program_counter;
        char* instruction = engine->memory->words[instr_addr].value;
        printf("Executing instruction %d of %d for PID %d: %s\n",
               instructions_executed + 1,
               max_quantum,
               current_proc->pcb.process_id,
               instruction);
        result = execute_instruction(
                    engine->memory,
                    current_proc,
                    engine->mutex_ctrl,
                    &engine->ready_queues[
                        engine->algorithm == 2
                          ? current_proc->pcb.priority - 1
                          : 0]);
        printf("Instruction result: %d, quantum_used: %d\n",
               result, current_proc->quantum_used);
        log_cycle(engine, current_proc, instruction, result);
        instructions_executed++;
        if (result != 1) break;
    }

    // 2) aftermath: block, terminate or preempt
    if (result == -1) {
        printf("PID %d blocked\n", current_proc->pcb.process_id);

    } 
    else if (result == 0) {
         printf("PID %d terminated\n", current_proc->pcb.process_id);
         current_proc->pcb.state = TERMINATED;
         snprintf(engine->memory->words[current_proc->pcb_start + 1].value,
                  MAX_VALUE, "Terminated");
         engine->completed_processes++;
         //        free_process(engine->memory, current_proc);
         // just clear its memory region, leave struct alive until Reset:
         for (int i = current_proc->pcb.memory_lower;
              i <= current_proc->pcb.memory_upper;
              i++) {
             engine->memory->words[i].name[0] = '\0';
             engine->memory->words[i].value[0] = '\0';
         }
     }
    // only RR or MLFQ ever preempt
    else if ((engine->algorithm == ALG_RR  && instructions_executed >= max_quantum)
          || (engine->algorithm == ALG_MLFQ && instructions_executed >= max_quantum)) 
    {
        printf("Quantum expired for PID %d\n", current_proc->pcb.process_id);
        current_proc->pcb.state = READY;
        snprintf(engine->memory->words[current_proc->pcb_start + 1].value,
                 MAX_VALUE, "Ready");

        if (engine->algorithm == ALG_MLFQ) {
            // demote one level, unless already at 4
            if (current_proc->pcb.priority < 4) {
                current_proc->pcb.priority++;
                snprintf(engine->memory->words[current_proc->pcb_start + 2].value,
                         MAX_VALUE, "%d", current_proc->pcb.priority);
            }
        }

        // enqueue on the appropriate queue
        int qidx = (engine->algorithm == ALG_MLFQ)
                 ? current_proc->pcb.priority - 1
                 : 0;   // RR always queue 0
        enqueue(&engine->ready_queues[qidx], current_proc);
    }

    // 3) advance the clock no matter what
    engine->clock_cycle++;
    return 1;
}


// int main() {
//     Memory memory;
//     init_memory(&memory);                        // ← initialize before use

//     MutexController mutex_ctrl;
//     init_mutex(&mutex_ctrl);

//     Process* p1 = build_process(&memory, 1, "./Program_1.txt");
//     Process* p2 = build_process(&memory, 2, "./Program_2.txt");
//     Process* p3 = build_process(&memory, 3, "./Program_3.txt");

//     SimulationEngine engine;
//     engine.memory = &memory;
//     engine.mutex_ctrl = &mutex_ctrl;
//     engine.process_count = 0;
//     engine.completed_processes = 0;
//     engine.clock_cycle = 0;
//     engine.algorithm = 0;
//     engine.quantum = 1;

//     create_gtk_gui(&engine);

//     return 0;
// }

//// filepath: /Users/mabde/Downloads/OS project/main.c
int main() {
    Memory memory;
    init_memory(&memory);

    MutexController mutex_ctrl;
    init_mutex(&mutex_ctrl);

    Process* p1 = build_process(&memory, 1, "./Program_1.txt");
    Process* p2 = build_process(&memory, 2, "./Program_2.txt");
    Process* p3 = build_process(&memory, 3, "./Program_3.txt");

    // — new: collect and initialize
    Process* procs[3];
    int      proc_count = 0;
    if (p1) procs[proc_count++] = p1;
    if (p2) procs[proc_count++] = p2;
    if (p3) procs[proc_count++] = p3;

    SimulationEngine engine;
    engine.memory             = &memory;
    engine.mutex_ctrl         = &mutex_ctrl;
    engine.processes          = procs;
    engine.process_count      = proc_count;
    engine.algorithm          = 0;    // default FCFS
    engine.quantum            = 1;    // default RR quantum
    engine.clock_cycle        = 0;
    engine.completed_processes= 0;

    // IMPORTANT: actually enqueue your initial procs
    init_simulation(&engine,
                    &memory,
                    &mutex_ctrl,
                    procs,
                    proc_count,
                    engine.algorithm,
                    engine.quantum);

    // now enter GTK
    create_gtk_gui(&engine);
    return 0;
}