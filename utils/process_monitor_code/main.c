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
#include <sys/resource.h>
#include "process_block.h"
#include "limit_result.h"

//#define DEBUG 1


// 以下为时间测定相关函数-----------------------------------------------------------
long long get_msec_from_1970(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long ret = (tv.tv_sec * 1000LL + tv.tv_usec / 1000LL);
    return ret;
}

long long get_msec_from(long long start){
    long long end = get_msec_from_1970();
    return end - start;
}

// 以下为监视相关变量和函数----------------------------------------------------------
char **command;                                     // 待监视程序的启动命令行
// 预限定
unsigned long cpu_mask = 0;                         // 绑定的 CPU 核，默认为 0 表示不绑定
int run_uid, run_gid;                               // 运行待监视进程的用户和组
int other_ugid = 0;                                 // 是否指定了其他执行用户和执行组
char *stdin_file = NULL;                            // 待监视程序的标准输入重定向的文件
char *stdout_file = NULL;                           // 待监视程序的标准输出重定向的文件
char *stderr_file = NULL;                           // 待监视程序的标准异常输出重定向的文件
// 执行中限定
int rejected_syscall[1000] = {0};                   // 被禁止的系统调用，默认为都不禁止
int have_rsc = 0;                                   // 是否有被禁止的系统调用
struct run_limit_record run_limit;                  // 运行限制
// 执行状态
struct process_block_pool tbp;                      // 待监视程序的进程池
struct run_result_record res;                       // 待监视程序的状态
pid_t cpid;                                         // 待监视程序主进程号
pid_t tpid;                                         // 计时进程的进程号
int signaled_all_process = 0;                       // 是否已经广播了KILL信号了

int signal_all_process(){
    if (!signaled_all_process) {
#ifdef DEBUG
	fprintf(stderr, "tbp.count=%d\n tpid=%d\n", tbp.count, tpid);
#endif
        int i;
        for (i = 0; i < tbp.count; ++i)
            if (tbp.list[i].ended == 0) kill(tbp.list[i].pid, SIGKILL);
        kill(tpid, SIGKILL);
        signaled_all_process = 1;
    }
}

void setup_pid(pid_t pid, struct process_block *tb) {
    if (tb == NULL || !tb->setuped) {
        ptrace(PTRACE_SETOPTIONS, pid, 0, (PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC));
#ifdef DEBUG
        fprintf(stderr,"\nNow Setup: %d\n", pid);
#endif
    }
    if (tb != NULL) tb->setuped = 1;
    if (pid == cpid && tb != NULL) tb->own_vm = 1;
}

pid_t start_target_program() {
    cpid = fork();
    if (cpid == 0) {
        // 处理标准输入、输出、异常输出的重定向
        if (stdin_file  != NULL) freopen(stdin_file,  "r", stdin);
        if (stdout_file != NULL) freopen(stdout_file, "w", stdout);
        if (stderr_file != NULL) freopen(stderr_file, "w", stderr);
        // 设置绑定的 CPU 核
        if (cpu_mask > 0)
            if (sched_setaffinity(0, sizeof(cpu_mask), &cpu_mask) < 0) exit(1);
        // 设置运行用户和用户组
        if (other_ugid) {
            setgid(run_gid);
            setuid(run_uid);
        }
        // 启动
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        if (execvp(command[0], command) == -1) exit(2);
    }
}

