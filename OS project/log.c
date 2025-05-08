#include "main.h"
#include <stdio.h>

/*
 * Log the result of an instruction or cycle.
 */
void log_cycle(SimulationEngine* engine,
               Process* proc,
               const char* instruction,
               int result) {
    (void)engine; (void)proc; (void)instruction; (void)result;
    // TODO: write to file or stdout
    // e.g.: printf("Log: cycle %d, pid %d, instr=%s, res=%d\n", ...)
}