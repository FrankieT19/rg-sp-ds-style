#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define fsync _commit
#define O_DIRECTORY 0
static int replace_file(const char*a,const char*b){return MoveFileExA(a,b,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)?0:-1;}
#else
#include <sys/wait.h>
#define replace_file rename
#endif
#include "stock_hardware.h"
#include "stock_settings.h"
static int node(const char*root,const char*name,int value){char p[2048];snprintf(p,sizeof p,"%s/sys/class/power_supply/axp2202-battery/%s",root,name);FILE*f=fopen(p,value<0?"r":"w");if(!f)return -1;int n=value;if(value<0){if(fscanf(f,"%d",&n)!=1)n=-1;}else if(fprintf(f,"%d",value)<0)n=-1;fclose(f);return n;}
int main(int argc,char**argv){
 if(argc!=3)return 2;const char*base=argv[1],*mode=argv[2],*root=getenv("DS_STYLE_TEST_ROOT");if(!root)root="";
 if(!*root&&(!stock_sleep_known()||!stock_settings_known())){fprintf(stderr,"stock settings: unknown firmware; unchanged\n");return 1;}
 char path[2048],temp[2080],backup[2048];snprintf(path,sizeof path,"%s/mnt/data/dmenu/dmenu_attr.ini",root);unsigned char before[140],after[140],check[140];
 if(!attr_read(path,before)){fprintf(stderr,"stock settings: unknown layout or checksum; unchanged\n");return 1;}
 if(!strcmp(mode,"restore")||!strcmp(mode,"sleep-policy")){
  if(*root||stock_sleep_policy_known()){unsigned flags=attr_u32(before+132);node(root,"workled_sleep",flags&1);node(root,"os_sleep",flags&16);fprintf(stderr,"stock sleep policy: workled_sleep=%u os_sleep=%u\n",flags&1,flags&16);}
  if(!strcmp(mode,"sleep-policy"))return 0;
 }
 if(!strcmp(mode,"restore")){
  int brightness=attr_u32(before+24),volume=attr_u32(before+36);node(root,"brightness",brightness);node(root,"openbor_volume",volume);

#ifndef _WIN32
  if(!*root){char exe[2048],level[16];snprintf(exe,sizeof exe,"%s/bin/dsstyle-brightness",base);snprintf(level,sizeof level,"%d",brightness);pid_t p=fork();if(!p){execl(exe,exe,"set",level,(char*)NULL);_exit(127);}if(p>0)waitpid(p,NULL,0);}

#endif
  fprintf(stderr,"stock settings: restored brightness=%d volume=%d\n",brightness,volume);return 0;
 }
 if(strcmp(mode,"save"))return 2;
 memcpy(after,before,140);int brightness=node(root,"brightness",-1),volume=node(root,"openbor_volume",-1);
 if(brightness>=0&&brightness<=6)attr_put(after+24,brightness);if(volume>=0&&volume<=10)attr_put(after+36,volume);attr_put(after+136,attr_crc(after,136));if(!memcmp(before,after,140))return 0;
 snprintf(backup,sizeof backup,"%s/state/dmenu-attr-before.bin",base);int fd=open(backup,O_WRONLY|O_CREAT|O_EXCL
#ifdef _WIN32
|O_BINARY
#endif
,0600);if(fd>=0){if(write(fd,before,140)!=140||fsync(fd)){close(fd);return 1;}close(fd);}else if(access(backup,F_OK))return 1;
 snprintf(temp,sizeof temp,"%s.dsstyle-%ld.tmp",path,(long)getpid());struct stat st;if(stat(path,&st))return 1;fd=open(temp,O_WRONLY|O_CREAT|O_EXCL
#ifdef _WIN32
|O_BINARY
#endif
,st.st_mode&0777);if(fd<0)return 1;
 int ok=write(fd,after,140)==140&&fsync(fd)==0;if(close(fd))ok=0;
 if(!ok||!attr_read(path,check)||memcmp(before,check,140)){unlink(temp);return 1;}
 if(replace_file(temp,path)){unlink(temp);return 1;}char*slash=strrchr(path,'/');if(slash){*slash=0;fd=open(path,O_RDONLY|O_DIRECTORY);if(fd>=0){fsync(fd);close(fd);}}
 fprintf(stderr,"stock settings: saved brightness=%d volume=%d with valid CRC\n",brightness,volume);return 0;
}
