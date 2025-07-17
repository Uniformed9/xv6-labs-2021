#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


int main(int argc, char *argv[])
{
  
  char*tmp[3];
    tmp[0] = "echo";
    tmp[1] = "hello";
    tmp[2] = 0;
    exec("echo", tmp+1);
    printf("exec error\n");
    exit(0);
}
