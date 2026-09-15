#include <assert.h>
#include <stdio.h>
#include "../source/hardware_repeat.h"
#include "../source/fb_layout.h"
int main(void) {
 HardwareRepeat r={0};
 assert(hardware_repeat_event(&r,115,1,1,100));
 assert(!hardware_repeat_due(&r,449)); assert(hardware_repeat_due(&r,450));
 assert(!hardware_repeat_event(&r,115,2,0,450)); assert(r.brightness==1);
 hardware_repeat_reschedule(&r,500); assert(!hardware_repeat_due(&r,619)); assert(hardware_repeat_due(&r,620));
 hardware_repeat_event(&r,114,0,0,620); assert(r.key==115);
 hardware_repeat_event(&r,115,0,0,620); assert(!hardware_repeat_due(&r,9999));
 assert(hardware_repeat_event(&r,114,1,0,1000)); assert(!r.brightness);
 hardware_repeat_clear(&r); assert(!hardware_repeat_due(&r,9999));
 assert(fb_layout_selftest());
 for(unsigned bits=16;bits<=32;bits+=16) {
  unsigned char mem[256],row[12]; memset(mem,0xa5,sizeof mem);memset(row,0x42,sizeof row);
  unsigned stride=20, h=3, x=1, w=3;
  for(unsigned y=0;y<h;y++)fb_launch_row(mem,120,row,w,h,5,9,x,y,bits,stride);
  /* Two backed pages, third virtual page outside mapped memory is untouched. */
  for(unsigned y=0;y<12;y++)for(unsigned z=0;z<stride;z++) {
   int written=y<6 && z>=x*(bits/8) && z<(x+w)*(bits/8);
   assert(mem[y*stride+z]==(written?0x42:0xa5));
  }
 }
 puts("repeat timing, release, chord latch, reset, 16/32-bit page replication and bounds passed");
 return 0;
}
