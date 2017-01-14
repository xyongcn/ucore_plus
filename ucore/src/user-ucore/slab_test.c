#include <ulib.h>
#include <stdio.h>
#include <syscall.h>
#include <file.h>

const int max_child = 1000;

int
main(void) {
    int i, n, pid;
    unsigned int time = sys_gettime();
        for (n = 0; n < max_child; n ++) {
            if ((pid = fork()) == 0) {exit(0);}
	    else{wait();}
        }
        cprintf("use %04d msecs.\n", sys_gettime() - time);
    return 0;
}


