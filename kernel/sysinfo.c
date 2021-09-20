#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "sysinfo.h"

uint64
sys_sysinfo(void)
{
  printf("sys_sysinfo\n");

  uint64 addr; // user pointer to struct stat

  if (argaddr(0, &addr) < 0)
    return -1;
  return ksysinfo(addr);
}

int ksysinfo(uint64 addr)
{
  printf("sysinfo addr %d \n", addr);
  struct proc *p = myproc();
  struct sysinfo info;
  info.mem_free = 2;
  info.mem_used = 3;
  info.proc_count = 2;
  info.proc_run_count = 1;
  if (copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
  {
    return -1;
  }

  return 0;
}
