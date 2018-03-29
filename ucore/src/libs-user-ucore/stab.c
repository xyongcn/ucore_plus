#include <stab.h>
#include <file.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE                         4096
#define MAXSYMLEN                       10240

off_t nextOffset = 0;

struct Stab {
    uint16_t n_strx;
    uint16_t n_othr;
    uint16_t n_type;
    uint16_t n_desc;
    uint32_t n_value;
} stab[MAXSYMLEN];
int stabn;

#define ELF_MAGIC   0x464C457FU         // "\x7FELF" in little endian

/* file header */
struct elfhdr {
    uint32_t e_magic;     // must equal ELF_MAGIC
    uint8_t e_elf[12];
    uint16_t e_type;      // 1=relocatable, 2=executable, 3=shared object, 4=core image
    uint16_t e_machine;   // 3=x86, 4=68K, etc.
    uint32_t e_version;   // file version, always 1
    uint32_t e_entry;     // entry point if executable
    uint32_t e_phoff;     // file position of program header or 0
    uint32_t e_shoff;     // file position of section header or 0
    uint32_t e_flags;     // architecture-specific flags, usually 0
    uint16_t e_ehsize;    // size of this elf header
    uint16_t e_phentsize; // size of an entry in program header
    uint16_t e_phnum;     // number of entries in program header or 0
    uint16_t e_shentsize; // size of an entry in section header
    uint16_t e_shnum;     // number of entries in section header or 0
    uint16_t e_shstrndx;  // section number that contains section name strings
};

/* program section header */
struct proghdr {
    uint32_t p_type;   // loadable code or data, dynamic linking info,etc.
    uint32_t p_offset; // file offset of segment
    uint32_t p_va;     // virtual address to map segment
    uint32_t p_pa;     // physical address, not used
    uint32_t p_filesz; // size of segment in file
    uint32_t p_memsz;  // size of segment in memory (bigger if contains bss）
    uint32_t p_flags;  // read/write/execute bits
    uint32_t p_align;  // required alignment, invariably hardware page size
};

struct secthdr {
   uint32_t   sh_name;
   uint32_t   sh_type;
   uint32_t   sh_flags;
   uint32_t   sh_addr;
   uint32_t   sh_offset;
   uint32_t   sh_size;
   uint32_t   sh_link;
   uint32_t   sh_info;
   uint32_t   sh_addralign;
   uint32_t   sh_entsize;
};

/* values for Proghdr::p_type */
#define ELF_PT_LOAD                     1

/* flag bits for Proghdr::p_flags */
#define ELF_PF_X                        1
#define ELF_PF_W                        2
#define ELF_PF_R                        4

struct DebugInfo debugInfo[MAXSYMLEN];
int debugInfon = 0;

int
load_icode_cont_read(int fd, void *buf, size_t len) {
    return load_icode_read(fd, buf, len, nextOffset);
}

int
load_icode_read(int fd, void *buf, size_t len, off_t offset) {
    int ret;
    nextOffset = offset + len;
    if ((ret = seek(fd, offset, 0)) != 0) {
        return ret;
    }
    if ((ret = read(fd, buf, len)) != len) {
        return (ret < 0) ? ret : -1;
    }
    return 0;
}

char buf[BUFSIZE];

char symstr[128 * 1024];    // 128KB
struct SymTab symtab[MAXSYMLEN];
int symtabn = 0;

char stabstr[128 * 1024];   // a stabstr of 128KB
char* stabstrTab[MAXSYMLEN];

char shstrBuf[16 * 1024];   // a stabstr of 16KB
char* shstrtab[100];

char loadedFile[256];
char codeLine[128 * 1024];  // maximum code file limitation: 128KB
char* codeLineTab[16 * 1024];   // cant be more than 16*1024 lines
int codeLineN = 0;

struct DebugInfo* lastDebugInfo = 0;

int printCodeLine(char* source, int line) {
    // cprintf("%s %d\n", source, line);
    if(strcmp(source, loadedFile) != 0) {
        cprintf("[%s] not loaded.\n", source);
        return -1;
    }
    if(line > codeLineN)
        return -1;
    cprintf("%4d    %s\n", line, codeLineTab[line]);
    return 0;
}

int printCodeLineByDinfo(struct DebugInfo* p) {
    if(p == 0 || p->type != N_SLINE)
        return -1;
    if( lastDebugInfo == 0 || 
        lastDebugInfo->soStr != p->soStr || 
        lastDebugInfo->func != p->func)
            cprintf("function [%s] at %s:%d\n", p->func->symStr, p->soStr, p->sourceLine);
    lastDebugInfo = p;
    int ret = printCodeLine(p->soStr, p->sourceLine);
    if(ret != 0)
        cprintf("0x%08x\n", p->vaddr);
    return ret;
}

int findStr(char** arr, char* target) {
    for(int i = 0; arr[i]; ++i) 
        if(strcmp(arr[i], target) == 0)
            return i;
    return -1;
}

