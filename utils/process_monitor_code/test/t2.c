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


int do_something(){
    int b[100];
//    int *b = NULL;
    printf("b[0] = %d\n", b[0]);
}

int main(int argc, char *args[]) {
    int argsi, j;
    printf("getuid = %d , getgid = %d\n", getuid(), getgid());
    printf("argv: %d\n", argc);
    for (argsi = 0; argsi < argc; ++argsi) printf("args[%d]: %s\n", argsi, args[argsi]);

    scanf("%d", &j);
    int x, y;
    while (j--) {
        scanf("%d %d", &x, &y);
        fprintf(stderr, "%d\n", x + y);
    }

    FILE *f = fopen("in.txt", "r");
    int aa = 3;
    int bb = aa / 2;
    printf("f = %p\n", f);
    printf("xxx = %d\n", (int)f/(aa - bb - 2));

    return 1;


//    int cp = 1;
//    int *a = NULL;

//    pid_t child = fork();

    int cp = 2;
    int a[10001];
    char *list[10];

    for (j = 0; j < 10; ++j){
        usleep(10000);
        list[j] = malloc(1<<20);
        int ii;
        for (ii = (1<<20)-1; ii>=0; --ii) list[j][ii] = 1;
    }

    for (j = 0; j < 10; ++j){

//        free(list[j]);
    }

    for (j = 0; j < 10; ++j){
        usleep(10000);
        list[j] = malloc(1<<20);
        int ii;
        for (ii = (1<<20)-1; ii>=0; --ii) list[j][ii] = 2;
    }

    for (j = 0; j < 10; ++j){

        free(list[j]);
    }

//    if (child != 0) waitpid(-1, NULL, 0);

//    for (j = 1; j < 100000000; j++) a[argsi]++;
//    sleep(1);

    exit(0);


/*

    int ct[16384];
    printf("[%d] Hello World!\n", getpid());
    if (fork() != 0){
        sleep(1);
        printf("cp = %d\n", cp / (cp - 1));
    } else if (vfork() != 0) {
        printf("a[10000] = %d\n", a[10000]);
    } else if (clone(do_something, ct, CLONE_VM | CLONE_FILES, NULL) != -1) {
        printf("a[100] = %d\n", a[100]);
    } else {
        printf("clone failed.\n");
    }

    exit(0);
    return 0;
*/

}
