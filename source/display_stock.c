/* RG SP stock /dev/disp ARM32 ABI. Switch command 0x0f and mode 5
   match dmenu's allen_lcd_hdmi_switch at 0x13706. Geometry uses the
   sunxi disp2 layer ABI, without replacing buffers or audio policy. */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
struct Box {int x,y;unsigned w,h;};
struct Size {unsigned w,h;};
struct Crop {int64_t x,y,w,h;};
struct Image {uint64_t addr[3];struct Size size[3];unsigned align[3];int format,space;unsigned right[3];bool premult;struct Crop crop;int flags,scan;};
struct LayerInfo {int mode;unsigned char z,alpha_mode,alpha;struct Box window;bool stereo;int stereo_mode;union {unsigned colour;struct Image image;};unsigned id;};
struct Layer {struct LayerInfo info;bool enabled;unsigned channel,id;};
static int cable(void){char line[64];const char*paths[]={"/sys/class/switch/hdmi/state","/sys/class/extcon/hdmi/state"};for(int i=0;i<2;i++){FILE*f=fopen(paths[i],"r");if(f){int ok=fgets(line,sizeof line,f)!=NULL;fclose(f);if(ok){if(strstr(line,"HDMI=1")||line[0]=='1')return 1;if(strstr(line,"HDMI=0")||line[0]=='0')return 0;}}}return -1;}
int main(int argc,char**argv){if(argc!=2||(strcmp(argv[1],"enter")&&strcmp(argv[1],"leave")))return 2;int connected=cable();if(connected<0)return 0;int fd=open("/dev/disp",O_RDWR);if(fd<0)return 1;uint32_t args[4]={0};int before=ioctl(fd,0x09,args),target=connected?4:1;
 if(before!=target){if(before!=1&&before!=4){close(fd);return 1;}int brightness=ioctl(fd,0x103,args);args[1]=target;args[2]=5;if(ioctl(fd,0x0f,args)<0){close(fd);return 1;}for(int i=0;i<30;i++){usleep(50000);uint32_t q[4]={0};if(ioctl(fd,0x09,q)==target)break;}usleep(150000);if(!connected&&brightness>=0&&brightness<=255){uint32_t q[4]={0,(uint32_t)brightness,0,0};ioctl(fd,0x102,q);}}
 int fb=open("/dev/fb0",O_RDONLY);struct fb_var_screeninfo v={0};if(fb<0||ioctl(fb,FBIOGET_VSCREENINFO,&v)<0){if(fb>=0)close(fb);close(fd);return 1;}close(fb);
 uint32_t q[4]={0};int w=ioctl(fd,0x07,q),h=ioctl(fd,0x08,q);if(w<=0||h<=0||w>4096||h>4096){close(fd);return 1;}
 struct Layer layer={0};layer.channel=1;layer.id=0;uint32_t p[4]={0,(uint32_t)(uintptr_t)&layer,1,0};if(ioctl(fd,0x48,p)<0||!layer.enabled||layer.info.mode!=0||!layer.info.image.addr[0]){close(fd);return 1;}
 /* Only adjust the output window; current framebuffer addresses and crop survive. */
 struct Box box={0,0,(unsigned)w,(unsigned)h};if(!strcmp(argv[1],"enter")&&connected){if((int64_t)w*2>(int64_t)h*3){box.w=h*3/2;box.x=(w-box.w)/2;}else{box.h=w*2/3;box.y=(h-box.h)/2;}}
 layer.info.window=box;int rc=ioctl(fd,0x47,p);fprintf(stderr,"Display: output=%d window=%d,%d %ux%u rc=%d\n",target,box.x,box.y,box.w,box.h,rc);close(fd);return rc<0?1:0;
}
