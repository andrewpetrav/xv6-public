#include "types.h"
#include "stat.h"
#include "syscall.h"
#include "fcntl.h"
#include "user.h"

int main(int argc, char **argv){
	//First, let's compare the walkers
	if(compareWalkers()){
		printf(1, "NO DIFFERENCES FOUND\n"); //if true, no diffs
	}
	//now, change the file system
	mkdir("test");
	printf(1,"CREATED DIRECTORY test INSIDE OF root\n");
	int fd=open("/test/testFile", O_CREATE|O_RDWR); //file descriptor
	//create new file testFile in test that we can read and write 
	if(fd>=0){ //if successful
		printf(1,"CREATED FILE testFile INSIDE OF /test\n");
	}
	write(fd, "This is a test write", 21); //21 bc 20 chars+1 for null term
	close(fd); //close
	//Now we are going to call the directoryWalker to show that it does
	//indeed work
	directoryWalker("test");
	mkdir("test2");
	printf(1, "CREATED DIRECTORY test2 INSIDE OF root\n");
	deleteIData(24); //test1 dir
	deleteIData(26); //test2 dir
	printf(1,"CORRUPTING INODES 24 and 26\n");
	if(compareWalkers()){ //this time we are expecting differences
		printf(1, "ERROR: DIFFERENCES EXPECTED BUT NONE FOUND\n");
		//error message
	}

	//Now we will recover the file system
	recoverFS();
	printf(1,"FILE SYSTEM RECOVERED\n");
	if(compareWalkers()){
		printf(1, "NO DIFFERENCES FOUND, FILE SYSTEM SUCCESSFULLY RECOVERED\n");
	}

	exit();
	return 0;
}
