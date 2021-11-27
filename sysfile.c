//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"
#include "buf.h"



// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.

//Added
void directoryWalkerSub(char* path);
void strcat(char* st1, const char* st2); //string 1 and string 2
int strcmp(const char* st1, char* st2); //string 1 and string 2
int sys_directoryWalker(void);
int sys_inodeTBWalker(void);
void fixFile(int inumber);
void reverse(char input[]);
void itoa(int n, char input[]);
void fixDir(int index, int pIndex); //directory's index and parent's index
void resetArrays();


struct superblock sb;
int corDirs[200]; //corrupted directories
int inodeLink[200]; //for directoryWalker
int inodeTBWalkerLink[200]; //for TBWalker
	

void fixDir(int idx, int pdx){
	begin_op();
	struct inode* dirPtr=igetCaller(idx); //ptr to dir
	struct inode* prntPtr=igetCaller(pdx); //ptr to parent
	dirlink(dirPtr,".",dirPtr->inum); //link
	dirlink(dirPtr,"..", prntPtr->inum); //link up 2 levels
	end_op();
}

void fixFile(int i){
	begin_op();
	char name[14]={0};
	strcat(name,"temp_");
	char temp[4];
	itoa(i, temp);
	strcat(name,temp);
	struct inode* ip=igetCaller(1);
	ilock(ip);
	dirlink(ip,name,i);
	iunlock(ip);
	end_op();
}

void reverse(char original[]){ //does what the method name implies
	char reversed;
	int i,j;
	for(i=0, j=strlen(original)-1;i<j; i++, j--){
		reversed=original[i];
		original[i]=original[j];
		original[j]=reversed;
	}
}

void itoa(int n, char input[]){ 
	int positive=n; //if <0, then negative (false)
	if(n<0){
		n=-n; //make pos
	}
	int i=0; //init and declare
	do{ //gen digits in reversed order
		input[i++]=n%10+'0'; //get the next digit
	}while((n/=10)>0); //delete
	if(positive<0){
		input[i++]='-'; //then negate
	}
	input[i]='\0'; //add null term
	reverse(input); //reverse it

}

void strcat(char* s1, const char* s2){
	while(*s1){
		s1++;
	}
	while(*s2){
		*s1++=*s2++;
	}
	*s1=0;
}

int strcmp(const char* s1, char* s2){
	for(; *s1 && *s2 && *s1==*s2; s1++, s2++);
	return *(unsigned char*)s1-*(unsigned char*)s2;
}

void resetArrays(){
	//init the link log and the corrupt dir log
	for(int i=0;i<200;i++){
		inodeLink[i]=0;
		corDirs[i]=0;
	}
}

void directoryWalkerSub(char* path){
	struct inode* ip=namei(path);
	if(!ip){ //if doesn't exist
		panic("INVALID FILE PATH");
	}
	uint offset;
	struct dirent d;
	begin_op();
	ilock(ip); //lock
	if(ip->type==T_DIR){ //if directory
		for(offset=0; offset<ip->size; offset+=sizeof(d)){
			if(readi(ip,(char*)&d,offset,sizeof(d))!=sizeof(d)){
				panic("READING NOT SUCCESSFUL");
			}
			if(offset==0 && (strcmp(d.name,"."))){
				char name[14]="..";
				iunlock(ip);
				struct inode* broken=nameiparent(path,name);
				corDirs[ip->inum]=broken->inum;
				//record inode number of corrupted dir 
				//and its parent's number
				ilock(ip);
			}
			if(d.inum==0){
				continue;
			}
			cprintf("NAME: %s \t INODE NUMBER: %d\n", d.name, d.inum);
			if(strcmp(d.name,".")==0 || strcmp(d.name, "..")==0){
				continue;
			}
			inodeLink[d.inum]++;

			char path2[14]={0};
			strcat(path2, path); //concatenate
			strcat(path2,"/"); 
			strcat(path2,d.name);
			iunlock(ip);
			directoryWalkerSub(path2);
			ilock(ip);
		}

	}
	iunlock(ip);
	end_op();
}

