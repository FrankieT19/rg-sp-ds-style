/* Shared arithmetic used by the Linux framebuffer adapter and host self-test. */
#ifndef DSSTYLE_FB_LAYOUT_H
#define DSSTYLE_FB_LAYOUT_H
#include <stdint.h>
#include <string.h>
static int fb_window_fits(uint32_t w,uint32_t h,uint32_t vw,uint32_t vh,
                          uint32_t x,uint32_t y,uint32_t bpp,uint32_t stride,uint64_t bytes) {
    if (!w || !h || (bpp!=16 && bpp!=32) || !stride) return 0;
    return (uint64_t)x+w<=vw && (uint64_t)y+h<=vh &&
           ((uint64_t)x+w)*(bpp/8)<=stride &&
           ((uint64_t)y+h)*stride<=bytes;
}
static uint32_t fb_next_y(uint32_t h,uint32_t vh,uint32_t current,uint32_t step) {
    if (step && h%step==0 && (uint64_t)h*2<=vh && (current==0 || current==h))
        return current==0?h:0;
    return current;
}
/* Seed every complete page before handing the display to stock. Never repaint
   once the emulator owns it. Row is a separate rendered scanline buffer. */
static void fb_launch_row(unsigned char *mem,uint64_t size,const void *row,
                         uint32_t w,uint32_t h,uint32_t vw,uint32_t vh,
                         uint32_t x,uint32_t y,uint32_t bpp,uint32_t stride) {
    if(!h || y>=h) return;
    for(uint64_t page=0;page+h<=vh;page+=h) {
        if(!fb_window_fits(w,h,vw,vh,x,(uint32_t)page,bpp,stride,size)) break;
        memcpy(mem+(page+y)*stride+(uint64_t)x*(bpp/8),row,(size_t)w*(bpp/8));
    }
}
static int fb_layout_selftest(void) {
    return fb_window_fits(720,480,720,960,0,480,32,2880,2764800) &&
        !fb_window_fits(720,480,720,960,0,481,32,2880,2764800) &&
        !fb_window_fits(720,480,720,960,1,0,32,2880,2764800) &&
        !fb_window_fits(720,480,720,960,0,480,32,2880,1382400) &&
        !fb_window_fits(720,480,720,960,0,0,32,2879,2764800) &&
        !fb_window_fits(720,480,720,960,0,UINT32_MAX,32,2880,UINT64_MAX) &&
        fb_window_fits(720,480,720,480,0,0,16,1440,691200) &&
        fb_next_y(480,960,0,1)==480 && fb_next_y(480,960,480,1)==0 &&
        fb_next_y(480,480,0,1)==0 && fb_next_y(480,960,0,0)==0 &&
        fb_next_y(480,960,0,7)==0;
}
#endif
