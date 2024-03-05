#include "user_lib.h"
#include "string.h"
#include "util/types.h"

int main(int argc, char *argv[])
{
	printu("\n======== Shell Start ========\n\n");
	printu("hello world\n");
	
	{
		printu("================================\n");
		char path[256];
		memset(path, 0, 256);
		pwd_u(path);
		printu("pwd: %s\n", path);
	}

	{
		printu("================================\n");
		char path[256];
		memset(path, 0, 256);

		cd_u("/RAMDISK0");
		pwd_u(path);
		printu("pwd: %s\n", path);

		cd_u("..");
		pwd_u(path);
		printu("pwd: %s\n", path);

		cd_u("RAMDISK0");
		pwd_u(path);
		printu("pwd: %s\n", path);

		cd_u("...");
		pwd_u(path);
		printu("pwd: %s\n", path);

		cd_u("test");
		pwd_u(path);
		printu("pwd: %s\n", path);
	}
	
	exit(0);
	return 0;
}
