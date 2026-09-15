/* Match stock dmenu's ARM32 /dev/disp ioctl ABI, including on a 64-bit kernel.
 * Recovered from this RG SP's dmenu: get=0x103, set=0x102, screen=0.
 * Never set a guessed value when reading fails. No boot/default changes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
int main(int argc,char **argv) {
    const int levels[]={5,10,20,50,70,140,170};
    if((argc!=2&&argc!=3)||(strcmp(argv[1],"up")&&strcmp(argv[1],"down")&&strcmp(argv[1],"get")&&strcmp(argv[1],"set")))return 2;
    int fd=open("/dev/disp",O_RDWR);if(fd<0){perror("brightness open");return 1;}
    uint32_t args[4]={0};int current=ioctl(fd,0x103,args);
    if(current<0||current>255){perror("brightness read");close(fd);return 1;}
    int next=current;if(!strcmp(argv[1],"set")){if(argc!=3||atoi(argv[2])<0||atoi(argv[2])>6){close(fd);return 2;}next=levels[atoi(argv[2])];}
    if(!strcmp(argv[1],"up")){for(int i=0;i<7;i++)if(levels[i]>current){next=levels[i];break;}}
    if(!strcmp(argv[1],"down")){for(int i=6;i>=0;i--)if(levels[i]<current){next=levels[i];break;}}
    if(next!=current){args[1]=(uint32_t)next;if(ioctl(fd,0x102,args)<0){perror("brightness write");close(fd);return 1;}}
    args[1]=0;int actual=ioctl(fd,0x103,args);if(actual<0||actual>255){close(fd);return 1;}next=actual;
    int index=0;while(index<6&&levels[index]<next)index++;
    if(next!=current){FILE*f=fopen("/sys/class/power_supply/axp2202-battery/brightness","w");if(f){fprintf(f,"%d",index);fclose(f);}}
    printf("%d\n",index);fprintf(stderr,"Brightness: %d -> %d\n",current,next);close(fd);return 0;
}
