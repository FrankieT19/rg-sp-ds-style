/* RG SP stock-menu bridge. All core names, defaults, choice arrays and launch
 * paths come from the installed dmenu ELF, never DS Style profiles or CORES.txt.
 * The documented Thumb/table ABI is checked; unknown firmware fails closed.
 * .setcore is stock RetroArch selection; .mycore belongs to Game Rooms. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
static unsigned char bin[1048576];static size_t size;static const char*root;
static unsigned half(size_t p){return p+2<=size?(unsigned)bin[p]|(unsigned)bin[p+1]<<8:0;}
static unsigned word(size_t p){return p+4<=size?half(p)|half(p+2)<<16:0;}
static size_t address(unsigned a){unsigned ph=word(28),n=half(44),step=half(42);for(unsigned i=0;i<n;i++){size_t p=ph+i*step;unsigned at=word(p+8),len=word(p+16);if(word(p)==1&&a>=at&&a-at<len&&word(p+4)+(a-at)<size)return word(p+4)+a-at;}return size;}
static const char*string(unsigned a){size_t p=address(a);if(p>=size)return "";const char*s=(const char*)bin+p;if(!memchr(s,0,size-p))return "";for(const char*q=s;*q;q++)if((unsigned char)*q<32||(unsigned char)*q>126)return "";return s;}
static const char*literal(size_t p){return string(word(p));}
static int path(char*out,size_t n,const char*p){return *p=='/'&&snprintf(out,n,"%s%s",root,p)<(int)n;}
/* Decode just the installed pure core-choice lookup's table operands. No
 * firmware instructions are executed and no system-to-core map is compiled in. */
static const char*choice_core(unsigned id,unsigned choice){
 for(size_t p=0x21cca;p<0x21eb6;p+=2){if(half(p)!=(0x2b00|id))continue;
  size_t q=p+2;while(q<0x21eb6&&(half(q)&0xff00)!=0x2900)q+=2;
  if(q>=0x21eb6||choice>(half(q)&255))return "";
  unsigned base=0,offset=0;int got=0;
  for(q+=2;q<0x21eb6&&half(q)!=0x4770;q+=2){unsigned h=half(q),n=half(q+2);
   if((h&0xf800)==0x4800)base=word(((q+0x10000+4)&~3u)-0x10000+(h&255)*4);
   if((h&0xfff0)==0xf8d0&&(n>>12)==0){offset=n&4095;got=1;break;}
   if((h&0xf800)==0x6800&&(h&7)==0){offset=((h>>6)&31)*4;got=1;break;}
   if(h==0xf850&&n==0x0021){offset=0;got=1;break;}
  }
  if(!got||!base)return "";size_t at=address(base+offset+choice*4);return at<size?string(word(at)):"";
 }
 return "";
}
static int selection(const char*file,const char*rom,int card){
 FILE*f=fopen(file,"rb");if(!f)return 0;char line[2048];int result=0,first=1;const char*name=strrchr(rom,'/');name=name?name+1:rom;
 while(fgets(line,sizeof line,f)){
  if(first){first=0;if(strncmp(line,"Version=1",9))break;}
  char*fields[5];fields[0]=line;int i;for(i=1;i<5;i++){char*q=strchr(fields[i-1],':');if(!q)break;*q=0;fields[i]=q+1;}
  if(i==5&&!strcmp(fields[1],name)&&atoi(fields[2])==card&&atoi(fields[3])==1){result=atoi(fields[4]);break;}
 }
 fclose(f);return result>0?result:0;
}
static int invoke(const char*cwd,const char*exe,const char*a,const char*b,const char*c,int plan){
 char wd[4096],command[4096];if(!path(wd,sizeof wd,cwd)||!path(command,sizeof command,exe))return 20;
 printf("Working directory: %s\nExecutable: %s\nArgument 1: %s\nArgument 2: %s\nArgument 3: %s\n",wd,command,a?a:"",b?b:"",c?c:"");fflush(stdout);if(plan)return 0;
 if(chdir(wd)){perror("Stock working directory");return 20;}
#ifndef _WIN32
 if(c)execl(command,exe,a,b,c,(char*)NULL);else if(b)execl(command,exe,a,b,(char*)NULL);else if(a)execl(command,exe,a,(char*)NULL);else execl(command,exe,(char*)NULL);
 perror("Stock launcher");
#endif
 return 20;
}
static int native_command(const char*cwd,const char*command,const char*rom,int plan){
 char exe[4096],full[4096];snprintf(exe,sizeof exe,"%s",command);char*option=strchr(exe,' ');if(option)*option++=0;
 if(*exe!='/'){snprintf(full,sizeof full,"%s/%s",cwd,exe);}else snprintf(full,sizeof full,"%s",exe);
 return invoke(cwd,full,option?option:rom,option?rom:NULL,NULL,plan);
}
/* When the mod wrapper is absent, use the command template embedded in stock,
 * not DS Style's former configuration/flags. Split its fixed template into
 * argv directly; substituted ROM paths are never interpreted as shell code. */
