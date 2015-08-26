#include <stdio.h>
#include <memory.h>

//#define debug

char *usage =
"usage: oj_cmp file1 file2 AClevel PElevel [tab-width] [case-insensitive]\n\
\n\
argvs:\n\
    file1, file2:\n\
        比较的文件。\n\
    AClevel:\n\
        返回 AC 的比较级别。\n\
    PElevel:\n\
        返回 PE 的比较级别，PElevel 要求总是小于等于 AClevel。\n\
    tab-width:\n\
        设置 tab 字符的视在宽度，默认为4，仅在级别 2 的时候有意义。\n\
        但是如果设成了0，那么 tab 字符在所有级别的比较中都将被忽略。\n\
    case-insensitive:\n\
        如果这个参数不是 0（默认是 0），那么在判 PE 时将开启 \n\
        case-insensitive 模式，即在进入 PE 的判定时将不区分字符大小写。\n\
\n\
比较级别：\n\
    level 忽略字符ASCII码     描述\n\
    0:    (0~32,127)        忽略了所有控制符、空白后，逐字符比较两个文件；\n\
    1:    (0~8,11~31,127)   9、10 号字符以外的控制字符都视而不见，将 9、\n\
                            10、32 号字符看作空白，忽略文件最开始和最后的\n\
                            所有空白，任意两个连续的空白段只要包含的换行符\n\
                            个数相同则认为相同，在这样的情形下比较两个文件；\n\
                            （如 \"a\\nb\" 和 \"  a \\t \\n b \\t\\n\" 和\n\
                            \"\\ta  \\nb\\t\\n\\n \" 双引号中的内容在此模式下\n\
                            都被认为是相同的）\n\
    2:    (0~8,11~31,127)   两个文件只要打印的效果相同则认为相同，9 号字符\n\
                            根据 tab-width 参数考察其视在宽度；\n\
    3:    (0~8,11~31,127)   忽略 9、10 号字符以外的控制字符后，逐字符比较\n\
                            两个文件。\n\
\n\
返回值：\n\
    return  标准输出    含义\n\
    0:      AC        两个文件在 AClevel 下比较的结果是->相同\n\
    1:      PE        两个文件在 AClevel 下比较的结果是->不相同，但是\n\
                      两个文件在 PElevel 下且进入 case-insensitive 模式（如果\n\
                      参数设置了）后的比较结果是->相同\n\
    2:      WA        即便是在 PElevel 且启动了 case-insensitive 模式（如果\n\
                      参数设置了），还是发现两个文件不一样\n\
    3:      *         比较的文件至少有一个打开失败了\n\
    4:      *         参数不合规范\n\
\n\
sample:\n\
    command: oj_cmp f1.txt f2.txt 2 0 4 1\n\
    f1.txt: Case 1: 10\n\
    f2.txt: case 1:10\n\
    command 将返回 1, 且屏幕上会输出 PE 。\n";

/*
本程序中的关键的概念 "块"：
    '\t', ' ', '\n' 都被视作空白字符。
    对于任意一个字符，将此字符和其前面邻近的全部连续空白看作其对应的一个块，
    对于一个块有以下参数：
    c   - 块的关键字符，即块中的最后一个字符
    hd  - 块中是否存在空白部分，1 或 0
    d   - 块中的空白部分中最后一个'\n'后面的空白的视在宽度
    h   - 块中的空白部分中'\n'的个数
    ls  - 当前块的关键字符是否是空白字符，1 或 0
    如果一个块的关键字符不是空白，那么这个块是一个"富块"，否则是"普通块"

    得到一个块的方法：
    可以由当前的块和下一个字符，轻松得到下一个块的参数。根据最新读取到的不
    应该忽略的字符，采取行动：
     9('\t') => ls ? d+=tab_width   : ls=hd=1,d=tab_width,h=0
    32(' ')  => ls ? d++            : ls=hd=d=1,h=0
    10('\n') => ls ? d=0,h++        : ls=hd=h=1,d=0
       *     => ls ? ls=0           : hd=h=d=0

比较级别与处理对策：
    level   名称              忽略字符ASCII码  比较对象   比较时检查的参数
    0:      nonspace_same   (0~32,127)      富块      (c)
    1:      interval_same   (0~8,11~31,127) 富块      (c,hd,h)
    2:      look_same       (0~8,11~31,127) 富块      (c,hd,d,h)
    3:      completely_same (0~8,11~31,127) 块        (c)


*/

const char *res_str[] = {"AC", "PE", "WA"};
int tab_width = 4;
int canuse_case_insensitive = 0;
int case_insensitive = 0;
FILE * f[2];
int c[2], h[2], d[2], hd[2], ls[2];
int nowlevel, minlevel;
static char buf[2][8192];
char ignore[4][128];
//int unread_flag[2];