void findSection(int fd, struct elfhdr* elf, char* target, struct secthdr* header) {
    struct secthdr __header, *tmpheader = &__header;
    off_t shoff;
    int ret;
    for(int i = 0; i < elf->e_shnum; ++i) {
        shoff = elf->e_shoff + sizeof(struct secthdr) * i;
        if ((ret = load_icode_read(fd, tmpheader, sizeof(struct secthdr), shoff)) != 0) 
            return;
        if(strcmp(shstrBuf + tmpheader->sh_name, target) == 0) {
            memcpy(header, tmpheader, sizeof(struct secthdr));
            return;
        }
    }
}

void outpStabInfo() {
    cprintf("%d %d\n", stabn, sizeof(struct Stab));
    for(int i = 0; i < stabn; ++i) {
        cprintf("%6x %6d %08x %6d %s\n", 
            stab[i].n_type, 
            stab[i].n_desc, stab[i].n_value, stab[i].n_strx,
            stabstr + stab[i].n_strx);
    }
}

struct DebugInfo* locateFunction(uint32_t pc) {
    for(int i = 0; i < debugInfon; ++i) {
        if(debugInfo[i].type == N_SLINE && debugInfo[i].vaddr == pc)
            return debugInfo[i].func;
    }
    return 0;
}

struct DebugInfo* findFunc(char* name) {
    for(int i = 0; i < debugInfon; ++i) {
        if( debugInfo[i].type == N_FUN &&
            strcmp(debugInfo[i].symStr, name) == 0) {
            return &(debugInfo[i]);
        }
    }
    return 0;
}

struct DebugInfo* findSymbol(uint32_t pc, char* name) {
    struct DebugInfo* func = locateFunction(pc);
    if(func != 0) {
        // local symbols first
        for(int i = 0; i < debugInfon; ++i) {
            if( debugInfo[i].func != func || debugInfo[i].symStr == 0)
                continue;
            if(debugInfo[i].type == N_LSYM || debugInfo[i].type == N_PSYM)
                if(strcmp(debugInfo[i].symStr, name) == 0)
                    return &(debugInfo[i]);
        }
    }
    for(int i = 0; i < debugInfon; ++i) {
        if( (debugInfo[i].type == N_GSYM || debugInfo[i].type == N_FUN)  && 
            debugInfo[i].symStr != 0 && 
            strcmp(debugInfo[i].symStr, name) == 0)
                return &(debugInfo[i]);
    }
    return 0;
}

struct DebugInfo* findSline(uint32_t pc) {
    struct DebugInfo* ret = 0;
    for(int i = 0; i < debugInfon; ++i) {
        if( debugInfo[i].type == N_SLINE && 
            debugInfo[i].vaddr <= pc) {
            if(ret == 0 || ret->vaddr < debugInfo[i].vaddr)
                ret = &(debugInfo[i]);
        }
    }
    return ret;
}

uint32_t getVaddrByLine(int line) {
    for(int i = 0; i < debugInfon; ++i) {
        if( debugInfo[i].type == N_SLINE && 
            strcmp(loadedFile, debugInfo[i].soStr) == 0 && 
            debugInfo[i].sourceLine == line)
            return debugInfo[i].vaddr;
    }
    return -1;
}

void processSymStr(struct DebugInfo* p) {
    int j;
    for(j = 0; p->symStr[j] && p->symStr[j] != ':'; ++j);
    p->symStr[j] = 0;
    // TODO type specific process needed
}

void buildDebugInfo() {
    int n = 0;
    char* soStr = 0;
    uint32_t funBase = 0;
    struct DebugInfo* func = 0;
    debugInfo[0].symStr = 0;
    for(int i = 0; i < stabn; ++i) {
        switch(stab[i].n_type) {
            // define source file
            case N_SO:
                soStr = stabstr + stab[i].n_strx + 5;
            break;
            // define function
            case N_FUN:
                funBase = stab[i].n_value;
                func = &debugInfo[n];
                debugInfo[n].soStr = soStr;
                debugInfo[n].vaddr = funBase;
                debugInfo[n].type = N_FUN;
                debugInfo[n].func = func;
                debugInfo[n].symStr = stabstr + stab[i].n_strx;
                processSymStr(&debugInfo[n]);
                n++;
            break;
            // line of source
            case N_SLINE:
                debugInfo[n].soStr = soStr;
                debugInfo[n].vaddr = funBase + stab[i].n_value;
                debugInfo[n].type = N_SLINE;
                debugInfo[n].sourceLine = stab[i].n_desc;
                debugInfo[n].func = func;
                debugInfo[n].symStr = 0;
                debugInfo[n].mark = 1;
                if(n > 0 && debugInfo[n-1].type == N_SLINE)
                    debugInfo[n-1].mark = 0;
                n++;
            break;
            // local symbol
            case N_LSYM:
                debugInfo[n].soStr = soStr;
                debugInfo[n].vaddr = stab[i].n_value;
                debugInfo[n].type = N_LSYM;
                debugInfo[n].func = func;
                debugInfo[n].symStr = stabstr + stab[i].n_strx;
                processSymStr(&debugInfo[n]);
                n++;
            break;
            // parameter symbol
            case N_PSYM:
                debugInfo[n].soStr = soStr;
                debugInfo[n].vaddr = stab[i].n_value;
                debugInfo[n].type = N_PSYM;
                debugInfo[n].func = func;
                debugInfo[n].symStr = stabstr + stab[i].n_strx;
                processSymStr(&debugInfo[n]);
                n++;
            break;
            // global symbol
            case N_GSYM:
                debugInfo[n].soStr = soStr;
                debugInfo[n].symStr = stabstr + stab[i].n_strx;
                processSymStr(&debugInfo[n]);
                for(int j = 0; j < symtabn; ++j) {
                    /*
                    cprintf("%s %s %d\n", 
                        symstr + symtab[j].nameIndex, 
                        debugInfo[n].symStr, 
                        (strcmp(symstr + symtab[j].nameIndex, debugInfo[n].symStr)));
                    */
                    if(strcmp(symstr + symtab[j].nameIndex, debugInfo[n].symStr) == 0) {
                        debugInfo[n].vaddr = symtab[j].value;
                        break;
                    }    
                }
                debugInfo[n].type = N_GSYM;
                debugInfo[n].func = func;
                n++;
            break;
        }
    }
    debugInfon = n;
}