static int stock_direct(const char*core,const char*rom,int plan){
 char debug[4096],cwd[4096],exe[4096],cp[4096],text[4096];path(debug,sizeof debug,literal(0x132c4));
 snprintf(text,sizeof text,"%s",literal(access(debug,F_OK)?0x132d0:0x132cc));
 path(cwd,sizeof cwd,literal(0x132bc));snprintf(cp,sizeof cp,"%s%s/%s",root,literal(0x132c8),core);
 char*args[20],*save=NULL;int n=0;for(char*t=strtok_r(text," ",&save);t&&n<17;t=strtok_r(NULL," ",&save)){
  if(!strcmp(t,"%s/%s"))args[n++]=cp;else if(strchr(t,'%'))return 20;else args[n++]=t;
 }
 if(!n)return 20;snprintf(exe,sizeof exe,"%s/%s",cwd,args[0]);args[n++]=(char*)rom;args[n]=NULL;
 printf("Route: stock embedded command template\nWorking directory: %s\nExecutable: %s\n",cwd,exe);for(int i=1;i<n;i++)printf("Argument %d: %s\n",i,args[i]);fflush(stdout);if(plan)return 0;
#ifndef _WIN32
 if(chdir(cwd))return 20;execv(exe,args);perror("Stock RetroArch");
#endif
 return 20;
}
int main(int argc,char**argv){
 if(argc!=4||(strcmp(argv[1],"plan")&&strcmp(argv[1],"game")))return 2;
 int plan=!strcmp(argv[1],"plan");const char*sys=argv[2],*rom=argv[3];root=getenv("DS_STYLE_TEST_ROOT");if(!root)root="";
 char file[4096],sd1[4096],sd2[4096];path(file,sizeof file,"/mnt/vendor/bin/dmenu.bin");FILE*f=fopen(file,"rb");if(!f){fprintf(stderr,"Installed stock menu not found\n");return 20;}size=fread(bin,1,sizeof bin,f);int extra=fgetc(f);fclose(f);
 const unsigned char signature[]={0x03,0x68,0x0e,0x2b,0x08,0xd1,0x05,0x29};
 if(extra!=EOF||size<0x36d58||memcmp(bin,"\177ELF\1\1",6)||memcmp(bin+0x21cc8,signature,8)||strcmp(literal(0x13258),"/mnt/mod/ctrl/RA_launch.sh")){fprintf(stderr,"Unsupported stock menu ABI; use Stock OS. No guessed core or profile fallback.\n");return 20;}
 path(sd1,sizeof sd1,"/mnt/mmc/Roms/");path(sd2,sizeof sd2,"/mnt/sdcard/Roms/");int card=!strncmp(rom,sd1,strlen(sd1))?0:!strncmp(rom,sd2,strlen(sd2))?1:-1;if(card<0)return 20;
 size_t entry=size;for(size_t p=0x36260;p+52<=size&&word(p)<256;p+=52)if(!strcasecmp(string(word(p+12)),sys)||!strcasecmp(string(word(p+4)),sys)){entry=p;break;}
 if(entry==size){fprintf(stderr,"System absent from installed stock table: %s\n",sys);return 20;}
 unsigned id=word(entry);const char*core=string(word(entry+24));int chosen=0;
 if(word(entry+44)){if(!path(file,sizeof file,literal(0x109b8)))return 20;chosen=selection(file,rom,card);if(chosen){core=choice_core(id,(unsigned)chosen);if(!*core){fprintf(stderr,"Invalid stock core choice; use Stock OS to reset it\n");return 20;}}}
 printf("Route: installed stock RetroArch menu\nSystem: %s\nStock default: %s\nStock per-game choice: %d\nCore: %s\nROM: %s\n",sys,string(word(entry+24)),chosen,core,rom);
 /* Hand the chosen stock values to the existing stock wrapper. It owns RA
  * arguments, config, overrides, save paths and mod-specific per-ROM rules. */
 const char*wrapper=literal(0x13258);path(file,sizeof file,wrapper);
 if((*core&&strstr(core,"_libretro.so"))||id==53){
  if(access(file,F_OK)){if(id==53)return 20;return stock_direct(core,rom,plan);}
  return invoke(literal(0x132bc),wrapper,id==53?"ports":core,rom,NULL,plan);
 }
 #ifndef _WIN32
 if(!plan&&(id==0||id==2||id==4)){char node[4096];path(node,sizeof node,"/sys/class/switch/hdmi/state");FILE*hf=fopen(node,"r");int hdmi=0;if(hf){fscanf(hf,"%d",&hdmi);fclose(hf);}if(hdmi)setenv("AUDIODEV","hw:2,0",1);}
#endif
 /* Native branches are stock's verified dispatcher ABI, with paths read from
  * its own literals. Game Rooms still uses the separate stock menu table. */
 if(id==0)return native_command(literal(0x1324c),literal(0x13254),rom,plan);
 if(id==29)return native_command(literal(0x1328c),literal(0x13290),rom,plan);
 if((id==5||id==34||id==12)&&strstr(core,literal(0x1329c)))return native_command(literal(0x132a0),literal(0x132a4),rom,plan);
 if(id==44)return native_command(literal(0x132ac),literal(strstr(core,literal(0x132a8))?0x132b0:0x132b8),rom,plan);
 if(id==2){
  const char*raw=literal(0x1326c);char script[4096];snprintf(script,sizeof script,"%s",raw);char*space=strchr(script,' ');if(!space)return 20;*space=0;
  if(!plan){char target[4096];path(target,sizeof target,script);
#ifndef _WIN32
   /* Stock savedir must complete before its run operation. */
   extern int stock_prepare(const char*,const char*);if(stock_prepare(target,rom))return 20;
#endif
  }
  return invoke(literal(0x13268),script,"run",rom,NULL,plan);
 }
 if(id==4){
  if(!plan){char script[4096];path(script,sizeof script,literal(0x13284));
#ifndef _WIN32
   extern int stock_prepare_arg(const char*,const char*,const char*);if(stock_prepare_arg(script,rom,NULL))return 20;
#endif
  }return native_command(literal(0x1327c),literal(0x13280),rom,plan);
 }
 fprintf(stderr,"Stock native route needs a verified dispatcher bridge; use Stock OS.\n");return 20;
}
#ifndef _WIN32
#include <sys/wait.h>
int stock_prepare_arg(const char*exe,const char*a,const char*b){pid_t p=fork();if(!p){if(b)execl(exe,exe,a,b,(char*)NULL);else execl(exe,exe,a,(char*)NULL);_exit(127);}if(p<0)return 20;int s;while(waitpid(p,&s,0)<0)if(errno!=EINTR)return 20;return WIFEXITED(s)?WEXITSTATUS(s):20;}
#endif

#ifndef _WIN32
int stock_prepare(const char*exe,const char*rom){return stock_prepare_arg(exe,"savedir",rom);}
#endif
