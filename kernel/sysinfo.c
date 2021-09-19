#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "sysinfo.h"

int ksysinfo(uint64 addr)
{
  printf("sysinfo addr %d \n", addr);
  struct proc *p = myproc();
  struct sysinfo st;

  // if (copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
  //   return -1;

  return 0;
}
