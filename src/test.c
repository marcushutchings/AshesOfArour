#include <stdint.h>
#include <stdio.h>

extern f32_imult(int32_t, int32_t);

int test(int32_t a1, int32_t a2){
  return f32_imult(a1, a2);
}

int main(){

  printf("%lx\n", test(0x10000, 0x10000));

  return 0;
}
