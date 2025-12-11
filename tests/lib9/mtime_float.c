#include <u.h>
#include <libc.h>

int main() {
    
  Dir *buf;
  ulong t;
  char* name = "mtime_float.c";

  buf = dirstat(name);
  free(buf);
  print("For %s mtime = %d, mtime_ = %f\n", name, buf->mtime, buf->mtime_);
}
