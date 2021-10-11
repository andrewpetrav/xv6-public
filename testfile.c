#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[]){
	int n=23;
	int traps[n];

	countTraps(traps, n);

	printf(1,"Initial Trap Counts");
	for(int i=1;i<n;i++){
		printf(1, "System Call #%d: %d\n", i, traps[i]);
	}


	printf(1, "Parent Trap Counts");
	//sample calls
	mkdir("ABC");
	int f=open("writefile.txt", O_CREATE | O_WRONLY);
	write(f,"XYZ", strlen("XYZ"));

	//countTraps
	countTraps(traps, n);

	for(int i=1; i<n;i++){
		printf(1, "System Call #%d: %d\n", i, traps[i]);
	}

	if(fork()==0){
		char *c=(char *) malloc(sizeof("XYZ"));
		printf(1, "Child Trap Counts");
		int f2=open("writefile.txt",O_RDONLY);
		read(f2,c,20);
		close(f2);

		countTraps(traps,n);
		for(int i=1; i<n; i++){
			printf(1,"System Call #%d: %d\n", i ,traps[i]);
		}
	}
	wait();
	exit();
}
