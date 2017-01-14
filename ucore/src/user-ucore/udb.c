#include <stdio.h>
#include <ulib.h>
#include <types.h>
#include <string.h>
#include <dir.h>
#include <file.h>
#include <error.h>
#include <unistd.h>
#include <stab.h>

enum DebugSignal {
    DEBUG_ATTACH = 0,
    DEBUG_WAIT = 1,
    DEBUG_CONTINUE = 2,
    DEBUG_STEPINTO = 3,
    DEBUG_STEPOVER = 4,
    DEBUG_SETBREAKPOINT = 5, 
    DEBUG_DELBREAKPOINT = 6, 
    DEBUG_PRINT = 7, 
    DEBUG_PRINT_REG = 8,
    DEBUG_BACKTRACE = 9,
};

struct DebugProcessInfo {
    int state;
    uint32_t pc;
};

#define printf(...)                     fprintf(1, __VA_ARGS__)
#define putc(c)                         printf("%c", c)

#define BUFSIZE                         4096
#define WHITESPACE                      " \t\r\n"
#define SYMBOLS                         "<|>&;"

#define MAXSYMLEN                       64


// definitions for asmparser

#define NO_INFO 0
#define ASM_CODE 1
#define GCC_CODE 2
#define FUNC_DEF 3

char *empty_string = "(assembly not loaded)";

struct asminfo {
    char type;
    char buf[256];
    int  pos;
};

