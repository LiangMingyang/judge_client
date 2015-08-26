#include "process_block.h"
#include <stdlib.h>
#include <stdio.h>

void set_pool_size(struct process_block_pool *tbp, int _cap){
    tbp->cap = _cap;
    tbp->count = tbp->live_count = 0;
    tbp->list = (struct process_block *)malloc(sizeof(struct process_block) * _cap);
    int i;
    for (i = 0; i < _cap; ++i) {
        tbp->list[i].pid = 0;
        tbp->list[i].insyscall = 0;
        tbp->list[i].ru_maxrss = 0;
        tbp->list[i].ended = 0;
        tbp->list[i].exitcode = 0;
        tbp->list[i].type = 0;
        tbp->list[i].own_vm = 0;
    }
}

struct process_block* search_block(struct process_block_pool *tbp, pid_t pid){
    int i;
    for (i = 0; i < tbp->count; ++i)
        if (tbp->list[i].pid == pid) return &(tbp->list[i]);
    return NULL;
}

struct process_block* insert_pool(struct process_block_pool *tbp, pid_t pid, int *newp){
    struct process_block* ret = search_block(tbp, pid);
    *newp = 0;
    if (ret == NULL){
        *newp = 1;
        if (tbp->count >= tbp->cap) return NULL;
        tbp->list[tbp->count].pid = pid;
        tbp->list[tbp->count].insyscall = 0;
        tbp->list[tbp->count].ended = 0;
        tbp->list[tbp->count].type = -1;
        tbp->list[tbp->count].own_vm = 0; // 默认与父进程共享地址空间，直到被发现有独立的地址空间时，再计算其内存消耗
        tbp->list[tbp->count].setuped = 0;
        ret = &(tbp->list[tbp->count]);
        tbp->count++;
        tbp->live_count++;
    }
    return ret;
}

int del_from_pool(struct process_block_pool *tbp, pid_t pid){
    struct process_block* tb = search_block(tbp, pid);
    if (tb == NULL) return -1;
    if (tb->ended == 0) return -2;
    *tb = tbp->list[tbp->count - 1];
    tbp->count--;
    return 0;
}

void print_pool(struct process_block_pool *tbp, FILE* f){
    fprintf(f, "\n-----------------\n");
    fprintf(f, "----Cap:        %d\n", tbp->cap);
    fprintf(f, "----count:      %d\n", tbp->count);
    fprintf(f, "----live_count: %d\n", tbp->live_count);
    int i;
    for (i = 0; i < tbp->count; ++i){
        fprintf(f, "----Process[%d]:\n", i);
        fprintf(f, "--pid:       %d\n", tbp->list[i].pid);
        fprintf(f, "--insyscall: %d\n", tbp->list[i].insyscall);
        fprintf(f, "--ru_maxrss: %d\n", tbp->list[i].ru_maxrss);
        fprintf(f, "--ended:     %d\n", tbp->list[i].ended);
        fprintf(f, "--exitcode:  %d\n", tbp->list[i].exitcode);
        fprintf(f, "--type:      %d\n", tbp->list[i].type);
        fprintf(f, "--own_vm:    %d\n", tbp->list[i].own_vm);
        fprintf(f, "--setuped:   %d\n", tbp->list[i].setuped);
    }
}
