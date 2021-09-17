// Create a zombie process that
// must be reparented at exit.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void)
{
  helloos(0,0);
  exit(0);
}
