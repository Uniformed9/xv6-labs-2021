#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)]='\0';
  return buf;
}
void find(char *path, char *name)
{
  // 递归查找就好
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, 0)) < 0)
  {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }
  //只有打开一个文件才能制度文件的type,在此之前不知道是目录文件还是普通文件
  if (fstat(fd, &st) < 0)
  {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }
  //printf("----------------\n");
  switch(st.type){
  case T_FILE:
    //printf("file:%s\n", path);
    if(strcmp(fmtname(path),name)==0){
      printf("%s\n", path);
    }
    
    break;

  case T_DIR:
  //printf("dir:%s\n", path);
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, strlen(de.name));
      p[strlen(de.name)] = 0;
      //printf("fmtname:%sand compare:%d\n",fmtname(buf),strcmp(fmtname(buf),"."));
      if( !(strcmp(fmtname(buf),".")==0||strcmp(fmtname(buf),"..")==0)){
        //printf("enter next find %s\n",buf);
        find(buf,name);
      }
      
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc == 2)
  {
    find(".", argv[1]);
  }
  else if (argc != 3)
  {
    fprintf(2, "need 3 parameter");
  }
  find(argv[1],argv[2]);
  exit(0);
}
