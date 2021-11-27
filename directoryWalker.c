#include "types.h"
#include "fs.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

int main(int argc, char *argv[]){
	if(argc<2){ //if not enough arguments....
		directoryWalker(".");
		exit();
	}
	directoryWalker(argv[1]);
	exit();
}
