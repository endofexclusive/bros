/* GCC stack smashing protector */

#include <stdint.h>
 
const uintptr_t __stack_chk_guard = (uintptr_t) 0xfedcba9876543211;
 
void __stack_chk_fail(void);
void __stack_chk_fail(void) {
  while(1);
}

