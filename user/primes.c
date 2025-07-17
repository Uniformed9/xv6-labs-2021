#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define debug 0
//  if (debug)
//           {

//             if (pid == 0)
//             {
//               char buf[100];
//               int len = itoa(buf, getpid());
//               memcpy(buf + len, "children\n", 10);
//               write(1, buf, len + 10);
//             }
//             else

//             {
//               char buf[100];
//               int len = itoa(buf, getpid());
//               memcpy(buf + len, "father\n", 7);
//               write(1, buf, len + 7);
//             }
//           }
int itoa(char buf[], int num)
{
  int k = num;
  int count = 0;
  int tmp;
  while (k)
  {
    buf[count++] = (k % 10) + '0';
    k /= 10;
  }
  for (int i = 0; i < count / 2; i++)
  {
    tmp = buf[i];
    buf[i] = buf[count - 1 - i];
    buf[count - 1 - i] = tmp;
  }
  // buf[count] = 0;
  return count;
}
void fun(int p[])
{
  // 完成从左边接受数据然后传递给右边？
  int tmp;
  int flag = 0;
  int ne[2];
  int pid;
  int n;
  int prime;
  int prime_flag = 0;
  while ((n = read(p[0], &tmp, sizeof(int))) != 0)
  {
    if (debug)
    {
      printf("pid:%d--read:%d\n", getpid(), tmp);
    }
    if (prime_flag == 0)
    {
      prime_flag = 1;
      prime = tmp;
      printf("prime %d\n", tmp);
    }
    else
    {

      if (tmp % prime != 0)
      {
        // 第一次需要输入到下一个进程；
        if (!flag)
        {
          // 初始化管道和fork
          pipe(ne);
          pid = fork();
         
          
          flag = 1;
        }
        if (flag)
        {
          if (pid == 0)
          {
            // 子进程
            close(ne[1]);
            fun(ne);
          }
          else
          {
            // 父进程
            close(ne[0]);
            write(ne[1], &tmp, sizeof(int));
          }
        }
        // 处理父子进程逻辑
      }
    }
  }
  if (flag)
  {

    if (pid == 0)
    {
      close(ne[0]);
    }
    else
    {
      close(ne[1]);
      wait(0);
    }
  }
  exit(0);
}
int main(int argc, char *argv[])
{
  int p[2];
  pipe(p);
  if (fork() == 0)
  {
    close(p[1]);
    fun(p);
  }
  else
  {
    // 父进程
    close(p[0]);
    for (int i = 2; i <= 35; i++)
    {
      write(p[1], &i, sizeof(int));
    }
    close(p[1]);
  }
  wait(0);
  exit(0);
}
