#include "UpStair.hpp"
#define CLOCKS_PERSEC ((clock_t)1000)
int main()
{
	clock_t start_t = clock();
	long long times = DPUpStair(80);
	clock_t end_t = clock();
	printf("��ʱ��%d\t������%lld\n",(end_t-start_t)/CLOCKS_PER_SEC, times);
	getchar();
	return 0;
}