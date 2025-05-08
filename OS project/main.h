#ifndef MAIN_H
#define MAIN_H

#include "memory.h"

#define MAX_MUTEXES    3   // userInput, userOutput, file

/* process states */
typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
} ProcessState;

/* PCB */
typedef struct {
    int          process_id;
    ProcessState state;
    int          priority;
    int          program_counter;
    int          memory_lower;
    int          memory_upper;
} PCB;

/* process control block */
typedef struct Process {
    PCB     pcb;
    int     instruction_start;   // where code begins in memory.words[]
    int     instruction_count;   // # of instructions loaded
    int     pcb_start;           // index in memory.words[] where PCB data begins
    int     quantum_used;
    int     remaining_time;
} Process;

/* queue node */
typedef struct QueueNode {
    struct QueueNode* next;
    Process*          process;
} QueueNode;

/* simple FIFO queue */
typedef struct {
    QueueNode* front;
    QueueNode* rear;
} Queue;

/* mutex controller holds per-mutex lock & blocked queues */
typedef struct {
    int    locked[MAX_MUTEXES];
    Queue  blocked_queues[MAX_MUTEXES];
    Queue  general_blocked;
} MutexController;

/* simulation engine state */
typedef struct SimulationEngine {
    Memory*          memory;
    MutexController* mutex_ctrl;
    Process**        processes;
    int              process_count;
    int              algorithm;     /* 0=FCFS,1=RR,2=MLFQ */
    int              quantum;
    Queue            ready_queues[4];
    int              clock_cycle;
    int              completed_processes;
} SimulationEngine;

/* core APIs */
void     init_mutex(MutexController* mutex_ctrl);
Process* build_process(Memory* mem, int pid, const char* filename);
void     free_process(Memory* mem, Process* proc);
void     init_simulation(SimulationEngine* engine,
                         Memory*          memory,
                         MutexController* mutex_ctrl,
                         Process**        processes,
                         int              process_count,
                         int              algorithm,
                         int              quantum);

/* queue operations */
void     init_queue(Queue* queue);
void     enqueue(Queue* queue, Process* process);
Process* dequeue(Queue* queue, int param);

/* instruction execution & logging */
int      execute_instruction(Memory* mem,
                             Process* proc,
                             MutexController* mutex_ctrl,
                             Queue* ready_queue);
void     log_cycle(SimulationEngine* engine,
                   Process* proc,
                   const char* instruction,
                   int result);

/* --- functions used by gui.c --- */
Process* select_next_process(SimulationEngine* engine);
int      execute_cycle(SimulationEngine* engine,
                       Process* current_proc);

/* utility */
#define MAX_LINE_LENGTH 256

#endif // MAIN_H