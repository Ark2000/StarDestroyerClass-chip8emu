#include <stdio.h>
#define func(...)\
	do {\
		int a;\
		fprintf(stdout, __VA_ARGS__);\
	} while (0)
int main()
{
	func("FUCKYOU");
	return 0;
}