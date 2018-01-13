#include <stdio.h>
#include <criterion/criterion.h>
#include "cachelab.h"

/*
int main()
{
    printSummary(0, 0, 0);
    return 0;
}
*/

Test(t, sim){
	unsigned long addr = 0x1234567f9;
	sscanf("1234567f9", "%lX", &addr);
	printf("%ld \n", addr);

	void** ptr;
	sscanf(" 1234567f9", "%p", &ptr);
	printf("%ld \n", (unsigned long)ptr);

	printf(">>%ld \n", sizeof(unsigned long));
}
