//TODO: fix iar to handle .7 objects
#ifndef arm64
#pragma lib "libmini.a"
#endif

extern void xwrite(char*, int);
extern void xexit(void);

// To debug you can use
//  - gdb (when it works) 
//  - strace
//  - calling the C compiler (e.g., 5c) with -S to see the assembly code
//  - objdump -D (as the plan9 assembly code is actually not the final
//    assembly machine code)
//  - [5678vi]l -v -W -a (different debugging output)
void main() {
  xwrite("Hello C World\n", 14);
  // good to have another call with another string as
  // it usually exposes bugs such as forgetting to use setR12
  // or setSB with some linkers in a _start init function.
  xwrite("It works!\n", 10);
  // to help debug issues
  //for(;;) { }
  xexit();
}
