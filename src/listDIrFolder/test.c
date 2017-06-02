#include <stdio.h>

void argFunc(int argc, char** ap_argv)
{
	printf("%s, %s, %s\n", ap_argv[0], ap_argv[1], ap_argv[2]);
}

int main()
{
	int argc = 3;
    char *ap_argv[] = {"client", "127.0.0.1", "11800"};

    argFunc(argc, ap_argv);

    return 0;
}