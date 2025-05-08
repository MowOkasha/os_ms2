// #include "main.h"
// #include "memory.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// enum { MUTEX_INPUT=0, MUTEX_OUTPUT=1, MUTEX_FILE=2 };

// // map resource name → index
// static int get_mutex_index(const char* name) {
//     if (strcmp(name, "userInput")==0)  return MUTEX_INPUT;
//     if (strcmp(name, "userOutput")==0) return MUTEX_OUTPUT;
//     if (strcmp(name, "file")==0)       return MUTEX_FILE;
//     return -1;
// }

// /*
//  * Execute one instruction for `proc`. 
//  * Return:
//  *   1 = ran one instruction successfully,
//  *   0 = process terminated,
//  *  -1 = blocked
//  */
// int execute_instruction(Memory* mem,
//                         Process* proc,
//                         MutexController* mc,
//                         Queue* ready_queue)
// {
//     int addr = proc->instruction_start + proc->pcb.program_counter;
//     char buf[MAX_VALUE];
//     strncpy(buf, mem->words[addr].value, MAX_VALUE-1);
//     buf[MAX_VALUE-1] = '\0';

//     char* op  = strtok(buf, " \t\n");
//     char* a1  = strtok(NULL, " \t\n");
//     char* a2  = strtok(NULL, "\n");

//     if (!op) return -1;

//     // semWait
//     if (strcmp(op, "semWait")==0) {
//         int mi = get_mutex_index(a1);
//         if (mc->locked[mi]==0) {
//             mc->locked[mi]=1;
//         } else {
//             proc->pcb.state = BLOCKED;
//             enqueue(&mc->blocked_queues[mi], proc);
//             enqueue(&mc->general_blocked, proc);
//             return -1;
//         }

//     // semSignal
//     } else if (strcmp(op, "semSignal")==0) {
//         int mi = get_mutex_index(a1);
//         mc->locked[mi]=0;
//         Process* wake = dequeue(&mc->blocked_queues[mi], 0);
//         if (wake) {
//             wake->pcb.state = READY;
//             enqueue(ready_queue, wake);
//         }

//     // assign x y
//     } else if (strcmp(op, "assign")==0) {
//         if (strcmp(a2, "input")==0) {
//             printf("Please enter a value: ");
//             char in[MAX_VALUE];
//             if (fgets(in, MAX_VALUE, stdin)) {
//                 in[strcspn(in,"\n")] = '\0';
//                 set_variable(mem, proc, a1, in);
//             }
//         } else if (strncmp(a2, "readFile",8)==0) {
//             char* fn = strtok(a2+8," \t");
//             char tmp[MAX_VALUE];
//             if (get_variable(mem, proc, fn, tmp)==0) fn = tmp;
//             FILE* f = fopen(fn,"r");
//             char content[MAX_VALUE] = "";
//             char line[MAX_VALUE];
//             while (f && fgets(line, MAX_VALUE, f))
//                 strncat(content, line, MAX_VALUE-strlen(content)-1);
//             if (f) fclose(f);
//             set_variable(mem, proc, a1, content);
//         } else {
//             char val[MAX_VALUE];
//             if (get_variable(mem, proc, a2, val)==0)
//                 set_variable(mem, proc, a1, val);
//             else
//                 set_variable(mem, proc, a1, a2);
//         }

//     // print x
//     } else if (strcmp(op, "print")==0) {
//         char val[MAX_VALUE];
//         if (get_variable(mem, proc, a1, val)==0)
//             printf("%s\n", val);
//         else
//             printf("%s\n", a1);

//     // writeFile x y
//     } else if (strcmp(op, "writeFile")==0) {
//         char fn[MAX_VALUE], data[MAX_VALUE];
//         if (get_variable(mem, proc, a1, fn)!=0) strcpy(fn,a1);
//         if (get_variable(mem, proc, strtok(a2," \t"), data)!=0)
//             strcpy(data,a2);
//         FILE* f = fopen(fn,"w");
//         if (f) { fputs(data,f); fclose(f); }

//     // printFromTo x y
//     } else if (strcmp(op, "printFromTo")==0) {
//         char s1[MAX_VALUE], s2[MAX_VALUE];
//         if (get_variable(mem, proc, a1, s1)==0) ; else strcpy(s1,a1);
//         if (get_variable(mem, proc, strtok(a2," \t"), s2)==0) ; else strcpy(s2,a2);
//         int v1 = atoi(s1), v2 = atoi(s2);
//         for (int i = v1; i <= v2; i++)
//             printf("%d\n", i);

//     } else {
//         return -1;
//     }

//     proc->pcb.program_counter++;
//     proc->quantum_used++;
//     if (proc->pcb.program_counter >= proc->instruction_count)
//         return 0;
//     return 1;
// }

#include "main.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui.h"

enum { MUTEX_INPUT=0, MUTEX_OUTPUT=1, MUTEX_FILE=2 };

// map resource name → index
static int get_mutex_index(const char* name) {
    if (strcmp(name, "userInput")==0)  return MUTEX_INPUT;
    if (strcmp(name, "userOutput")==0) return MUTEX_OUTPUT;
    if (strcmp(name, "file")==0)       return MUTEX_FILE;
    return -1;
}

#define LOG(fmt, ...)  do {          \
    char _buf[512];                  \
    snprintf(_buf,sizeof(_buf),fmt,##__VA_ARGS__); \
    append_log(global_gui,_buf);     \
  } while(0)

