#include "user_lib.h"
#include "util/string.h"
#include "util/types.h"

int main(int argc, char *argv[])
{
	// // char x[] = "hello world";
	// // char y[] = "hello world";
	// // printu("%s\n", x);
	// printu("argc address is %p\n", &argc);
	// // printu("argc val is %d\n", argc);
	// printu("argc val is %0x\n", argc);
	// printu("argv address is %p\n", argv);
	// printu("argv[0] address is %p\n", argv[0]);
	char *new_dir = argv[0];

	printu("\n======== mkdir command ========\n");

	mkdir_u(new_dir);
	printu("mkdir: %s\n", new_dir);

	exit(0);
	return 0;
}
