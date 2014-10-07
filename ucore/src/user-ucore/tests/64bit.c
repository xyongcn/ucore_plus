/*************************************************************************
	> File Name: tests/64bit.c
	> Author: Mengxing Liu
	> Mail: liumengxing@qq.com 
	> Created Time: 2014年10月07日 星期二 23时34分15秒
 ************************************************************************/
#include <stdio.h>

#define printf cprintf

int main(void)
{
	unsigned long long val = -1;
	void *ptr = (void *)-1;
	printf("%p\n", ptr);

	val = 123456789;
	//	TODO ucore has no access to get input from console now
	//		 this function should be add in some time
	//	scanf("123456789", "%Lx", &val);
	printf("val = %lx\n", val);
	return 0;
}
