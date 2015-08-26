#include <stdio.h>
#include <signal.h>
#include "limit_result.h"

int INIT_TIME_LIMIT = 2000; // msec
int INIT_MEM_LIMIT = 65535; // KB
int INIT_PROCESS_LIMIT = 20;

char *result_str[] = {
    "No Error",                 // 0
    "Time Limit Exceed",        // 1
    "Memory Limit Exceed",      // 2
    "Runtime Error",            // 3
    "Process Limit Exceed",     // 4
    "Input File Not Ready",     // 5
    "Output File Not Ready",    // 6
    "Error File Not Ready",     // 7
    "Other Error"               // 8
};

#define ERROR_SIGNAL_COUNT  31

int error_signal_table[ERROR_SIGNAL_COUNT] = {
    SIGHUP,     //1
    SIGINT,     //2
    SIGQUIT,    //3
    SIGILL,     //4
    SIGTRAP,    //5
    SIGABRT,    //6
    SIGBUS,     //7
    SIGFPE,     //8
    SIGKILL,    //9
    SIGUSR1,    //10
    SIGSEGV,    //11
    SIGUSR2,    //12
    SIGPIPE,    //13
    SIGALRM,    //14
    SIGTERM,    //15
    SIGSTKFLT,  //16
    SIGCHLD,    //17
    SIGCONT,    //18
    SIGSTOP,    //19
    SIGTSTP,    //20
    SIGTTIN,    //21
    SIGTTOU,    //22
    SIGURG,     //23
    SIGXCPU,    //24
    SIGXFSZ,    //25
    SIGVTALRM,  //26
    SIGPROF,    //27
    SIGWINCH,   //28
    SIGIO,      //29
    SIGPWR,     //30
    SIGSYS      //31
};

char *error_signal_str[ERROR_SIGNAL_COUNT] = {
    "SIGHUP",     //1
    "SIGINT",     //2
    "SIGQUIT",    //3
    "SIGILL",     //4
    "SIGTRAP",    //5
    "SIGABRT",    //6
    "SIGBUS",     //7
    "SIGFPE",     //8
    "SIGKILL",    //9
    "SIGUSR1",    //10
    "SIGSEGV",    //11
    "SIGUSR2",    //12
    "SIGPIPE",    //13
    "SIGALRM",    //14
    "SIGTERM",    //15
    "SIGSTKFLT",  //16
    "SIGCHLD",    //17
    "SIGCONT",    //18
    "SIGSTOP",    //19
    "SIGTSTP",    //20
    "SIGTTIN",    //21
    "SIGTTOU",    //22
    "SIGURG",     //23
    "SIGXCPU",    //24
    "SIGXFSZ",    //25
    "SIGVTALRM",  //26
    "SIGPROF",    //27
    "SIGWINCH",   //28
    "SIGIO",      //29
    "SIGPWR",     //30
    "SIGSYS"      //31
};

void init_run_limit(struct run_limit_record *limit){
    limit->time_limit   = INIT_TIME_LIMIT;
    limit->mem_limit    = INIT_MEM_LIMIT;
    limit->process_limit = INIT_PROCESS_LIMIT;
}

void print_run_limit(struct run_limit_record *limit, FILE *f){
    fprintf(f, "Time limit:       %d msec\n",  limit->time_limit);
    fprintf(f, "Mem limit:        %d KB\n",    limit->mem_limit);
    fprintf(f, "Process limit:    %d\n",       limit->process_limit);
}


void init_run_result(struct run_result_record *res){
    res->result = RESULT_NE;
    res->error_signal = 0x7fffffff;
    res->time_used = 0;
    res->mem_used = 0;
    res->exitcode = 0;
    res->process_used = 0;
    res->syscall_count = 0;
}

void print_run_result(struct run_result_record *res, FILE *f){
    fprintf(f, "Run result code:  %d\n", res->result);
    if (res->result == RESULT_RE) {
        if (res->error_signal <= 0) {
            fprintf(f, "Run result:       %s (SYSCALL %d)\n", result_str[res->result], -(res->error_signal));
        } else {
            int i;
            for (i = 0; i < ERROR_SIGNAL_COUNT && error_signal_table[i] != res->error_signal; ++i) ;
            fprintf(f, "Run result:       %s (%s)\n", result_str[res->result], (i < ERROR_SIGNAL_COUNT ? error_signal_str[i] : "Unknow signal"));
        }
    } else {
        fprintf(f, "Run result:       %s\n", result_str[res->result]);
    }

    fprintf(f, "Time used:        %lld\n", res->time_used);
    fprintf(f, "Mem used:         %lld\n", res->mem_used);
    fprintf(f, "Process used:     %d\n",     res->process_used);
    fprintf(f, "SYSCALL count:    %d\n",      res->syscall_count);
    fprintf(f, "Exit code:        %d\n",      res->exitcode);
}
