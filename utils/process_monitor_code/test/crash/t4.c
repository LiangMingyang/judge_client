#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/reg.h>
#include <sys/stat.h>


int main(int argc, char* argv[]){
	int a, b;
	scanf("%d%d", &a, &b);
	printf("%d\n", (a+b)/(a-b));
	printf("%d\n", CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_SYSVSEM|CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID);
    execvp(argv[1], &argv[1]);
	return 0;
}