void do_monitor(){
#ifdef DEBUG
    fprintf(stderr,"Main pid is %d, parent is %d\n", getpid(), getppid());
#endif
    // 检查重定向文件--------------------------------------------------------------
    if (stdin_file != NULL && access(stdin_file, R_OK) == -1)
        res.result = RESULT_INFILE_NOTREADY;
    if (stdout_file != NULL && access(stdout_file, F_OK) == 0 && access(stdout_file, W_OK) == -1)
        res.result = RESULT_OUTFILE_NOTREADY;
    if (stderr_file != NULL && access(stderr_file, F_OK) == 0 && access(stderr_file, W_OK) == -1)
        res.result = RESULT_ERRFILE_NOTREADY;
    if (res.result != RESULT_NE) return;

    // 运行待监视程序--------------------------------------------------------------
    start_target_program();
    // 启动计时进程---------------------------------------------------------------
    long long start = get_msec_from_1970();
    tpid = fork();
    if (tpid == 0){
        usleep(run_limit.time_limit * 1000);
        exit(0);
    }
#ifdef DEBUG
    fprintf(stderr,"Timer pid is %d\n", tpid);
#endif
    // 不断获取待监视进程的动态------------------------------------------------------
    struct process_block *tb, *etb;
    int newp;
    int i = 0, stat;
    pid_t wp;
    struct rusage ru;
    tb = insert_pool(&tbp, cpid, &newp);
    setup_pid(cpid, tb);
    while (wp = wait4(-1, &stat, __WALL, &ru), ++i, wp != -1) {
#ifdef DEBUG
        fprintf(stderr,"\nWait[%d]:\n", i);
        fprintf(stderr,"  wp                = %d\n", wp);
        fprintf(stderr,"  errno             = %d\n", errno);

        char binstr[33];
        int j, stat_tmp;
        stat_tmp = stat;
        for (j = 0; j < 32; j++) {
            binstr[31 - j] = (stat_tmp % 2) + '0';
            stat_tmp /= 2;
        }
        binstr[32] = 0;
        fprintf(stderr,"  stat              = %s\n", binstr);

        fprintf(stderr,"  WIFEXITED(stat)   = %d\n", WIFEXITED(stat));
        fprintf(stderr,"  WEXITSTATUS(stat) = %d\n", WEXITSTATUS(stat));
        fprintf(stderr,"  WIFSIGNALED(stat) = %d\n", WIFSIGNALED(stat));
        fprintf(stderr,"  WTERMSIG(stat)    = %d\n", WTERMSIG(stat));
        fprintf(stderr,"  WCOREDUMP(stat)   = %d\n", WCOREDUMP(stat));
        fprintf(stderr,"  WIFSTOPPED(stat)  = %d\n", WIFSTOPPED(stat));
        fprintf(stderr,"  WSTOPSIG(stat)    = %d\n", WSTOPSIG(stat));
        fprintf(stderr,"  WIFCONTINUED(stat)= %d\n", WIFCONTINUED(stat));
        fprintf(stderr,"rusage:\n");
        fprintf(stderr,"  rn.ru_maxrss      = %ld\n", ru.ru_maxrss);
        fprintf(stderr,"  rn.ru_ixrss       = %ld\n", ru.ru_ixrss);
        fprintf(stderr,"  rn.ru_idrss       = %ld\n", ru.ru_idrss);
        fprintf(stderr,"  rn.ru_isrss       = %ld\n", ru.ru_isrss);
        fprintf(stderr,"  rn.ru_minflt      = %ld\n", ru.ru_minflt);
        fprintf(stderr,"  rn.ru_majflt      = %ld\n", ru.ru_majflt);
        fprintf(stderr,"  rn.ru_nswap       = %ld\n", ru.ru_nswap);
        fprintf(stderr,"  rn.ru_inblock     = %ld\n", ru.ru_inblock);
        fprintf(stderr,"  rn.ru_oublock     = %ld\n", ru.ru_oublock);
        fprintf(stderr,"  rn.ru_msgsnd      = %ld\n", ru.ru_msgsnd);
        fprintf(stderr,"  rn.ru_msgrcv      = %ld\n", ru.ru_msgrcv);
        fprintf(stderr,"  rn.ru_nsignals    = %ld\n", ru.ru_nsignals);
        fprintf(stderr,"  rn.ru_nvcsw       = %ld\n", ru.ru_nvcsw);
        fprintf(stderr,"  rn.ru_nivcsw      = %ld\n", ru.ru_nivcsw);
#endif
        if (wp == tpid) {
            // 是计时进程
            if (WIFEXITED(stat)) {
                // 计时器到时了，关闭所有进程
#ifdef DEBUG
                fprintf(stderr,"\n\nTime limit exceed.\n%lld msecond passed.\n", get_msec_from(start));
#endif
                if (res.result == RESULT_NE) res.result = RESULT_TLE;
                res.time_used = run_limit.time_limit;
                signal_all_process();
            } else if (WIFSIGNALED(stat)) {
                // 计时进程被关闭，说明程序在超时前结束了，虽然不一定是正常结束了
#ifdef DEBUG
                fprintf(stderr,"\n\nTime limit NOT exceed.\n%lld msecond passed.\n", get_msec_from(start));
#endif
                res.time_used = get_msec_from(start);
                if (res.time_used > run_limit.time_limit) res.time_used = run_limit.time_limit;
            }
            continue;
        }

        int process_is_ok = 1;

        // 将其加入进程池
        tb = insert_pool(&tbp, wp, &newp);
        setup_pid(wp, tb);

        if (tb == NULL) {
            // 被监视程序进程过多，加入进程池失败
            process_is_ok = 0;
#ifdef DEBUG
            fprintf(stderr, "\nToo many processes, all will be signaled.\n");
#endif
            if (res.result == RESULT_NE) res.result = RESULT_PLE;
        } else {

            if (ru.ru_maxrss > tb->ru_maxrss) {
                // 记录内存峰值，实际内存峰值应该等于所有待监视进程/线程的内存峰值之和
                if (tb->own_vm) res.mem_used += ru.ru_maxrss - tb->ru_maxrss;
                tb->ru_maxrss = ru.ru_maxrss;
            }
            if (res.mem_used > run_limit.mem_limit) {
                // 内存使用量超限
                process_is_ok = 0;
#ifdef DEBUG
                fprintf(stderr,"\n\nMemory limit exceed.\n%lld KB used.\n", res.mem_used);
#endif
                if (res.result == RESULT_NE) res.result = RESULT_MLE;
            }

            if (WIFEXITED(stat)) {
                // 进程自然退出了
#ifdef DEBUG
                fprintf(stderr,"\nProcess %d is really dead.\n", wp);
#endif
                tb->ended = 1;
                tb->exitcode = WEXITSTATUS(stat);
                tbp.live_count--;
                // 记录被监视程序主进程的退出代码
                if (wp == cpid) res.exitcode = tb->exitcode;
            } else if (WIFSIGNALED(stat)) {
                // 被监视程序进程被信号中止了
                // Pascal 和 Java 编译的程序都拦截了 SIGFPE 和 SIGSEGV 信号，Python 甚至拦截了几乎全部的信息
                // 所以对这些语言的程序，监视程序无法截获这两类崩溃并返回 Runtime Error
                process_is_ok = 0;
#ifdef DEBUG
                fprintf(stderr,"\nProcess %d is crashed with %d.\n", wp, WTERMSIG(stat));
#endif
                tb->ended = 2;
                tbp.live_count--;

                if (res.result == RESULT_NE) {
                    res.result = RESULT_RE;
                    res.error_signal = WTERMSIG(stat);
                }
            } else if (WIFSTOPPED(stat)) {
                // 程序暂停了，分情况讨论
                int stopsig = WSTOPSIG(stat);
                if (stopsig == SIGTRAP){
#ifdef DEBUG
                    fprintf(stderr,"\nEVENT_ID[(stat>>16)&0xffff] = %d\n", (stat >> 16) & 0xffff);
#endif
                    int ev = ((stat >> 16) & 0xffff);
                    // 若 ev 是以下 switch 的六种情况，则是 ptrace 引发的事件
#ifdef DEBUG
                    char *msg = NULL;
                    switch (ev){
                        case PTRACE_EVENT_FORK:         // wp 进程/线程想要调用 fork
                            msg = "PTRACE_EVENT_FORK";
                            break;
                        case PTRACE_EVENT_VFORK:        // wp 进程/线程想要调用 vfork
                            msg = "PTRACE_EVENT_VFORK";
                            break;
                        case PTRACE_EVENT_CLONE:        // wp 进程/线程想要调用 clone
                            msg = "PTRACE_EVENT_CLONE";
                            break;
                        case PTRACE_EVENT_EXIT:         // wp 进程/线程想要结束
                            msg = "PTRACE_EVENT_EXIT";
                            break;
                        case PTRACE_EVENT_VFORK_DONE:   // wp 进程/线程从 vfork 的借壳状态脱离
                            msg = "PTRACE_EVENT_VFORK_DONE";
                            break;
                        case PTRACE_EVENT_EXEC:         // wp 进程/线程想要调用 exec
                            msg = "PTRACE_EVENT_EXEC";
                            break;
                    }
                    fprintf(stderr,"\nProcess %d %s\n", wp, msg);
#endif
                    int ep;
                    if (ev == PTRACE_EVENT_FORK || ev == PTRACE_EVENT_VFORK || ev == PTRACE_EVENT_CLONE) {
                        ptrace(PTRACE_GETEVENTMSG, wp, 0, &ep);
#ifdef DEBUG
                        fprintf(stderr,"\nProcess %d will started.\n\n", ep);
#endif
                        etb = insert_pool(&tbp, ep, &newp);
                        //setup_pid(ep, etb);
                        if (etb == NULL) {
                            // 被监视程序进程/线程过多
                            process_is_ok = 0;
#ifdef DEBUG
                            fprintf(stderr, "\nToo many processes, all will be signaled.\n");
#endif
                            if (res.result == RESULT_NE) res.result = RESULT_PLE;
                        } else {
                            etb->type = ev;
                            if (ev == PTRACE_EVENT_CLONE) {
                                long clone_flag = ptrace(PTRACE_PEEKUSER, wp, 4 * RBX, 0);
#ifdef DEBUG
                                fprintf(stderr, "\nclone_flag: %ld\n", clone_flag);
#endif
                                //if ((clone_flag & CLONE_VM) == 0) etb->own_vm = 1;
                            } else if (ev == PTRACE_EVENT_FORK){
                                etb->own_vm = 1;
                            } else if (ev == PTRACE_EVENT_VFORK){
                                ;
                            }
                            if (etb->own_vm == 1) {
                                // 刚确认了此进程有自己的地址空间，立刻将其加入到统计量中，因为此时此进程可能已经跑了有一段时间了
                                res.mem_used += etb->ru_maxrss;
                            }
                        }
                    } else if (ev == PTRACE_EVENT_EXEC) { // wp 进程/线程想要执行 exec 系统调用
/*
                        if (tb->own_vm == 0 && tb->type == PTRACE_EVENT_VFORK) {
                            // 曾经没有自己的地址空间，且是以 vfork 形式启动的
                            etb->own_vm = 1;
                            res.mem_used += tb->ru_maxrss;
                        }
*/
                    } else if (ev == PTRACE_EVENT_EXIT) { // wp 进程/线程想要结束
#ifdef DEBUG
                        ptrace(PTRACE_GETEVENTMSG, wp, 0, &ep);
                        fprintf(stderr,"\nProcess %d will be dead with %d.\n\n", wp, ep);
#endif
                    }
                    if (process_is_ok) {
                        // 一切正常
                        // 这个信号仅仅是用来提醒监视者的，不发给子进程
                        ptrace(PTRACE_SYSCALL, wp, 0, 0);
                    }
                } else if (stopsig == (SIGTRAP | 0x80)) {
                    // 此情况为被监视程序调用了系统调用，其中 orig_rax 为调用的系统调用号
                    // Python 和 Java 都包含了大量的 _llseek（140号） 系统调用，Java 还会有大量的 read（不知道在读什么，总是与 _llseek 交替），Python 则是有大量的 open（各种找包）
                    int orig_rax = ptrace(PTRACE_PEEKUSER, wp, 4 * ORIG_RAX, 0);
#ifdef DEBUG
                    fprintf(stderr,"\nSYSCALL_ID[orig_rax] = %d\n", orig_rax);
#endif
                    if ((tb->insyscall = orig_rax - tb->insyscall) != 0)
                        ++res.syscall_count;
                    // 这个系统调用是被禁止的，必须中止
                    if (rejected_syscall[orig_rax]) {
                        process_is_ok = 0;
                        if (res.result == RESULT_NE) {
                            res.result = RESULT_RE;
                            res.error_signal = -orig_rax;
                        }
                    }
                    if (process_is_ok) {
                        // 一切正常
                        // 这个信号仅仅是用来提醒监视者的，不发给子进程
                        ptrace(PTRACE_SYSCALL, wp, 0, 0);
                    }
                } else {
                    // 此时都不是 ptrace 引发的停止（被监视程序自然停止）
#ifdef DEBUG
                    fprintf(stderr,"\nProcess %d stoped with %d\n", wp, stopsig);
#endif
                    if (process_is_ok) {
                        // 一切正常，放行所有信号给子进程，要崩溃请自便（会在 "if (WIFSIGNALED(stat))" 下的逻辑中处理）
                        ptrace(PTRACE_SYSCALL, wp, 0, stopsig);
                    }
                }
            }
        }

        if (!process_is_ok || res.result != RESULT_NE) {
            // 这个进程已经出异常了或者被监视程序已经出异常了
            ptrace(PTRACE_KILL, wp, 0, 0);
            signal_all_process();
        }
        if (tbp.count > 0 && tbp.live_count == 0) {
            // 待监视程序已经全部退出，可以结束计时进程了
#ifdef DEBUG
            fprintf(stderr,"\nAll processes is ended, kill the timer process.\n");
#endif
            kill(tpid, SIGKILL);
        }

#ifdef DEBUG
        fprintf(stderr,"\n[%d]%lld msecond passed. %lld KB memory is used.\n", i, get_msec_from(start), res.mem_used);
        print_pool(&tbp, stderr);
#endif
    }
    res.process_used = tbp.count;
}