static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *curproc = myproc();

  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd] == 0){
      curproc->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

int
sys_dup(void)
{
  struct file *f;
  int fd;

  if(argfd(0, 0, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

int
sys_read(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return fileread(f, p, n);
}

int
sys_write(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return filewrite(f, p, n);
}

int
sys_close(void)
{
  int fd;
  struct file *f;

  if(argfd(0, &fd, &f) < 0)
    return -1;
  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

int
sys_fstat(void)
{
  struct file *f;
  struct stat *st;

  if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
    return -1;
  return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
int
sys_link(void)
{
  char name[DIRSIZ], *new, *old;
  struct inode *dp, *ip;

  if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
    return -1;

  begin_op();
  if((ip = namei(old)) == 0){
    end_op();
    return -1;
  }

  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  end_op();

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  end_op();
  return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}

//PAGEBREAK!
int
sys_unlink(void)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ], *path;
  uint off;

  if(argstr(0, &path) < 0)
    return -1;

  begin_op();
  if((dp = nameiparent(path, name)) == 0){
    end_op();
    return -1;
  }

  ilock(dp);

  // Cannot unlink "." or "..".
  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  if(ip->type == T_DIR && !isdirempty(ip)){
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();

  return 0;

bad:
  iunlockput(dp);
  end_op();
  return -1;
}

static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;
  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && ip->type == T_FILE)
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }

  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);

  return ip;
}

int
sys_open(void)
{
  char *path;
  int fd, omode;
  struct file *f;
  struct inode *ip;

  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  end_op();

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
  return fd;
}

int
sys_mkdir(void)
{
  char *path;
  struct inode *ip;

  begin_op();
  if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_mknod(void)
{
  struct inode *ip;
  char *path;
  int major, minor;

  begin_op();
  if((argstr(0, &path)) < 0 ||
     argint(1, &major) < 0 ||
     argint(2, &minor) < 0 ||
     (ip = create(path, T_DEV, major, minor)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_chdir(void)
{
  char *path;
  struct inode *ip;
  struct proc *curproc = myproc();
  
  begin_op();
  if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);
  if(ip->type != T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  iput(curproc->cwd);
  end_op();
  curproc->cwd = ip;
  return 0;
}

int
sys_exec(void)
{
  char *path, *argv[MAXARG];
  int i;
  uint uargv, uarg;

  if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv))
      return -1;
    if(fetchint(uargv+4*i, (int*)&uarg) < 0)
      return -1;
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    if(fetchstr(uarg, &argv[i]) < 0)
      return -1;
  }
  return exec(path, argv);
}

int
sys_pipe(void)
{
  int *fd;
  struct file *rf, *wf;
  int fd0, fd1;

  if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
    return -1;
  if(pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      myproc()->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  fd[0] = fd0;
  fd[1] = fd1;
  return 0;
}

//Added
int sys_compareWalkers(){
	resetArrays();
	cprintf("DIRECTORY WALKER\n");
	directoryWalkerSub(".");
	cprintf("INODE TABLE WALKER\n");
	sys_inodeTBWalker();
	
	int same=1; //holds if the walkers are the same or not
	for(int i=2; i<200; i++){ //start at 2 bc roots will be same
		if(inodeLink[i]!=inodeTBWalkerLink[i]){ //if diff
			same=0; //not the same bc there's a difference
			cprintf("DIFFERENCE FOUND AT INODE %d\n",i);
			cprintf("INODE LINKS: %d \t DIRECTORY WALKER LINKS: %d\n", inodeLink[i], inodeTBWalkerLink[i]);
		}
	}
	for(int i=2; i<200; i++){
		if(corDirs[i]!=0){
		cprintf("CORRUPTED DIRECTORY AT DIRECTORY %d\n",i);
		}
	}
	return same;
}

int sys_directoryWalker(void){
	char *path;
	argstr(0,&path);
	if(!path){
		path="."; //set to default
	}
	cprintf("PATH IS %s\n", path);
	resetArrays();
	directoryWalkerSub(path); //call the sub routine
	return 0;
}

int sys_inodeTBWalker(void){
	for(int i=0; i<200; i++){
		inodeTBWalkerLink[i]=0;
	}
	//declare structs
	struct buf *bp;
	struct dinode *dp;
	readsb(1,&sb); //read the super block
	for(int i=1; i<sb.ninodes;i++){
		bp=bread(1, IBLOCK(i, sb));
		dp=(struct dinode*)bp->data+i%IPB;
		if(dp->type!=0){ //if not a free inode
			cprintf("INODE %d \t TYPE %d\n", i,dp->type);
			inodeTBWalkerLink[i]++; //increment
		}
		brelse(bp);
	}
	return 0;
}

int sys_deleteIData(void){
	int i;
	argint(0,&i);
	callDeleteInFS(i);
	return 0;
}

int sys_recoverFS(){
	for(int i=2;i<200;i++){
		if(corDirs[i]!=0){
			fixDir(i,corDirs[i]); //directory needs fixing
		}
	}
	for(int i=2; i<200; i++){
		if(inodeLink[i]!=inodeTBWalkerLink[i]){
			fixFile(i); //file needs fixing
		}
	}
	//Now clear the arrays now that everything is fixed
	for(int i=0; i<200;i++){
		inodeLink[i]=0;
		corDirs[i]=0;
	}
	return 1;
}

