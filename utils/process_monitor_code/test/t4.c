#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/reg.h>
#include <stdlib.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/stat.h>

int list[1000000];
int n = 100;
int child_count;

int cmp(const void *x, const void *y){
    return (*(int *)x) - (*(int *)y);
}

int child(void *arg){
    child_count++;
//    printf("[%d] starting...\n", (int)arg);
    int *lt = ((int)arg == 1 ? list : &list[n / 2]);
    int len = ((int)arg == 1 ? n / 2 : n - n / 2);
    if ((int)arg == 1) sleep(1);
    else sleep(2);
//    printf("[%d] lt = %p, len = %d\n", (int)arg, lt, len);
    qsort(lt, len, sizeof(int), cmp);
    printf("[%d] ended.\n", (int)arg);
}

int main(){
    srand(130213);
    int i;
    for (i = 0; i < n; ++i) list[i] = rand() % 1000;

    child_count = 0;
/*
    if (fork() == 0) {
        child((void *)1);
        exit(0);
    }
*/
    if (fork() == 0) {
        child((void *)2);
        exit(0);
    }

    int ct1[16384];
//    int ct2[16384];

    clone(child, ct1 + 10000, CLONE_VM | CLONE_FILES, (void *)1);
//    clone(child, ct2 + 10000, CLONE_VM | CLONE_FILES, (void *)2);

    printf("wait child thread end...\n");
    pid_t pid;
    while (pid = waitpid(-1, NULL, __WALL), pid != -1) printf("pid = %d\n", pid);
    printf("all child thread ended...\n");


    for (i = 0; i < n; ++i) printf("%d ", list[i]);
    putchar(10);



    return 0;
}
