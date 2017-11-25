/*
    比较两个文件

    返回值：0-对 1-错 2-格式错（将回车算为空白则相同） 3-没找到文件
    行首空白 删
    行尾空白 删
    文件尾空行 删
    行中间空白 留
    中间空行 留
    fgetchar() < 0 认为是文件结束
    空白包括空格' '和tab'  ';      is_space()
    回车包括13和10（每次忽略一次13） lfgetchar()

*/

#include <stdio.h>
#include <assert.h>

FILE * f[2];
int c[2], h[2];
int line_begin[2]={1,1}, have_ln=1;

int t;

inline int is_space(int cc){
    return (cc==' ' || cc=='\t' || (!have_ln&&cc==10));
}

inline int lfgetchar(int n){
    return ((t=fgetc(f[n])) == 13 ? fgetc(f[n]) : t);
}

void dread(int n){
    int isspace;
    h[n]=0;
    while (isspace=is_space(c[n]=lfgetchar(n)), h[n]+=isspace, isspace);
    if (c[n]==10)
    {
        h[n]=0;
        line_begin[n]=1;
        return ;
    }
    if (line_begin[n])
    {
        h[n]=0;
        line_begin[n]=0;
    }

}

inline void read()
{
    dread(0);
    dread(1);
}

inline int not_empty(int n)
{
    while (c[n]==10)
        dread(n);
    if (c[n] < 0)
        return 0;
    return 1;
}

int compare()
{
    do
    {
        while (read(), c[0]==c[1] && (!((h[0]==0)^(h[1]==0))) && c[0] >= 0);

        if (c[0] < 0 || c[1] < 0) break;

        if (c[0] == 10 || c[1] == 10){
            have_ln=0;
            dread(c[1]==10);
            continue;
        }

        return 1;

    }
    while (c[0]>=0 && c[1]>=0);

    if (c[0]<0 && c[1]<0)
        return (have_ln ? 0 : 2);
    if (not_empty(c[0]<0))
        return 1;
    return (have_ln ? 0 : 2);

}

static char buf[2][8192];

int do_compare(char *file1, char *file2)
{
    const char *ststr[] = {"AC", "WA", "PE"};
    f[0] = fopen(file1,"r");
    f[1] = fopen(file2,"r");

    assert(f[0] && f[1]);
    setvbuf(f[0], buf[0], _IOFBF, 8192);
    setvbuf(f[1], buf[1], _IOFBF, 8192);

    int c = compare();
    printf("compare: %s\n", ststr[c]);
    return c;
}

int main(int argc, char *argv[])
{
    return do_compare(argv[2], argv[3]);
}

