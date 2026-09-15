/* UI sound queue using the device's own tinyplay, the playback path that was
 * audible in the first device trial. Never kill a sound for a newer UI event.
 * Only temporary, volume-scaled copies are made; originals stay unchanged. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef AUDIO_OFFLINE
#include <sys/wait.h>
#include <poll.h>
#include <signal.h>
static volatile sig_atomic_t stopping;
static void stop(int sig){(void)sig;stopping=1;}
#endif
static const char*names[]={"accept","back","launch","menu","move","startup","tab"};
static unsigned u32(const unsigned char*p){return p[0]|p[1]<<8|p[2]<<16|(unsigned)p[3]<<24;}
static void put32(unsigned char*p,unsigned v){for(int i=0;i<4;i++)p[i]=v>>(8*i);}
static int volume(void){int n=10;
#ifndef AUDIO_OFFLINE
 FILE*f=fopen("/sys/class/power_supply/axp2202-battery/openbor_volume","r");if(f){if(fscanf(f,"%d",&n)!=1)n=10;fclose(f);}
#else
 if(getenv("AUDIO_LEVEL"))n=atoi(getenv("AUDIO_LEVEL"));
#endif
 return n<0?0:n>10?10:n;
}
static int prepare(const char*base,int id,const char*out){
 char path[2048];snprintf(path,sizeof path,"%s/assets/sounds/%s.wav",base,names[id]);FILE*f=fopen(path,"rb");if(!f)return 0;
 unsigned char head[44];if(fread(head,1,44,f)!=44||memcmp(head,"RIFF",4)||memcmp(head+8,"WAVEfmt ",8)||u32(head+16)!=16||head[20]!=1||head[22]!=2||u32(head+24)!=48000||head[34]!=16||memcmp(head+36,"data",4)){fclose(f);return 0;}
 unsigned bytes=u32(head+40);if(!bytes||bytes>4000000||bytes%4){fclose(f);return 0;}int16_t*pcm=malloc(bytes);if(!pcm){fclose(f);return 0;}if(fread(pcm,1,bytes,f)!=bytes){free(pcm);fclose(f);return 0;}fclose(f);
 int level=volume();unsigned frames=bytes/4;for(unsigned i=0;i<frames;i++){unsigned edge=i<frames-1-i?i:frames-1-i;if(edge>96)edge=96;for(int c=0;c<2;c++)pcm[i*2+c]=(int16_t)((int64_t)pcm[i*2+c]*edge*level/960);}
 f=fopen(out,"wb");int ok=0;if(f){put32(head+4,bytes+36);ok=fwrite(head,1,44,f)==44&&fwrite(pcm,1,bytes,f)==bytes;if(fclose(f))ok=0;}free(pcm);
 fprintf(stderr,"audio stock: %s, %u frames, volume=%d\n",names[id],frames,level);return ok;
}
int main(int argc,char**argv){if(argc!=2)return 2;
#ifdef AUDIO_OFFLINE
 const char*events=getenv("AUDIO_EVENTS"),*prefix=getenv("AUDIO_CAPTURE");if(!events||!prefix)return 2;
 for(int i=0;events[i];i++){int id=events[i]-'0';char out[2048];snprintf(out,sizeof out,"%s-%d.wav",prefix,i);if(id<0||id>6||!prepare(argv[1],id,out))return 1;}return 0;
#else
 const char*player="/mnt/vendor/bin/tinyplay";if(access(player,X_OK)){perror("stock tinyplay unavailable");return 1;}
 signal(SIGTERM,stop);signal(SIGINT,stop);
 int queue[4],count=0,closing=0,status=0;pid_t child=-1;char out[]="/tmp/dsstyle-audio-XXXXXX";int temp=mkstemp(out);if(temp<0)return 1;close(temp);
 fcntl(0,F_SETFL,fcntl(0,F_GETFL)|O_NONBLOCK);
 for(;;){
  if(stopping){if(child>0){kill(child,SIGTERM);waitpid(child,NULL,0);}break;}
  unsigned char commands[64];ssize_t n=closing?-1:read(0,commands,sizeof commands);if(n==0)closing=1;if(n<0&&errno!=EAGAIN&&errno!=EINTR&&!closing)closing=1;
  for(ssize_t i=0;i<n;i++)if(commands[i]<7){int id=commands[i];/* Coalesce repeated move ticks while a longer sound finishes. */if(id==4&&count&&queue[count-1]==4)continue;if(id==2)count=0; /* Launch supersedes pending navigation, never the active sound. */if(count<4)queue[count++]=id;}
  if(child>0&&waitpid(child,&status,WNOHANG)==child){if(!WIFEXITED(status)||WEXITSTATUS(status))fprintf(stderr,"stock tinyplay exit status=%d\n",status);child=-1;}
  if(child<0&&count){int id=queue[0];memmove(queue,queue+1,(--count)*sizeof *queue);if(prepare(argv[1],id,out)){child=fork();if(child==0){execl(player,"tinyplay",out,(char*)NULL);_exit(127);}}}
  if(closing&&child<0&&!count)break;
  struct pollfd fd={0,POLLIN,0};if(closing)poll(NULL,0,5);else poll(&fd,1,10);
 }
 unlink(out);return 0;
#endif
}
