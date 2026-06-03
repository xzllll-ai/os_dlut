#ifndef PROC_H
#define PROC_H

#include "types.h"

#define NPROC 32
#define PROC_NAME_MAX 24
#define PROC_QUANTUM_MS 10

typedef enum {
    PROC_UNUSED,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_ZOMBIE,
} proc_state_t;

typedef enum {
    SCHED_FCFS,
    SCHED_RR,
} sched_algo_t;

typedef int (*program_entry_t)(int argc, char **argv);

struct proc {
    int pid;
    int ppid;
    proc_state_t state;
    char name[PROC_NAME_MAX];
    program_entry_t entry;
    int argc;
    char *argv[8];
    int exit_code;
    u64 runtime_ticks;
    u64 quantum_left;
};

struct semaphore {
    int value;
};

struct mutex {
    bool locked;
    int owner;
};

void proc_init(void);
int proc_spawn(const char *name, program_entry_t entry, int argc, char **argv, int parent);
void proc_exit(int pid, int code);
int proc_wait(int parent, int child);
int proc_kill(int pid);
struct proc *proc_current(void);
struct proc *proc_find(int pid);
void proc_tick(void);
void proc_run_ready(void);
void proc_set_scheduler(sched_algo_t algo);
sched_algo_t proc_scheduler(void);
void proc_dump(void);
void sem_init(struct semaphore *sem, int value);
void sem_wait(struct semaphore *sem);
void sem_post(struct semaphore *sem);
void mutex_init(struct mutex *m);
void mutex_lock(struct mutex *m);
void mutex_unlock(struct mutex *m);

#endif
