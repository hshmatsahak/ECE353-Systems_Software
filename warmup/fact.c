#include "common.h"

int recurse(int val){
	if (val == 0)
		return 1;
	else
		return val*recurse(val-1);
}

int main(int argc, char **argv)
{
	if (argc < 2){
		printf("Huh?\n");
		return 0;
	}
	char *p = argv[1];
	while (*p != '\0'){
		if (*p<'0' || *p>'9'){
			printf("Huh?\n");
			return 0;
		}
		p++;
	}
	int val = atoi(argv[1]);
	if (val<=0){
		printf("Huh?\n");
		return 0;
	}
	if (val>12){
		printf("Overflow\n");
		return 0;
	}
	int ans = recurse(val);
	printf("%d\n", ans);
	return 0;
}
