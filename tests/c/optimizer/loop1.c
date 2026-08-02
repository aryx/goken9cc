
void test() {
  for (int i = 0; i < 1000000; i++)
     ;
  // from Richard Miller talk https://www.youtube.com/watch?v=LHJqdXGb0uc
  // where gcc requires in the loop 'asm volatile("" ::: "memory")'
  // to prevent the optimizer to remove the entire loop
  // kencc does no need that because simpler optimizer.
}
