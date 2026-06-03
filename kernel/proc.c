#include "console.h"
#include "proc.h"
#include "string.h"
#include "user.h"

static struct proc procs[NPROC];
static int next_pid = 1;
static int current_pid;
static sched_algo_t scheduler = SCHED_RR;

static const char *state_name(proc_state_t s) {
    switch (s) {
    case PROC_READY: return "ready";
    case PROC_RUNNING: return "running";
    case PROC_BLOCKED: return "blocked";
    case PROC_ZOMBIE: return "zombie";
    default: return "unused";
    }
}

void proc_init(void) {
    memset(procs, 0, sizeof(procs));
    procs[0].pid = next_pid++;
    procs[0].ppid = 0;
    procs[0].state = PROC_RUNNING;
    strcpy(procs[0].name, "kernel");
    current_pid = procs[0].pid;
}

struct proc *proc_current(void) {
    return proc_find(current_pid);
}

struct proc *proc_find(int pid) {
    for (int i = 0; i < NPROC; i++) {
        if (procs[i].state != PROC_UNUSED && procs[i].pid == pid) {
            return &procs[i];
        }
    }
    return NULL;
}

int proc_spawn(const char *name, program_entry_t entry, int argc, char **argv, int parent) {
    for (int i = 0; i < NPROC; i++) {
        if (procs[i].state == PROC_UNUSED) {
            procs[i].pid = next_pid++;
            procs[i].ppid = parent;
            procs[i].state = PROC_READY;
            strncpy(procs[i].name, name, PROC_NAME_MAX - 1);
            procs[i].entry = entry;
            procs[i].argc = argc > 8 ? 8 : argc;
            for (int a = 0; a < procs[i].argc; a++) {
                strncpy(procs[i].arg_storage[a], argv[a], sizeof(procs[i].arg_storage[a]) - 1);
                procs[i].arg_storage[a][sizeof(procs[i].arg_storage[a]) - 1] = 0;
                procs[i].argv[a] = procs[i].arg_storage[a];
            }
            procs[i].exit_code = 0;
            procs[i].runtime_ticks = 0;
            procs[i].quantum_left = PROC_QUANTUM_MS;
            return procs[i].pid;
        }
    }
    return -1;
}

int proc_fork(int pid) {
    struct proc *src = proc_find(pid);
    if (!src || !src->entry) {
        return -1;
    }
    int parent = proc_current() ? proc_current()->pid : src->pid;
    return proc_spawn(src->name, src->entry, src->argc, src->argv, parent);
}

void proc_exit(int pid, int code) {
    struct proc *p = proc_find(pid);
    if (!p || p->state == PROC_UNUSED) {
        return;
    }
    p->exit_code = code;
    p->state = PROC_ZOMBIE;
}

int proc_wait(int parent, int child) {
    for (;;) {
        struct proc *p = proc_find(child);
        if (!p || p->ppid != parent) {
            return -1;
        }
        if (p->state == PROC_ZOMBIE) {
            int code = p->exit_code;
            memset(p, 0, sizeof(*p));
            return code;
        }
        proc_run_ready();
    }
}

int proc_kill(int pid) {
    struct proc *p = proc_find(pid);
    if (!p || p->pid == 1) {
        return -1;
    }
    p->state = PROC_ZOMBIE;
    p->exit_code = -9;
    return 0;
}

void proc_tick(void) {
    struct proc *p = proc_current();
    if (!p) {
        return;
    }
    p->runtime_ticks++;
    if (scheduler == SCHED_RR && p->quantum_left > 0) {
        p->quantum_left--;
    }
}

void proc_run_ready(void) {
    for (int i = 0; i < NPROC; i++) {
        struct proc *p = &procs[i];
        if (p->state != PROC_READY || !p->entry) {
            continue;
        }
        int prev = current_pid;
        current_pid = p->pid;
        p->state = PROC_RUNNING;
        p->quantum_left = PROC_QUANTUM_MS;
        user_enter(user_start, p->user_stack + USER_STACK_SIZE, p->entry, p->argc, p->argv);
        if (p->state == PROC_RUNNING) {
            p->exit_code = 0;
            p->state = PROC_ZOMBIE;
        }
        current_pid = prev;
        if (scheduler == SCHED_FCFS) {
            return;
        }
    }
}

void proc_set_scheduler(sched_algo_t algo) {
    scheduler = algo;
}

sched_algo_t proc_scheduler(void) {
    return scheduler;
}

void proc_dump(void) {
    printf("pid  ppid state    ticks name\n");
    for (int i = 0; i < NPROC; i++) {
        if (procs[i].state != PROC_UNUSED) {
            printf("%d   %d    %s   %u   %s\n",
                   procs[i].pid, procs[i].ppid, state_name(procs[i].state),
                   procs[i].runtime_ticks, procs[i].name);
        }
    }
}

void sem_init(struct semaphore *sem, int value) {
    sem->value = value;
}

void sem_wait(struct semaphore *sem) {
    while (sem->value <= 0) {
        proc_run_ready();
    }
    sem->value--;
}

void sem_post(struct semaphore *sem) {
    sem->value++;
}

void mutex_init(struct mutex *m) {
    m->locked = false;
    m->owner = 0;
}

void mutex_lock(struct mutex *m) {
    int pid = proc_current() ? proc_current()->pid : 0;
    while (m->locked && m->owner != pid) {
        proc_run_ready();
    }
    m->locked = true;
    m->owner = pid;
}

void mutex_unlock(struct mutex *m) {
    int pid = proc_current() ? proc_current()->pid : 0;
    if (m->owner == pid) {
        m->locked = false;
        m->owner = 0;
    }
}
