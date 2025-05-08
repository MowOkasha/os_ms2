#include "main.h"

/*
 * Execute one instruction for `proc`. 
 * Return:
 *   1 = ran one instruction successfully,
 *   0 = process terminated,
 *  -1 = blocked/error
 */
int execute_instruction(Memory* mem,
                        Process* proc,
                        MutexController* mutex_ctrl,
                        Queue* ready_queue) {
    // TODO: decode and execute actual instruction in mem->words
    (void)mem; (void)proc; (void)mutex_ctrl; (void)ready_queue;
    return 1;
}