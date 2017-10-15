#include <stdio.h>
#include <ulib.h>

int main(void)
{
	__asm__ volatile ("int $14");
	panic("FAIL: T.T\n");
}