/*
 * Execute one instruction for `proc`. 
 * Return:
 *   1 = ran one instruction successfully,
 *   0 = process terminated,
 *  -1 = blocked
 */
int execute_instruction(Memory* mem,
                        Process* proc,
                        MutexController* mc,
                        Queue* ready_queue)
{
    // fetch text of current instruction
    int addr = proc->instruction_start + proc->pcb.program_counter;
    char buf[MAX_VALUE];
    strncpy(buf, mem->words[addr].value, MAX_VALUE-1);
    buf[MAX_VALUE-1] = '\0';

    // tokenize – include '\r' so Windows/Mac line‐ends get stripped
    char* op = strtok(buf, " \t\r\n");
    if (!op) return -1;
    char* a1 = strtok(NULL, " \t\r\n");
    char* a2 = strtok(NULL, "\r\n");   // rest of line

    // also strip any trailing '\r' from a1 or a2
    if (a1) {
      size_t L = strlen(a1);
      if (L>0 && a1[L-1]=='\r') a1[L-1] = '\0';
    }
    if (a2) {
      size_t L = strlen(a2);
      if (L>0 && a2[L-1]=='\r') a2[L-1] = '\0';
      // now strip leading spaces/tabs
      while (*a2==' ' || *a2=='\t') a2++;
    }

    // 1) semWait
    if (strcmp(op, "semWait")==0 && a1) {
        int mi = get_mutex_index(a1);
        if (mi < 0) {
            fprintf(stderr, "Error: unknown resource '%s'\n", a1);
            return -1;
        }
        if (mc->locked[mi]==0) {
            mc->locked[mi] = 1;
        } else {
            proc->pcb.state = BLOCKED;
            proc->queue_arrival_cycle = global_gui->engine->clock_cycle;
            enqueue(&mc->blocked_queues[mi], proc);
            enqueue(&mc->general_blocked,    proc);
            return -1;
        }

    // 2) semSignal
    } else if (strcmp(op, "semSignal")==0 && a1) {
        int mi = get_mutex_index(a1);
        mc->locked[mi] = 0;
        Process* w = dequeue(&mc->blocked_queues[mi], 0);
        if (w) {
            w->pcb.state = READY;
            enqueue(ready_queue, w);
        }

    // 3) assign x y
    } else if (strcmp(op, "assign")==0 && a1 && a2) {
        // "input"
        if (strcmp(a2, "input")==0) {
          char* in = ask_for_input_dialog("Please enter a value:");
          set_variable(mem,proc,a1,in);
          LOG("assign %s ← \"%s\"", a1, in);
          free(in);
        }
        // "readFile filenameVarOrLiteral"
        else if (strncmp(a2, "readFile",8)==0) {
            char* fn_tok = strtok(a2+8, " \t");
            char fn[MAX_VALUE];
            // resolve variable or literal
            if (fn_tok && get_variable(mem, proc, fn_tok, fn)==0)
                ; // fn has content
            else if (fn_tok)
                strncpy(fn, fn_tok, MAX_VALUE);
            else
                fn[0] = 0;
            // read file
            FILE* f = fopen(fn, "r");
            char content[MAX_VALUE] = "";
            char line[MAX_VALUE];
            while (f && fgets(line, MAX_VALUE, f))
                strncat(content, line, MAX_VALUE-strlen(content)-1);
            if (f) fclose(f);
            set_variable(mem, proc, a1, content);
        }
        // simple literal or variable
        else {
            char tmp[MAX_VALUE];
            if (get_variable(mem, proc, a2, tmp)==0)
                set_variable(mem, proc, a1, tmp);
            else
                set_variable(mem, proc, a1, a2);
        }

    // 4) print x
    } else if (strcmp(op, "print")==0 && a1) {
        char tmp[MAX_VALUE];
        if (get_variable(mem, proc, a1, tmp)==0)
            LOG("%s", tmp);
        else
            LOG("%s", a1);

    // 5) writeFile x y
    } else if (strcmp(op, "writeFile")==0 && a1 && a2) {
        char fn[MAX_VALUE], data[MAX_VALUE];
        // filename
        if (get_variable(mem, proc, a1, fn)!=0)
            strncpy(fn, a1, MAX_VALUE);
        // data (could be var or literal)
        char* data_tok = strtok(a2, " \t");
        if (data_tok && get_variable(mem, proc, data_tok, data)==0)
            ;
        else if (data_tok)
            strncpy(data, data_tok, MAX_VALUE);
        else
            data[0]=0;
        FILE* f = fopen(fn, "w");
        if (f) {
            fputs(data, f);
            fclose(f);
        }

    // 6) printFromTo x y
    } else if (strcmp(op, "printFromTo")==0 && a1 && a2) {
        char s1[MAX_VALUE], s2[MAX_VALUE];
        if (get_variable(mem, proc, a1, s1)!=0) strncpy(s1, a1, MAX_VALUE);
        char* ytok = strtok(a2, " \t");
        if (!(ytok && get_variable(mem, proc, ytok, s2)==0))
            strncpy(s2, ytok? ytok : "", MAX_VALUE);
        int v1 = atoi(s1), v2 = atoi(s2);
        for (int i = v1; i <= v2; i++)
            LOG("%d", i);

    } else {
        // unknown op
        return -1;
    }

    // advance PC + quantum count
    proc->pcb.program_counter++;
    proc->quantum_used++;

    // check for termination
    if (proc->pcb.program_counter >= proc->instruction_count)
        return 0;   // terminated

    return 1;       // ran one instruction
}