#include <stdio.h>
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

int main(){
	int i;
	for (i = 0; i < 2; i++) fork();
	printf("good-%d-%d\n", getpid(), getppid());
/*
	int a, b;
	scanf("%d%d", &a, &b);
	printf("%d\n", (a+b)/(a-b));
	
	sleep(10);
*/	return 0;
}
