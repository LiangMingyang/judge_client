#ifndef __LIMIT_RESULT_H__
#define __LIMIT_RESULT_H__


#define RESULT_NE               0
#define RESULT_TLE              1
#define RESULT_MLE              2
#define RESULT_RE               3
#define RESULT_PLE              4

#define RESULT_INFILE_NOTREADY  5
#define RESULT_OUTFILE_NOTREADY 6
#define RESULT_ERRFILE_NOTREADY 7
#define RESULT_OTHER_ERROR      8



struct run_limit_record{
    int time_limit;
    int mem_limit;
    int process_limit;
};

struct run_result_record{
    int result;
    int error_signal;       // 为负值或0则表示调用了被禁止的系统调用，正值则为导致运行时错误的信号
    long long time_used;
    long long mem_used;
    int exitcode;
    int process_used;
    int syscall_count;
};


void init_run_limit(struct run_limit_record *limit);
void print_run_limit(struct run_limit_record *limit, FILE *f);

void init_run_result(struct run_result_record *res);
void print_run_result(struct run_result_record *res, FILE *f);

#endif
