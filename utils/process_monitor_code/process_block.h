#ifndef __PROCESS_BLOCK_H__
#define __PROCESS_BLOCK_H__


#include <stdlib.h>
#include <stdio.h>

struct process_block{
    pid_t pid;      // 在进程ID
    int insyscall;  // 0或1，是否在系统调用中
    int ru_maxrss;  // 内存峰值，单位KB
    int ended;      // 0或1或2，结束状态，0：未结束，1：已正常结束（此时exitcode有意义），2：因异常结束
    int exitcode;   // 主函数的返回值
    int type;       // PTRACE_EVENT_FORK 或 PTRACE_EVENT_VFORK 或 PTRACE_EVENT_CLONE，为进程类型
    int own_vm;     // 是否有独立的地址空间
    int setuped;    // 是否设置了 PTRACE_SETOPTIONS
};


struct process_block_pool{
    struct process_block *list;
    int cap;
    int count;
    int live_count;
};


void set_pool_size(struct process_block_pool *tbp, int _cap);

struct process_block* search_block(struct process_block_pool *tbp, pid_t pid);
struct process_block* insert_pool(struct process_block_pool *tbp, pid_t pid, int *newp);
int del_from_pool(struct process_block_pool *tbp, pid_t pid);

void print_pool(struct process_block_pool *tbp, FILE *f);

#endif