int is_hex(const char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

int hex_to_int(const char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return 0;
}

/* *
 * asmparse - parse a line in a asm file. 
*/
void
asmparse(const char *line, struct asminfo *info) {
    int i, len;
    int flag;
    info->type = 0;
    info->buf[0] = '\0';
    info->pos = 0;
/*    if (strlen(line) > 10 
            && is_hex(line[0]) 
            && is_hex(line[1])
            && line[9] == '<') {
        info->type = FUNC_DEF;
        for (i = 0; i < 8; i++) {
            info->pos *= 16;
            info->pos += hex_to_int(line[i]);
        }
        for (i = 10; line[i] != '>' && line[i] != '\0'; i++) {
            info->buf[i-10] = line[i];
        }
        info->buf[i-10] = '\0';
        return;
    } else */if (strlen(line) > 10 
            && line[0] == ' '
            && line[1] == ' '
            && is_hex(line[2])
            && line[8] == ':') { 
        info->type = ASM_CODE;
        for (i = 2; i < 8; i++) {
            info->pos *= 16;
            info->pos += hex_to_int(line[i]);
        }
        len = strlen(line);
        strcpy(info->buf, line+30);
    }/* else {
        info->type = GCC_CODE;
        strcpy(info->buf, line);
        return;
    }*/
    return;
}
struct sym_node {
    int pos;
    char *val;
};

char bf[BUFSIZE * 1024];
int bf_n = -1;
// Assembly code
struct sym_node assym[BUFSIZE * 8];
int as_n = 0;
// C code
// struct sym_node gcsym[BUFSIZE * 8];
// int gc_n = 0;
// Function code
// struct sym_node fnsym[BUFSIZE * 8];
// int fn_n = 0;

char *
readl_fd(const char *prompt, int fd) {
    static char buffer[BUFSIZE];
    if (prompt != NULL) {
        printf("%s", prompt);
    }
    int ret, i = 0;
    while (1) {
        char c;
        if ((ret = read(fd, &c, sizeof(char))) < 0) {
            return NULL;
        }
        else if (ret == 0) {
            if (i > 0) {
                buffer[i] = '\0';
                break;
            }
            return NULL;
        }

        if (c == 3) {
            return NULL;
        }
        else if (c >= ' ' && i < BUFSIZE - 1) {
            if(fd == 0)
                putc(c);
            buffer[i ++] = c;
        }
        else if (c == '\b' && i > 0) {
            if(fd == 0)
                putc(c);
            i --;
        }
        else if (c == '\n' || c == '\r') {
            if(fd == 0)
                putc(c);
            buffer[i] = '\0';
            break;
        }
    }
    return buffer;
}

char *
readl_raw(const char *prompt) {
    return readl_fd(prompt, 0);
}

char *
readl(int fd) {
    return readl_fd(NULL, fd);
}

char** split(char* s) {
    static char* ret[EXEC_MAX_ARG_NUM + 1];
    int n = strlen(s);
    int cnt = 0;
    for(int i = 0; i < n; ++i) {
        ret[cnt++] = (s + i);
        for(;s[i] != ' ' && i < n; ++i);
        s[i] = 0;
        for(;s[i] == ' ' && i < n; ++i);
    }
    ret[cnt] = 0;
    return ret;
}

uintptr_t strToInt(char* s) {
    return strtol(s, 0, 0);
}

///////////// utilities upside //////////////////////

uint32_t pid;
struct DebugProcessInfo pinfo;

char* subArgv[EXEC_MAX_ARG_NUM + 1];

#define MAXSYM 256
#define MAXBUF 1024

char buf[MAXBUF];
char target[MAXBUF];
char loadedSource[MAXBUF];

char lastInput[MAXBUF];

struct DebugInfo* info = 0;

void uninit() {
    cprintf("child has exited.\n");
    exit(0);
}

/*

struct Symbol {
    char name[MAXSYMLEN + 1];
    uint32_t addr;
} sym[MAXSYM];
int symn = 0;

void readSym() {
    symn = 0;
    strcpy(buf, target);
    strcat(buf, ".sym");
    int fil = open(buf, O_RDONLY);
    if(fil < 0)
        cprintf("Failed to load sym file : %s.\n", buf);
    char* tmp;
    while(1) {
        tmp = readl(fil);
        if(tmp == 0)
            break;
        char** s = split(tmp);
        if(strlen(s[1]) > MAXSYMLEN) {
            cprintf("Symbol length too long: %d out of %d\n", strlen(s[1]), MAXSYMLEN);
            s[1][MAXSYMLEN] = 0;
        }
        strcpy(sym[symn].name, s[1]);
        sym[symn].addr = strtol(s[0], 0, 16);
        symn ++;
        if(symn == MAXSYM) {
            cprintf("Symbol list is full.\n");
            break;
        }
    }
    close(fil);
    cprintf("Symbol loaded. %d entries found.\n", symn);
}

uintptr_t getSym(char* s) {
    for(int i = 0; i < symn; ++i) {
        if(strcmp(sym[i].name, s) == 0)
            return sym[i].addr;
    }
    return -1;
}
*/
///////////////// parsing asm file ////////////////
//
void load_asm() {
    as_n = 0;// = gc_n = fn_n = 0;
    bf_n = -1;
    struct asminfo info;
    strcpy(buf, target);
    strcat(buf, ".asm");
    int fil = open(buf, O_RDONLY);
    int len, i;
    if(fil < 0)
        cprintf("Failed to load asm file : %s.\n", buf);
    char* tmp;
    while(1) {
        tmp = readl(fil);
        if (tmp == 0) {
            break;
        }
        asmparse(tmp, &info);
/*        if (info.type == FUNC_DEF) {
            fnsym[fn_n].pos = info.pos;
            fnsym[fn_n].val = bf + bf_n + 1;
            strcpy(fnsym[fn_n].val, info.buf);
            bf_n = bf_n + strlen(info.buf) + 1;
            fn_n ++;
        } else */
        if (info.type == ASM_CODE) {
            assym[as_n].pos = info.pos;
            assym[as_n].val = bf + bf_n + 1;
            strcpy(assym[as_n].val, info.buf);
            bf_n = bf_n + strlen(info.buf) + 1;
            // modify pos for c code, so that we know where 
            // in the memory does the c code points to
   //         for (i = gc_n - 1; i >= 0; i--) {
   //             if (gcsym[i].pos == 0) {
   //                 gcsym[i].pos = info.pos;
   //             } else {
   //                 break;
   //             }
   //         }
            as_n ++;
        }/* else if (info.type == GCC_CODE) {
            gcsym[gc_n].pos = 0;
            gcsym[gc_n].val = bf + bf_n + 1;
            strcpy(gcsym[gc_n].val, info.buf);
            bf_n = bf_n + strlen(info.buf) + 1;
            gc_n ++;
        }*/
    }
    close(fil);
}

char *find_asm_by_pos(uint32_t pos) {
    int i;
    for (i = 0; i < as_n; i++) {
        if ((int)pos == assym[i].pos) {
            return assym[i].val;
        }
    }
    return empty_string;
}

uint32_t doSysDebug(int sig, int arg) {
    // printf("[debug]%d\n", sig);
    uint32_t ret;
    if((ret = sys_debug(pid, sig, arg)) == -1)
        uninit();
    return ret;
}

void udbWait() {
    doSysDebug(DEBUG_WAIT, &pinfo);
    if(pinfo.state == -1)
        uninit();
    // cprintf("0x%08x\n", pinfo.pc);
}

int udbContinue(int argc, char* argv[]) {
    doSysDebug(DEBUG_CONTINUE, 0);
    udbWait();
    struct DebugInfo* p = findSline(pinfo.pc);
    printCodeLineByDinfo(p);
}

int udbStepInto(int argc, char* argv[]) {
    struct DebugInfo* p = 0;
    while(!(p && 
            p->vaddr == pinfo.pc)) {
        doSysDebug(DEBUG_STEPINTO, 0);
        udbWait();
        p = findSline(pinfo.pc);
    }
    printCodeLineByDinfo(p);
}

int udbInstStepInto(int argc, char* argv[]) {
    doSysDebug(DEBUG_STEPINTO, 0);
    udbWait();
    cprintf("0x%08x %s\n", pinfo.pc, find_asm_by_pos(pinfo.pc));
    struct DebugInfo* p = findSline(pinfo.pc);
    if(p && p->vaddr == pinfo.pc)
        printCodeLineByDinfo(p);
}

int udbStepOver(int argc, char* argv[]) {
    struct DebugInfo* last = findSline(pinfo.pc);
    if(last == 0) {
        udbInstStepInto(argc, argv);
        return 0;
    }
    if(last->mark == 1) {
        // the last line of function
        udbStepInto(argc, argv);
        return 0;
    }
    struct DebugInfo* p = 0;
    while(!(p && 
            p->vaddr == pinfo.pc && 
            p->func == last->func)) {
        doSysDebug(DEBUG_STEPINTO, 0);
        udbWait();
        p = findSline(pinfo.pc);
    }
    printCodeLineByDinfo(p);
    return 0;
}
/*
int getVaddr(char* s) {
    char* addr = s;
    char* vaddr;
    if(addr[0] == '*') {
        vaddr = strToInt(addr + 1);
    } else {
        vaddr = getSym(addr);
        if(vaddr == -1) {
            cprintf("Cannot find symbol: %s\n", vaddr);
            return -1;
        }
    }
    return vaddr;
}
*/

enum OperatorType {
    OPT_UNO,
    OPT_BINARY,
    OPT_INVALID,
    OPT_VALUE,
    OPT_SYMBOL,
    OPT_LBRACE,
    OPT_RBRACE,
};

struct Operator {
    char* symbol;
    int priority;
    int(*opt)(int a, int b);
    enum OperatorType type;
};

int calcErrFlag = 0;

int calcPlus(int a, int b)  {    return a + b;   }
int calcMinus(int a, int b) {    return a - b;   }
int calcMult(int a, int b)  {    return a * b;   }
int calcDiv(int a, int b)   {    return a / b;   }
int calcMod(int a, int b)   {    return a % b;   }
int calcShl(int a, int b)   {    return a << b;   }
int calcShr(int a, int b)   {    return a >> b;   }
int calcAnd(int a, int b)   {    return a & b;   }
int calcOr(int a, int b)    {    return a | b;   }
int calcPos(int a, int b)   {    return a;   }
int calcNeg(int a, int b)   {    return -a;   }
int calcAddrValue(int a, int b)  {
    uint32_t vaddr = a;
    subArgv[0] = vaddr;
    subArgv[1] = buf;
    subArgv[2] = 0;
    subArgv[3] = 0;
    int result = doSysDebug(DEBUG_PRINT, subArgv);
    if(result < 0)
    {
        cprintf("%s\n", subArgv[1]);
        calcErrFlag = -1;
        return -1;
    }else
        return strToInt(subArgv[1]);
}

static struct Operator operator[] = {
{"+", 10, calcPlus, OPT_BINARY},
{"-", 10, calcMinus, OPT_BINARY},
{"*", 11, calcMult, OPT_BINARY},
{"/", 11, calcDiv, OPT_BINARY},
{"\%", 11, calcMod, OPT_BINARY},
{"<<", 9, calcShl, OPT_BINARY},
{">>", 9, calcShr, OPT_BINARY},
{"&", 8, calcAnd, OPT_BINARY},
{"|", 8, calcOr, OPT_BINARY},
{"+", -1, calcPos, OPT_UNO},
{"-", -1, calcNeg, OPT_UNO},
{"(", -1, 0, OPT_LBRACE},
{")", -1, 0, OPT_RBRACE},
{"*", -1, calcAddrValue, OPT_UNO},
};
#define NOPERATORS (sizeof(operator)/sizeof(struct Operator))

#define MAXCALCSTACK 256

struct Operator calcStack[MAXCALCSTACK];
uint32_t calcValue[MAXCALCSTACK];
int valuen = 0;
int stackn = 0;

char calcBuf[MAXBUF];
enum OperatorType calcMark[MAXCALCSTACK];

char* reservedChars = "`~!@#$%%^&*()+-={}|[]\\:\";'<>?,./";

int findSpecOpt(char* s, enum OperatorType type) {
    for(int j = 0; j < NOPERATORS; ++j) {
        if(strcmp(s, operator[j].symbol) == 0 && operator[j].type == type) {
            return j;
        }
    }
    return -1;
}

int findOpt(char* s) {
    for(int j = 0; j < NOPERATORS; ++j) {
        if(strcmp(s, operator[j].symbol) == 0) {
            return j;
        }
    }
    return -1;
}

void printCalcStack() {
    cprintf("--------\n"); 
    for(int i = 1; i <= stackn; ++i) {
        cprintf("%d ", calcStack[i].type);
    }
    cprintf("\n");
    for(int i = 1; i <= valuen; ++i) {
        cprintf("%d ", calcValue[i]);
    }
    cprintf("\n");
    cprintf("--------\n");
}

void doCalc() {
    if(calcStack[stackn].type == OPT_UNO) {
        calcValue[valuen] = calcStack[stackn].opt(calcValue[valuen], 0);
    } else {
        calcValue[valuen - 1] = calcStack[stackn].opt(calcValue[valuen], calcValue[valuen -1]);
        valuen --;
    }
    stackn --;
}

uint32_t calc(int argc, char* argv[]) {
    calcErrFlag = 0;
    int now = 0;
    for(int i = 1; i < argc; ++i) 
        for(int j = 0; argv[i][j]; ++j)
            buf[now++] = argv[i][j];
    buf[now] = 0;
    cprintf("%s : ", buf);
    now = 0;
    for(int i = 0; buf[i]; ++i) {
        int flag = 0;
        for(int j = 0; j < NOPERATORS; ++j) {
            if(buf[i] == operator[j].symbol[0]) {
                flag = 1;
                calcBuf[now++] = buf[i];
                if(buf[i + 1] == '<' || buf[i + 1] == '>' )
                    break;
                calcBuf[now++] = ' ';
                break;
            }
        }
        if(flag)
            continue;
        if('0' <= buf[i] && buf[i] <= '9') {
            while(  buf[i] == 'x' ||
                    '0' <= buf[i] && buf[i] <= '9' || 
                    'a' <= buf[i] && buf[i] <= 'f' || 
                    'A' <= buf[i] && buf[i] <= 'F' ) {
                calcBuf[now++] = buf[i++];
            }
            calcBuf[now++] = ' ';
            i --;
            continue;
        }
        flag = *strfind(reservedChars, buf[i]);
        if(flag) {
            cprintf("Invalid character: %c\n", buf[i]);
            calcErrFlag = -1;
            return -1;
        }
        while(1) {
            flag = *strfind(reservedChars, buf[i]);
            if(!flag && buf[i])
                calcBuf[now++] = buf[i++];
            else {
                calcBuf[now++] = ' ', i--;
                break;
            }
        }
    }
    calcBuf[now] = 0;
    // cprintf("%s\n", calcBuf);
    char** calcComp = split(calcBuf);
    stackn = 0;
    valuen = 0;
    int m = 0;
    for(int i = 0; calcComp[i]; ++i) {
        m = i;
        int j;
        if((j = findOpt(calcComp[i])) != -1) {
            if(operator[j].type == OPT_LBRACE || operator[j].type == OPT_RBRACE)
                calcMark[i] = operator[j].type;
            else if(i == 0 || 
                    calcMark[i-1] == OPT_UNO || 
                    calcMark[i-1] == OPT_BINARY ||
                    calcMark[i-1] == OPT_LBRACE)
                calcMark[i] = OPT_UNO;
            else
                calcMark[i] = OPT_BINARY;
            continue;
        }
        if('0' <= calcComp[i][0] && calcComp[i][0] <= '9') {
            calcMark[i] = OPT_VALUE;
            continue;
        }
        calcMark[i] = OPT_SYMBOL;
    }
    /*
    for(int i = 0; i <= m; ++i) 
        cprintf("%d ", calcMark[i]);
    cprintf("\n");
    */
    for(int i = m; i >= 0; --i) {
        //printCalcStack();
        int j;
        struct DebugInfo* p;    
        switch(calcMark[i]) {
            case OPT_UNO:
                j = findSpecOpt(calcComp[i], OPT_UNO);
                if(j == -1) {
                    cprintf("Invalid expression!\n");
                    calcErrFlag = -1;
                    return -1;
                }
                calcStack[++stackn] = operator[j];
                doCalc();
            break;
            case OPT_BINARY:
                j = findSpecOpt(calcComp[i], OPT_BINARY);
                if(j == -1) {
                    cprintf("Invalid expression!\n");
                    calcErrFlag = -1;
                    return -1;
                }
                while(  stackn > 0 &&
                        calcStack[stackn].type == OPT_BINARY && 
                        calcStack[stackn].priority > operator[j].priority)
                    doCalc();
                calcStack[++stackn] = operator[j];
            break;
            case OPT_LBRACE:
                while(calcStack[stackn].type != OPT_RBRACE)
                    doCalc();
                stackn--;
            break;
            case OPT_RBRACE:
                calcStack[++stackn].type = OPT_RBRACE;
            break;
            case OPT_VALUE:
                calcValue[++valuen] = strToInt(calcComp[i]);
            break;
            case OPT_SYMBOL:
                p = findSymbol(pinfo.pc, calcComp[i]);
                if(p == 0) {
                    cprintf("Cannot find symbol: %s\n", calcComp[i]);
                    calcErrFlag = -1;
                    return -1;
                }
                uint32_t vaddr = p->vaddr;
                int type = 0;
                if(p->type != N_GSYM)
                    type = 1;
                subArgv[0] = vaddr;
                subArgv[1] = buf;
                subArgv[2] = type;
                subArgv[3] = 0;
                int result = doSysDebug(DEBUG_PRINT, subArgv);
                if(result < 0) {
                    cprintf("%s", subArgv[1]);
                    calcErrFlag = -1;
                    return -1;
                }
                calcValue[++valuen] = strToInt(subArgv[1]);
            break;
        }
    }
    while(stackn) {
        //printCalcStack();
        doCalc();
    }
    cprintf("%d\n", calcValue[1]);
    return calcValue[1];
}

int udbSetBreakpoint(int argc, char* argv[]) {
    int vaddr;
    char* s = argv[1];
    if(argc == 1) {
        vaddr = 0;
    } else if(s[0] == '*') {
        vaddr = strToInt(s + 1);
    } else if('0' <= s[0] && s[0] <= '9') {
        vaddr = getVaddrByLine(strToInt(s));
    } else {
        struct DebugInfo* p = findSymbol(pinfo.pc, s);
        if(p <= 0) {
            cprintf("Cannot find function: %s\n", s);
            return -1;
        }
        vaddr = p->vaddr;
    }
    uint32_t retAddr = doSysDebug(DEBUG_SETBREAKPOINT, vaddr);
    cprintf("Breakpoint set at 0x%x\n", retAddr);
    return 0;
}

int udbPrint(int argc, char* argv[]) {
    if(argc == 1)
        return;
    int result;
    char* s = argv[1];
    uint32_t vaddr;
    // return calc(argc, argv);
    if(s[0] == '$') {
        subArgv[0] = s + 1;
        subArgv[1] = 0;
        result = doSysDebug(DEBUG_PRINT_REG, subArgv);
    } else {
        /*
        struct DebugInfo* p = findSymbol(pinfo.pc, s);
        if(p == 0) {
            cprintf("Cannot find symbol: %s\n", s);
            return -1;
        }
        vaddr = p->vaddr;
        int type = 0;
        if(p->type != N_GSYM)
            type = 1;
        subArgv[0] = vaddr;
        subArgv[1] = buf;
        subArgv[2] = type;
        subArgv[3] = 0;
        result = doSysDebug(DEBUG_PRINT, subArgv);
        */
        calc(argc, argv);
    }
    return 0;
}

int udbList(int argc, char* argv[]) {
    struct DebugInfo* p = findSline(pinfo.pc);
    if(p == 0) {
        cprintf("No source code to be shown.\n");
        return -1;
    }
    int linen = 10;
    if(argc > 1)
        linen = strToInt(argv[1]);
    for(int i = p->sourceLine; 
        i < p->sourceLine + linen && printCodeLine(p->soStr, i) == 0; 
        ++i);
}

int udbQuit(int argc, char* argv[]) {
    cprintf("Quit.\n");
    exit(0);
}

int doHelp(int argc, char* argv[]);

int udbBacktrace(int argc, char* argv[]) {
    subArgv[0] = buf;
    subArgv[1] = 0;
    int result = doSysDebug(DEBUG_BACKTRACE, subArgv);
    char** res = split(buf);
    for(int i = 0; res[i]; i += 2) {
        uint32_t ebp = strToInt(res[i]);
        uint32_t eip = strToInt(res[i + 1]);
        struct DebugInfo* deb = findSline(eip);
        cprintf(
            "0x%08x in %s (ebp=%08x, eip=%08x) at %s:%d\n", 
            eip, deb->func->symStr, ebp, eip, deb->soStr, deb->sourceLine);
    }
}

struct command {
    const char *name;
    const char *alias;
    const char *desc;
    int(*func)(int argc, char **argv);
};

static struct command commands[] = {
    {"help", "h", "Display this list of commands.", doHelp},
    {"continue", "c", "Continue running.", udbContinue},
    {"step", "s", "Step into.", udbStepInto},
    {"stepinst", "si", "Step into (instruction).", udbInstStepInto},
    {"next", "n", "Step over.", udbStepOver},
    {"breakpoint", "b", "Set a breakpoint", udbSetBreakpoint},
    {"list", "l", "List source code", udbList},
    {"print", "p", "Print an expression for once", udbPrint},
    {"backtrace", "bt", "Print a backtrace of the entire stack: one line per frame for all frames in the stack.", udbBacktrace},
    {"quit", "q", "Quit udb", udbQuit},
};

#define NCOMMANDS (sizeof(commands)/sizeof(struct command))

int doHelp(int argc, char* argv[]) {
    cprintf("Usage:\n");
    for(int i = 0; i < NCOMMANDS; ++i) {
        cprintf("%8s(%s)    %s\n", commands[i].name, commands[i].alias, commands[i].desc);
    }
}
//char test[50] = "p +-1+(-2*3+4)-5";
//char test[50] = "p +-1+(-testValue*3+4)-5";
//char test[50] = "p +-1+(-testValue<<1*3+4)-5 | 0x001000";

int main(int argc, char* argv[]) {
    if(argc == 1)
    {
        strcpy(target, "testudb");
    } else
        strcpy(target, argv[1]);
    if ((pid = fork()) == 0) {
        subArgv[0] = target;
        subArgv[1] = NULL;
        sys_debug(0, DEBUG_ATTACH, subArgv);
        exit(0);
    }
    info = loadStab(target);
    strcpy(buf, target);
    strcat(buf, ".c");
    strcpy(loadedSource, buf);
    loadCodeFile(buf);
    // load_asm();
    cprintf("Attached.\n");
    udbWait();
    
    // test begins
    /*
    int con = 0;
    char** testtemp = split(test);
    cprintf("%d\n", con);
    while(testtemp[con]) con++;
    cprintf("%d\n", calc(con, testtemp));
    return 0;
    */
    // test ends
    
    char* inp_raw;
    while(1) {
        inp_raw = readl_raw("udb>");
        if(inp_raw == NULL)
            break;
        if(strlen(inp_raw) == 0)
            inp_raw = lastInput;
        else
            strcpy(lastInput, inp_raw);
        char** inp = split(inp_raw);
        int cnt = 0;
        while(inp[cnt]) cnt++;
        if(cnt == 0)
            continue;
        for(int i = 0; i < NCOMMANDS; ++i) {
            if(strcmp(commands[i].name, inp[0]) == 0 || strcmp(commands[i].alias, inp[0]) == 0) {
                commands[i].func(cnt, inp);
            }
        }
    }
    return 0;
}
