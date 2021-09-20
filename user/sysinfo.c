// Create a zombie process that
// must be reparented at exit.

#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int main(void)
{

  struct sysinfo info;
  sysinfo(&info);
  fprintf(2, "sysinfo mem free:%d mem used:%d proc run %d proc all:%d\n", info.mem_free, info.mem_used, info.proc_run_count, info.proc_count);

  exit(0);
}
