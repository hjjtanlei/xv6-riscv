#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void trapinithart(void) // 内核发生的中断与异常处理, kernelvec 最终会调用函数kerneltrap， 状态寄存器应该存放在当前进程内核stack,内核栈的设置应该在sched逻辑内（待确认）
{
  w_stvec((uint64)kernelvec);
}

/*

返回到用户态时重新设定usertrapret（其中断函数）  w_stvec(TRAMPOLINE + (uservec - trampoline));
用户态的中断需要进行内核栈切换（用户态信息保存）等动作，比内核要复杂些

进入内核时重新设定   w_stvec((uint64)kernelvec);内核中断处理函数
为何如此设计？

分配user进程时设定了其开始执行forkret
allocproc ->  p->context.ra = (uint64)forkret;
*/

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void usertrap(void)
{
  //printf("usertrap \n");
  int which_dev = 0;

  if ((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  // w_satp(MAKE_SATP(p->pagetable));
  // sfence_vma();

  //printf("usertrap begin pid:%d\n", p->pid);
  // save user program counter.
  p->trapframe->epc = r_sepc();
  //printf("usertrap():  scause %p pid=%d\n", r_scause(), p->pid);
  // printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
  if (r_scause() == 8)
  {
    // system call

    if (p->killed)
    {
      printf("usertrap 8 exit pid:%d\n", p->pid);
      exit(-1);
    }
    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sstatus &c registers,
    // so don't enable until done with those registers.
    intr_on();
    // printf("usertrap syscall pid:%d\n", p->pid);
    syscall();
    //  printf("usertrap():  syscall scause %p pid=%d\n", r_scause(), p->pid);
    //  printf("syscall------cpu %d run pid :%d name:%s \n", cpuid(), p->pid, p->name);
    // printf("usertrap end syscall pid:%d\n", p->pid);
  }
  else if ((which_dev = devintr()) != 0)
  {
    // ok
    // printf("usertrap(): devintr scause %p pid=%d\n", r_scause(), p->pid);
    // printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
  }
  else
  {
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    p->killed = 1;
  }

  if (p->killed)
  {
    printf("usertrap exit pid:%d\n", p->pid);
    exit(-1);
  }
  // give up the CPU if this is a timer interrupt.
  if (which_dev == 2)
    yield();
  // printf("usertrap end pid:%d\n", p->pid);
  usertrapret();
}

//
// return to user space
//
void usertrapret(void)
{

  struct proc *p = myproc();
  //printf("usertrapret------cpu %d run pid :%d name:%s pagetable:%p\n", cpuid(), p->pid, p->name, r_satp());

  //printf("usertrapret pid:%d\n", p->pid);
  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to trampoline.S
  w_stvec(TRAMPOLINE + (uservec - trampoline));
  // printf("usertrapret p:%d \n", p->pid);
  // set up trapframe values that uservec will need when
  // the process next re-enters the kernel.
  p->trapframe->kernel_satp = r_satp();         //MAKE_SATP(p->pagetable); //       //  //r_satp();         // MAKE_SATP(p->pagetable); // MAKE_SATP(p->pagetable); //    // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp(); // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.

  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to trampoline.S at the top of memory, which
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 fn = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64, uint64))fn)(TRAPFRAME, satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void kerneltrap()
{
  // struct proc *p = myproc();
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();

  if ((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if (intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if ((which_dev = devintr()) == 0)
  {
    printf("scause %p\n", scause);
    printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }
  // printf(" -------------kerneltrapcpu %d pagetable:%p \n", cpuid(), r_satp());

  // if (p ){
  //    printf(" -------------kerneltrapcpu %d run pid :%d name:%s \n", cpuid(),p->pid,p->name);
  // }else {
  //    printf(" -------------kerneltrapcpu %d  pnull\n", cpuid() );
  // }

  // give up the CPU if this is a timer interrupt.
  if (which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
  {
    //    if (p ){
    //    printf(" -------------kerneltrap yield cpu %d run pid :%d name:%s \n", cpuid(),p->pid,p->name);
    // }else {
    //    printf(" -------------kerneltrap yield cpu %d  pnull\n", cpuid() );
    // }
    yield();
  }

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void clockintr()
{
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int devintr()
{
  uint64 scause = r_scause();

  if ((scause & 0x8000000000000000L) &&
      (scause & 0xff) == 9)
  {
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if (irq == UART0_IRQ)
    {
      // printf("uartintr:%d\n",scause);
      uartintr();
    }
    else if (irq == VIRTIO0_IRQ)
    {
      virtio_disk_intr();
    }
    else if (irq)
    {
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if (irq)
      plic_complete(irq);

    return 1;
  }
  else if (scause == 0x8000000000000001L)
  {
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if (cpuid() == 0)
    {
      clockintr();
    }

    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  }
  else
  {
    return 0;
  }
}
