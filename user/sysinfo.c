// Create a zombie process that
// must be reparented at exit.

#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int main(void)
{

  struct sysinfo st;
  gsysinfo(&st);
  exit(0);
}
