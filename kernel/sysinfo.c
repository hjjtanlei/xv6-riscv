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
  struct sysinfo *info;
  uint64 addr; // user pointer to struct stat

  if (argaddr(0, &addr) < 0)
    return -1;
  return dosysinfo(addr);
}

int dosysinfo(uint64 addr)
{
  printf("sysinfo addr %d \n", addr);
  struct proc *p = myproc();
  struct sysinfo st;

  // if (copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
  //   return -1;

  return 0;
}