// 获取一个字符，屏蔽回退标记
#define _fgetc(w) fgetc(f[w]) //(unread_flag[w] ? unread_flag[w] = 0, c[w] : fgetc(f[w]))
// 获取下一个当前级别不应该被忽略的字符
#define igetc(w) { for (c[w] = _fgetc(w); (c[w] >= 0 && c[w]<128 && ignore[nowlevel][c[w]]); c[w] = _fgetc(w)); }
// 将新读的字符对应的块的参数计算好
#define count(w)    switch (c[w]){                                          \
    case  9: if (ls[w]) d[w]+=tab_width;else ls[w]=hd[w]=1,d[w]=tab_width,h[w]=0;   break;  \
    case 32: if (ls[w]) d[w]++;         else ls[w]=hd[w]=d[w]=1,h[w]=0;             break;  \
    case 10: if (ls[w]) d[w]=0,h[w]++;  else ls[w]=hd[w]=h[w]=1,d[w]=0;             break;  \
    default: if (ls[w]) ls[w]=0;        else hd[w]=h[w]=d[w]=0;                     break;  \
}
// 获取下一个块
#define readblock(w) { igetc(w); count(w); }
// 如果有需求，则升级当前的块（若当前的块是普通块而当前比较级别的比较对象是富块，那么获取下一个块，最后一定会得到一个富块）
#define space_together(w) { if (nowlevel < 3) while (ls[w]) readblock(w); }
// 根据当前的比较级别，获取一个新的比较对象
#define read(w) { readblock(w); space_together(w); }

// 小写转大写
#define uppercase(a) ((a) <= 'z' && (a) >= 'a' ? (a) - 'a' + 'A' : (a))
// 判断两个字符是否相等，用来屏蔽 case-insensitive 模式的问题
#define check_equ(a,b) (case_insensitive ? uppercase(a) == uppercase(b) : (a) == (b))
// 判断两个块是否一样
#define _check() (check_equ(c[0],c[1]) && (c[0]>=0&&c[0]<128 ? (nowlevel==1 || nowlevel==2 ? hd[0]==hd[1]&&h[0]==h[1] : 1) && (nowlevel==2 ? d[0]==d[1] : 1) : 1))

#ifdef debug
// 判断两个块是否一样，输出比较细节
#define check() ( printf("nowlevel=%d\nc[0]=%d,h[0]=%d,d[0]=%d,hd[0]=%d,ls[0]=%d\nc[1]=%d,h[1]=%d,d[1]=%d,hd[1]=%d,ls[1]=%d\n\n",nowlevel,c[0],h[0],d[0],hd[0],ls[0],c[1],h[1],d[1],hd[1],ls[1]), _check() )
#else
// 判断两个块是否一样，不输出比较细节
#define check() _check()
#endif


int compare(){
    // 指示是否已经降低了评价严格程度，比较等级未降，但打开了 case-insensitive 模式也算降低了评价
    int down = 0;
    // 初始化块参数，初始设置一开始有空白可以处理文件头的空白带来的不必要的麻烦
    h[0] = h[1] = d[0] = d[1] = 0;
    ls[0] = ls[1] = hd[0] = hd[1] = 1;
//  unread_flag[0] = unread_flag[1] = 0;
    // 开始
    while (1){
        read(0);
        read(1);
        if (!check()){
            if (nowlevel == minlevel && case_insensitive == canuse_case_insensitive) return 2;  // 已经不能降低评价严格程度了
            else {
                down = 1;
                nowlevel = minlevel;
                case_insensitive = canuse_case_insensitive;

                space_together(0);
                space_together(1);

                if (!check()) return 2;
            }
        } else if (c[0] < 0 || c[0] >= 128) break;
    }
    return down;
}

int strtoint(char *s, int *x){
    int i;
    for (*x=i=0; s[i]!=0; i++)
        if (s[i] >= '0' && s[i] <= '9') *x = *x * 10 + s[i] - '0';
        else return 0;
    return 1;
}

int main(int argc, char *argv[]){
    int i, r;
    // 处理运行参数
    if (argc < 5 || argc > 7 ||
    argv[3][0] < '0' || argv[3][0] > '9' || argv[3][1] != 0 ||
    argv[4][0] < '0' || argv[4][0] > '9' || argv[4][1] != 0 ||
    argv[3][0] < argv[4][0] || (argc >= 6 && !strtoint(argv[5], &tab_width))) {
        puts(usage);
        return 4;
    }
    if (argc >= 7 && (argv[6][0] != '0' || argv[6][1] != 0)) canuse_case_insensitive = 1;
    nowlevel = argv[3][0] - '0';
    minlevel = argv[4][0] - '0';
#ifdef debug
    printf("tab_width = %d, canuse_case_insensitive = %d\n", tab_width, canuse_case_insensitive);
#endif
    // 得到忽略字符表
    memset(ignore, 0, sizeof(ignore));
    for (i=0; i<=32; i++) ignore[0][i] = 1;
    for (i=0; i<=8; i++) ignore[1][i] = ignore[2][i] = ignore[3][i] = 1;
    for (i=11; i<=31; i++) ignore[1][i] = ignore[2][i] = ignore[3][i] = 1;
    ignore[0][127] = ignore[1][127] = ignore[2][127] = ignore[3][127] = 1;
    if (tab_width == 0) ignore[1][9] = ignore[2][9] = ignore[3][9] = 1;
    // 打开要比较的文件
    f[0] = fopen(argv[1], "r");
    f[1] = fopen(argv[2], "r");
    if (!f[0] || !f[1]) {
        if (!f[0]) printf("file(%s) not found!\n", argv[1]);
        if (!f[1]) printf("file(%s) not found!\n", argv[2]);
        return 3;
    }
    setvbuf(f[0], buf[0], _IOFBF, 8192);
    setvbuf(f[1], buf[1], _IOFBF, 8192);
    // 开始比较
    r = compare();
    // 输出和返回结果
    printf("%s\n", res_str[r]);
    return r;
}


