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

int list[100];
int n = 100;
int child_count;
int list_res[100];

int cmp(const void *x, const void *y){
    return (*(int *)x) - (*(int *)y);
}

int child(void *arg){
    int *lt = ((int)arg == 1 ? list : &list[n / 2]);
    int len = ((int)arg == 1 ? n / 2 : n - n / 2);
    qsort(lt, len, sizeof(int), cmp);
    printf("[%d] ended.\n", (int)arg);
}

int main(){
    srand(130213);
    int i, j, ilen, jlen, k;
    for (i = 0; i < n; ++i) list[i] = rand() % (10 * n);

    int ct1[16384];
    int ct2[16384];

    child_count = 0;
    clone(child, ct1 + 10000, CLONE_VM | CLONE_FILES, (void *)1);
    clone(child, ct2 + 10000, CLONE_VM | CLONE_FILES, (void *)2);

    printf("wait child thread end...\n");
    pid_t pid;
    while (pid = waitpid(-1, NULL, __WALL), pid != -1) printf("pid = %d\n", pid);
    printf("all child thread ended...\n");

    i = 0;
    j = n / 2;
    ilen = n / 2;
    jlen = n;
    for (k = 0; k < n; k++) {
        if (i < ilen && (j >= n || list[i] < list[j])) list_res[k] = list[i++];
        else list_res[k] = list[j++];
    }
    for (i = 0; i < n; ++i) printf("%d\n", list_res[i]);
    putchar(10);
    return 0;
}
