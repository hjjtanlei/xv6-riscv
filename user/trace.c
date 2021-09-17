#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(2, "Usage: trace mask cmd..\n");
        exit(1);
    }

    // get mask (mask is syscall)
    // fork
    // set mask
    // run cmd
    int mask = atoi(argv[1]);
 
    int pid;
    pid = fork();
    if (pid < 0)
    {
        printf("init: fork failed\n");
        exit(1);
    }
     // set mask
    helloos(pid,mask);
    if (pid == 0)
    {

        exec(argv[2], &argv[2]);
        printf("init: exec trace cmd failed\n");
        exit(1);
    }
    fprintf(2, "trace pid %d mask %d\n", pid, mask);

    exit(0);
}