int main(int argc, char *args[]){
    int argsi, j, tmp;
#ifdef DEBUG
    fprintf(stderr, "argv: %d\n", argc);
    for (argsi = 0; argsi < argc; ++argsi) fprintf(stderr, "args[%d]: %s\n", argsi, args[argsi]);
    fprintf(stderr, "---------------\n");
#endif
    // 被禁止的系统调用，默认为都不禁止
    memset(rejected_syscall, 0, sizeof(rejected_syscall));
    // 初始化限制参数
    init_run_limit(&run_limit);
    // 初始化运行状态
    init_run_result(&res);
    // 初始化运行被监视程序的用户和组
    run_uid = getuid();
    run_gid = getgid();

    // 以下设置参数---------------------------------------------------------------
    argsi = 1;
    while (argsi < argc) {
        if (strcmp(args[argsi], "-t") == 0) {
            run_limit.time_limit = atoi(args[argsi + 1]);
            if (run_limit.time_limit < 0) run_limit.time_limit = 0;
        } else if (strcmp(args[argsi], "-m") == 0) {
            run_limit.mem_limit = atoi(args[argsi + 1]);
            if (run_limit.mem_limit < 0) run_limit.mem_limit = 0;
        } else if (strcmp(args[argsi], "-p") == 0) {
            run_limit.process_limit = atoi(args[argsi + 1]);
            if (run_limit.process_limit < 1) run_limit.process_limit = 1;
        } else if (strcmp(args[argsi], "-uid") == 0) {
            run_uid = atoi(args[argsi + 1]);
            other_ugid = 1;
        } else if (strcmp(args[argsi], "-gid") == 0) {
            run_gid = atoi(args[argsi + 1]);
            other_ugid = 1;
        } else if (strcmp(args[argsi], "-cm") == 0) {
            cpu_mask = atoi(args[argsi + 1]);
        } else if (strcmp(args[argsi], "-inf") == 0) {
            stdin_file = args[argsi + 1];
        } else if (strcmp(args[argsi], "-outf") == 0) {
            stdout_file = args[argsi + 1];
        } else if (strcmp(args[argsi], "-errf") == 0) {
            stderr_file = args[argsi + 1];
        } else if (strcmp(args[argsi], "-rsc") == 0) {
            tmp = atoi(args[argsi + 1]);
            if (tmp >= 0 && tmp < 1000) {
                rejected_syscall[tmp] = 1;
                have_rsc = 1;
            }
        } else
            break;
        argsi += 2;
    }

    if (argsi >= argc) {
        printf("Usage: %s [options] program [arg1 arg2 ...]\n"
                "Options:\n"
                "  -t TIME_LIMIT                Limit the run time of the program. Unit is ms. \n"
                "  -m MEM_LIMIT                 Limit the peak memory of the program. Unit is KB.\n"
                "  -p PROCESS_LIMIT             Limit the count of the program's child process.\n"
                "  -uid UID                     Call setuid(UID) before execvp. Need root user.\n"
                "  -gid GID                     Call setgid(GID) before execvp. Need root user.\n"
                "  -cm CPU_MASK                 Call sched_setaffinity before execvp.\n"
                "  -inf STDIN_FILE              Redirect the stdin before execvp.\n"
                "  -outf STDOUT_FILE            Redirect the stdout before execvp.\n"
                "  -errf STDERR_FILE            Redirect the stderr before execvp.\n"
                "  -rsc REJECTED_SYSCALL_NUM    When the program call the syscall, it will be\n"
                "                               terminated and return a Runtime Error. This \n"
                "                               optionan be set multiply.\n", args[0]);
        exit(1);
    }
    command = &(args[argsi]);
    if (other_ugid && getuid() != 0) {
        printf("Must run as root for option \"-uid\" and \"-gid\"\n");
        exit(1);
    }
    // 设置进程池大小
    set_pool_size(&tbp, run_limit.process_limit);

    // 以下输出监视设置------------------------------------------------------------
    print_run_limit(&run_limit, stdout);
    if (other_ugid) {
        printf("Run uid:          %d\n", run_uid);
        printf("Run gid:          %d\n", run_gid);
    }
    if (cpu_mask > 0) {
        printf("CPU Mask:         %lu\n", cpu_mask);
    }
    if (stdin_file  != NULL) printf("Stdin Redirect:   %s\n", stdin_file);
    if (stdout_file != NULL) printf("Stdout Redirect:  %s\n", stdout_file);
    if (stderr_file != NULL) printf("Stderr Redirect:  %s\n", stderr_file);
    if (have_rsc) {
        printf("Rejected Syscall:");
        for (j = 0; j < 1000; j++) if (rejected_syscall[j]) printf(" %d", j);
        printf("\n");
    }
    printf("Command:         ");
    for (j = 0; command[j] != NULL; ++j) printf(" %s", command[j]);
    printf("\n");
    printf("-----------------\n");

    // 执行监视-------------------------------------------------------------------
    do_monitor();

    // 输出监视结果---------------------------------------------------------------
    print_run_result(&res, stdout);

    return 0;
}
