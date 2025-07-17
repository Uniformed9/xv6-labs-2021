#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int main(int argc, char *argv[])
{
    char *command[32];
    command[0] = "echo";
    command[1] = 0;
    char *exe = "echo";
    char buf[128];

    int n;
    int k = 0;
    int flag = 0;
    int group = 0;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')

        {
            if (strlen(argv[i]) != 2)
            {
                fprintf(2, "need arguments");
                exit(1);
            }
            if (argv[i][1] == 'n')
            {
                // 判断下一个
                if (argc == i + 1)
                {
                    fprintf(2, "need arguments");
                    exit(1);
                }
                group = atoi(argv[i + 1]);
                i++;
            }
        }
        else
        {
            if (!flag)
            {
                exe = argv[i];
                command[k++] = argv[i];
                flag = 1;
            }
            else
            {
                command[k++] = argv[i];
            }
        }
    }

    // 从标准输入中读取
    if ((n = read(0, buf, sizeof(buf))) != 0)
    {
        
        // buf里面有整个字符串
        int bufstart = 0;

        // key para
        int kk = k;
        int groupcount = 0;
        int exe_flag = 0;
        int marks=0;
        for (int i = 0; i < n; i++)
        {
            // if(i==n-1){
            //     printf("----------");
            //      printf("%c",buf[i]);
            //      printf("----------");
            // }
            if (buf[i] == '"')
            {
                if(marks==0){
                    bufstart=i+1;
                    marks=1;
                    continue;
                }else{
                    marks=0;
                    memmove(buf+i,buf+i+1,n-i-1);
                    n=n-1;
                }
                
            }
            if (buf[i] == ' '||buf[i] == '\n')
            {
                if (group != 0)
                {
                    groupcount++;
                }
                command[kk++] = buf + bufstart;
                buf[i] = '\0';
                bufstart = i + 1;
                if (group && groupcount == group)
                {
                    groupcount = 0;
                    exe_flag = 1;
                }
            }
            else if (buf[i] == '\\')
            {
                if (i + 1 < n)
                {
                    if (buf[i + 1] == 'n' || buf[i + 1] == 't')
                    {
                        if (group != 0)
                        {
                            groupcount++;
                        }
                        command[kk++] = buf + bufstart;
                        buf[i] = '\0';
                        bufstart = i + 2;
                        if (group && groupcount == group)
                        {
                            groupcount = 0;
                            exe_flag = 1;
                        }
                        i++;
                    }
                }
            }
            else if (i == n - 1)
            {
                command[kk++] = buf + bufstart;
                buf[i + 1] = '\0';
            }
            if (i == n - 1)
            {
                exe_flag = 1;
            }
            if (exe_flag)
            {
                exe_flag = 0;

                command[kk] = 0;

                kk = k;
                int pid = fork();
                if (pid == 0)
                {   
                    exec(exe, command);
                }
                else
                {
                    wait(0);
                }
            }
        }
    }
    exit(0);
}
