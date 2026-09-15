/* Read Game Rooms routes from the installed stock dmenu table. No shell eval.
 * Layout and argument forms verified against the user's captured RG SP binary.
 * Unknown layouts fail closed; the stock RetroArch route remains the default. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>
#include "stock_hardware.h"
static unsigned char bin[524288];static size_t size;
static unsigned word(size_t off){return bin[off]|bin[off+1]<<8|bin[off+2]<<16|(unsigned)bin[off+3]<<24;}
static const char*str(unsigned address){if(address<0x10000||address-0x10000>=size)return "";const char*p=(char*)bin+address-0x10000;size_t n=size-(address-0x10000);if(!memchr(p,0,n))return "";for(const char*q=p;*q;q++)if((unsigned char)*q<32||(unsigned char)*q>126)return "";return p;}
static int rooted(char*out,size_t n,const char*root,const char*path){return snprintf(out,n,"%s%s",root,path)<(int)n;}
int main(int argc,char**argv){
 if(argc!=4)return 2;const char*mode=argv[1],*system=argv[2],*rom=argv[3],*root=getenv("DS_STYLE_TEST_ROOT");if(!root)root="";
 if(strcmp(mode,"plan")&&strcmp(mode,"game"))return 2;
 char path[4096];rooted(path,sizeof path,root,"/mnt/vendor/bin/dmenu.bin");FILE*f=fopen(path,"rb");if(!f)return 20;size=fread(bin,1,sizeof bin,f);int extra=fgetc(f);fclose(f);
 if(extra!=EOF||size<0x35850||memcmp(bin+17160,stock_sleep_signature,sizeof stock_sleep_signature)||strcmp(str(word(0x35464)),"PSP")||strcmp(str(word(0x354bc)),"NDS")){fprintf(stderr,"Unknown stock Game Rooms table; unchanged\n");return 20;}
 const char*exe=NULL,*work=NULL,*alternate="";unsigned sysid=0;
 for(size_t off=0x3545c;off+44<=size;off+=44){const char*s=str(word(off+8)),*e=str(word(off+16));if(!*s||strncmp(e,"./",2)&&strncmp(e,"/mnt/",5))break;if(!strcasecmp(s,system)){exe=e;alternate=str(word(off+20));work=str(word(off+24));sysid=word(off+28);break;}}
 if(!exe){fprintf(stderr,"Stock has no Game Rooms route for %s. Select RetroArch.\n",system);return 20;}
 /* Stock .mycore is directory:filename:card:RA-flag:core-choice, optionally
  * followed by its binary CRC. It matches basename, card and RA flag (0 here).
  * Directory and table's system ID are not consulted by stock's lookup. */
 if(*alternate){rooted(path,sizeof path,root,"/mnt/data/misc/.mycore");f=fopen(path,"rb");if(f){char line[2048],sd1[4096],sd2[4096];rooted(sd1,sizeof sd1,root,"/mnt/mmc/");rooted(sd2,sizeof sd2,root,"/mnt/sdcard/");int card=!strncmp(rom,sd1,strlen(sd1))?0:!strncmp(rom,sd2,strlen(sd2))?1:-1;const char*name=strrchr(rom,'/');name=name?name+1:rom;
   while(fgets(line,sizeof line,f)){char*fields[5];fields[0]=line;int i;for(i=1;i<5;i++){char*q=strchr(fields[i-1],':');if(!q)break;*q=0;fields[i]=q+1;}if(i==5&&!strcmp(fields[1],name)&&atoi(fields[2])==card&&atoi(fields[3])==0){if(atoi(fields[4])==1)exe=alternate;break;}}fclose(f);}}
 char cwd[4096],executable[4096];if(*work=='/')rooted(cwd,sizeof cwd,root,work);else snprintf(cwd,sizeof cwd,"%s/mnt/vendor/bin/game/%s",root,work);
 if(*exe=='/')rooted(executable,sizeof executable,root,exe);else snprintf(executable,sizeof executable,"%s/%s",cwd,exe);
 int ps=strstr(exe,"pcsx")!=NULL,m68k=0;const char*name=strrchr(rom,'/');name=name?name+1:rom;
 if(strstr(exe,"fbasdl")){snprintf(path,sizeof path,"%s",rom);char*q=strrchr(path,'/');if(q)strcpy(q+1,"m68k.txt");m68k=!strcmp(name,"goldnaxe.zip")||!access(path,F_OK);}
 printf("Route: stock Game Rooms\nSystem: %s (%u)\nExecutable: %s\nWorking directory: %s\nROM: %s\nArguments: %sROM%s\n",system,sysid,executable,cwd,rom,ps?"-cdfile ":"",m68k?" m68k":"");fflush(stdout);
 if(!strcmp(mode,"plan"))return 0;
 if(access(executable,X_OK)||chdir(cwd)){perror("Stock Game Rooms launcher unavailable");return 20;}
#ifndef _WIN32
 /* Stock enables HDMI audio for its standalone SDL frontends. */
 if(strstr(exe,"PPSSPP")||strstr(exe,"drastic")||strstr(exe,"OpenBOR")){rooted(path,sizeof path,root,"/sys/class/switch/hdmi/state");f=fopen(path,"r");int hdmi=0;if(f){fscanf(f,"%d",&hdmi);fclose(f);}if(hdmi)setenv("AUDIODEV","hw:2,0",1);}
 if(ps)execl(executable,exe,"-cdfile",rom,(char*)NULL);else if(m68k)execl(executable,exe,rom,"m68k",(char*)NULL);else execl(executable,exe,rom,(char*)NULL);
 perror("Stock Game Rooms exec");return 20;
#else
 return 20; /* Host verification inspects plans; it never executes firmware. */
#endif
}
