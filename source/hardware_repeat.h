#ifndef DSSTYLE_HARDWARE_REPEAT_H
#define DSSTYLE_HARDWARE_REPEAT_H
#include <stdint.h>
typedef struct { int key, brightness; uint64_t due; } HardwareRepeat;
static void hardware_repeat_clear(HardwareRepeat *r) { r->key=0; r->due=0; }
/* Kernel repeats are ignored; latch the chord until this key is released. */
static int hardware_repeat_event(HardwareRepeat *r,int key,int value,int brightness,uint64_t now) {
    if(value==0) { if(r->key==key) hardware_repeat_clear(r); return 0; }
    if(value!=1 || r->key==key) return 0;
    r->key=key; r->brightness=brightness; r->due=now+350; return 1;
}
static int hardware_repeat_due(HardwareRepeat *r,uint64_t now) {
    return r->key && now>=r->due;
}
static void hardware_repeat_reschedule(HardwareRepeat *r,uint64_t now) { r->due=now+120; }
#endif
