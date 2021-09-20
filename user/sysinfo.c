// Create a zombie process that
// must be reparented at exit.

#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int main(void)
{

  struct sysinfo info;
  sysinfo(&info);
  fprintf(2, "sysinfo mem free:%d proc run %d\n", info.mem_free, info.proc_run_count);

  exit(0);
}
