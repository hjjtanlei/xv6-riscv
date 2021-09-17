#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
void main()
{
  if (cpuid() == 0)
  {
    consoleinit();
    printfinit();
    printf("\n");
    printf("xv6 kernel is booting\n");
    printf("\n");
    kinit();            // physical page allocator    ---ANNO
    kvminit();          // create kernel page table
    kvminithart();      // turn on paging             ---ANNO
    procinit();         // process table              --- ANNO 进程内核栈
    trapinit();         // trap vectors
    trapinithart();     // install kernel trap vector
    plicinit();         // set up interrupt controller
    plicinithart();     // ask PLIC for device interrupts
    binit();            // buffer cache
    iinit();            // inode cache
    fileinit();         // file table
    virtio_disk_init(); // emulated hard disk
    userinit();         // first user process    --- ANNO 第一个用户进程 initcode
    __sync_synchronize();
    started = 1;
  }
  else
  {
    while (started == 0)
      ;
    __sync_synchronize();
    printf("hart %d starting\n", cpuid());
    kvminithart();  // turn on paging
    trapinithart(); // install kernel trap vector
    plicinithart(); // ask PLIC for device interrupts
  }

  scheduler(); // 调度器开始执行 --- ANNO
}

/*

trampoline 存在的目的就是在内核与用户切换过程中保持 页表切换后还能继续执行切换代码。要求trampoline 需要在内核页表与用户进程页表映射位置相同

切换过程

1 页表切换
2 sp eip
3 ret （模式切换及跳转到eip）

w_stvec 用于将内核或者用户态的中断处理向量写入
在中断发生时处理器将自动调用。

内核和用户的中断处理程序都在trampoline 定义 
kernelvec
uservec

*/

// 用户系统调用定义usys.pl 生成usys.S