struct DebugInfo* loadStab(char* fil) {
    cprintf("Loading debug information from elf...");
    if(sizeof(enum StabSymbol) == sizeof(char)) {
        cprintf("Compiler not compatible? %d\n", sizeof(enum StabSymbol));
        return -1;
    }
    
    int fd = open(fil, O_RDONLY);
    int ret;
    
    struct elfhdr __elf, *elf = &__elf;
    if ((ret = load_icode_read(fd, elf, sizeof(struct elfhdr), 0)) != 0) {
        cprintf("Binary file not found : %s\n", fil);
        return -1;
    }
    
    if (elf->e_magic != ELF_MAGIC) {
        cprintf("Not a valid ELF file\n");
        return -1;
    }

    struct secthdr __header, *header = &__header;
    
    off_t shoff, offset;
    
    // first : get the name string of each section
    shoff = elf->e_shoff + sizeof(struct secthdr) * elf->e_shstrndx;
    if ((ret = load_icode_read(fd, header, sizeof(struct secthdr), shoff)) != 0) 
        return -1;
    
    shoff = header->sh_offset;
    offset = 0;
    load_icode_read(fd, shstrBuf, header->sh_size, shoff);
    shstrtab[0] = 0;
    int i = 0, j = 0;
    for(; i < header->sh_size; ++i) {
        if(shstrBuf[i] == 0)
            shstrtab[++j] = shstrBuf + i + 1;
    }
    shstrtab[j] = 0;
    
    // loab stab str
    findSection(fd, elf, ".stabstr", header);
    shoff = header->sh_offset;
    load_icode_read(fd, stabstr, header->sh_size, shoff);
    for(i = 0, j = 0; i < header->sh_size; ++i) {
        if(stabstr[i] == 0)
            stabstrTab[++j] = stabstr + i + 1;
    }
    stabstr[0] = stabstr[j] = 0;
    
    findSection(fd, elf, ".stab", header);
    shoff = header->sh_offset;
    load_icode_read(fd, stab, header->sh_size, shoff);
    stabn = header->sh_size / sizeof(struct Stab);
    
    // load global syms
    findSection(fd, elf, ".strtab", header);
    shoff = header->sh_offset;
    load_icode_read(fd, symstr, header->sh_size, shoff);
    
    findSection(fd, elf, ".symtab", header);
    shoff = header->sh_offset;
    //cprintf("symtab: %d\n", sizeof(struct SymTab));
    load_icode_read(fd, symtab, header->sh_size, shoff);
    symtabn = header->sh_size / sizeof(struct SymTab);
    close(fd);
    
    // done loading elf file
    
    buildDebugInfo();
    cprintf("Done. %d entries loaded. %d debug information gathered.\n", stabn, debugInfon);
    return debugInfo;
}

void loadCodeFile(char* filename) {
    strcpy(loadedFile, filename);
    int fd = open(filename, O_RDONLY);
    if(fd < 0) {
        cprintf("Load source file %s failed.\n", filename);
        return;
    }
    int ret = read(fd, codeLine, sizeof(codeLine));
    int cnt = 1;
    codeLineTab[1] = codeLine;
    int n = strlen(codeLine);
    for(int i = 0; i < n; ++i) {
        if(codeLine[i] == '\n') {
            codeLineTab[++cnt] = codeLine + i + 1;
            codeLine[i] = 0;
        }
    }
    codeLineTab[cnt] = 0;
    codeLineN = cnt - 1;
    close(fd);
    cprintf("Load source file successful.\n");
}
