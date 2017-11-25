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
#include <string.h>
#include <sys/socket.h> 


int list[10000000];
int n = 1000000;
int child_count;

int cmp(const void *x, const void *y){
    return (*(int *)x) - (*(int *)y);
}

int child(void *arg){
//    child_count++;
//    printf("[%d] starting...\n", (int)arg);
    int *lt = ((int)arg == 1 ? list : &list[n / 2]);
    int len = ((int)arg == 1 ? n / 2 : n - n / 2);
//    if ((int)arg == 1) sleep(1);
//    else sleep(2);
//    printf("[%d] lt = %p, len = %d\n", (int)arg, lt, len);
//    qsort(lt, len, sizeof(int), cmp);
//    qsort(lt, len, sizeof(int), cmp);
    char *tmp = malloc(100);
    int i;
    if ((int)arg == 1) {
//	for (i = 0; i < 5; i++) { 
           tmp = malloc(1024*1024*100);
           memset(tmp, 'a', 1024*1024*100);
           sleep(1);
//        }
    }
    qsort(lt, len, sizeof(int), cmp);
    printf("[%d] ended.\n", (int)arg);
//    child_count++;
    if ((int)arg == 2) {
        int sockfd;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        printf("socket ret: %d\n", sockfd);
        FILE *f = fopen("makefile", "r");
        fscanf(f, "%s", tmp);
        fclose(f);
        printf("%d\n", strlen(tmp));
    }

    fork();

    return (int)arg * 2;
}

int main(){
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    printf("socket ret: %d\n", sockfd);
    
    srand(130213);
    int i;
    for (i = 0; i < n; ++i) list[i] = rand() % n;

    int ct1[16384];
    int ct2[16384];

    child_count = 0;
    clone(child, ct1 + 10000, /*CLONE_VM | CLONE_FILES*/ 0, (void *)1);
    clone(child, ct2 + 10000, CLONE_VM | CLONE_FILES, (void *)2);

    printf("wait child thread end...\n");
//    while (child_count < 4) usleep(100*1000);
    pid_t pid;
    while (pid = waitpid(-1, NULL, __WALL), pid != -1) printf("pid = %d\n", pid);
    printf("all child thread ended...\n");


//    for (i = 0; i < n / 2; ++i) printf("%d ", list[i]);
//    putchar(10);
//    for (i = n / 2; i < n; ++i) printf("%d ", list[i]);
//    putchar(10);

    char tmp[1000];
    FILE *f = fopen("makefile", "r");
    fscanf(f, "%s", tmp);
    fclose(f);
    printf("%d\n", strlen(tmp));

    return 134;

    return 0;
}
