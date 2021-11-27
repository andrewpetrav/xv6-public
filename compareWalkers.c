#include "types.h"
#include "fcntl.h"
#include "stat.h"
#include "fs.h"
#include "user.h"

int main(int argc, char **argv){
	printf(1,"DIRECTORY WALKER\n");
	directoryWalker(".");
	printf(1,"INODE WALKER\n");
	inodeTBWalker();

	exit();
	return 0;
}
