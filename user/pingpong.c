#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  char buf[512];
  int p[2];
  pipe(p);
  if(fork()==0){
   
    read(p[0],buf,sizeof(buf));
    printf("%d: %s\n",getpid(),buf);
    write(p[1],"received pong",13);
    close(p[0]);
    close(p[1]);
  }else{
    
    write(p[1],"received ping",13);
    wait(0);
    read(p[0],buf,sizeof(buf));
    printf("%d: %s\n",getpid(),buf);
    close(p[0]);
    close(p[1]);
  }
  wait(0);
  exit(0);
}
