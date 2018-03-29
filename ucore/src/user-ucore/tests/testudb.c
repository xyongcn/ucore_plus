#include <stdio.h>
#include <ulib.h>

int testValue = 1;

void testSubFunc() {
    int subPartial = 2;
    cprintf("Test sub func.\n");
}

void testFunc(int arg0, int arg1) {
    int partial = 333;
    cprintf("%x\n", &partial);
    partial = arg0;
    partial = arg1;
    testSubFunc();
    cprintf("Test func.\n");
}

int main(void) {
    cprintf("%d\n", testValue);
    testValue = 2;
    testFunc(111, 222);
    testValue = 3;
    cprintf("%d\n", testValue);
    return 0;
}
