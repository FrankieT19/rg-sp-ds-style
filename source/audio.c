/* DS Style UI-only PCM worker. Holds hw:0,0 until stdin closes, then finishes
 * queued sounds and releases it before launching any stock emulator. */
#include <tinyalsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#ifndef AUDIO_OFFLINE
#include <sys/ioctl.h>
#include <sound/asound.h>
#include "stock_hardware.h"
/* The stock binary's tinymixer_set_value calls use zero-based control indices,
 * not ALSA numids: 1=63, 2=31, 3=1, 5=1, 8=1. Match the actual stock code
 * before reproducing that initialization. Query every type/range first. */
static void stock_mixer(void){
 int fd=open("/dev/snd/controlC0",O_RDWR);if(fd<0)return;
 struct snd_ctl_elem_list list={0};if(ioctl(fd,SNDRV_CTL_IOCTL_ELEM_LIST,&list)<0||list.count>1024){close(fd);return;}
 struct snd_ctl_elem_id*ids=calloc(list.count,sizeof *ids);if(!ids){close(fd);return;}list.space=list.count;list.pids=ids;
 int known=stock_audio_known();
 if(ioctl(fd,SNDRV_CTL_IOCTL_ELEM_LIST,&list)==0)for(unsigned i=0;i<list.used;i++){
  struct snd_ctl_elem_info info={0};struct snd_ctl_elem_value v={0};info.id=v.id=ids[i];
  if(ioctl(fd,SNDRV_CTL_IOCTL_ELEM_INFO,&info)<0||ioctl(fd,SNDRV_CTL_IOCTL_ELEM_READ,&v)<0)continue;
  if(info.type!=SNDRV_CTL_ELEM_TYPE_INTEGER&&info.type!=SNDRV_CTL_ELEM_TYPE_BOOLEAN)continue;
  fprintf(stderr,"audio mixer[%u] %s type=%d range=%ld..%ld value=%ld\n",i,ids[i].name,info.type,info.value.integer.min,info.value.integer.max,v.value.integer.value[0]);
  int target=i==1?63:i==2?31:(i==3||i==5||i==8)?1:-1;
  if(!known||target<0||info.count>128||target<info.value.integer.min||target>info.value.integer.max)continue;
  for(unsigned j=0;j<info.count;j++)v.value.integer.value[j]=target;
  if(ioctl(fd,SNDRV_CTL_IOCTL_ELEM_WRITE,&v)<0)perror("audio stock mixer");
 }
 free(ids);close(fd);fprintf(stderr,"audio: stock mixer signature %s\n",known?"matched":"unknown; left unchanged");
}
static int stock_gain(void){int level=10;FILE*f=fopen("/sys/class/power_supply/axp2202-battery/openbor_volume","r");if(f){if(fscanf(f,"%d",&level)!=1)level=10;fclose(f);}return level<0?0:level>10?10:level;}
#endif
#ifdef AUDIO_OFFLINE
#include "../tests/audio_mock.h"
#endif
enum { BLOCK=256, VOICES=16 };
typedef struct { int16_t *data; size_t frames; } Sample;
typedef struct { int sample; size_t pos; } Voice;
static const char *names[]={"accept","back","launch","menu","move","startup","tab"};
static unsigned le32(const unsigned char *p){return p[0]|p[1]<<8|p[2]<<16|(unsigned)p[3]<<24;}
static Sample load(const char *base,const char *name){
 Sample s={0};char path[2048];snprintf(path,sizeof path,"%s/assets/sounds/%s.wav",base,name);
 FILE*f=fopen(path,"rb");if(!f)return s;unsigned char h[12];
 if(fread(h,1,12,f)!=12||memcmp(h,"RIFF",4)||memcmp(h+8,"WAVE",4)){fclose(f);return s;}
 unsigned char chunk[8];int valid=0;
 while(fread(chunk,1,8,f)==8){unsigned n=le32(chunk+4);if(n>4000000)break;
  if(!memcmp(chunk,"fmt ",4)){unsigned char fmt[16];if(n<16||fread(fmt,1,16,f)!=16)break;valid=fmt[0]==1&&fmt[1]==0&&fmt[2]==2&&fmt[3]==0&&le32(fmt+4)==48000&&fmt[14]==16;fseek(f,n-16+(n&1),SEEK_CUR);}
  else if(!memcmp(chunk,"data",4)&&valid){s.data=malloc(n);if(s.data&&fread(s.data,1,n,f)==n)s.frames=n/4;else{free(s.data);s.data=NULL;}break;}
  else fseek(f,n+(n&1),SEEK_CUR);
 }fclose(f);return s;
}
int main(int argc,char **argv){
 if(argc!=2)return 2;Sample samples[7];Voice voices[VOICES];
 for(int i=0;i<7;i++){samples[i]=load(argv[1],names[i]);fprintf(stderr,"audio sample %s: %zu frames\n",names[i],samples[i].frames);}
 for(int i=0;i<VOICES;i++)voices[i].sample=-1;
 struct pcm_config cfg={.channels=2,.rate=48000,.period_size=BLOCK,.period_count=4,.format=PCM_FORMAT_S16_LE};
 struct pcm *pcm=pcm_open(0,0,PCM_OUT,&cfg);
 if(!pcm_is_ready(pcm)){fprintf(stderr,"audio open: %s\n",pcm_get_error(pcm));pcm_close(pcm);return 1;}
#ifndef AUDIO_OFFLINE
 stock_mixer();
#endif
 fprintf(stderr,"audio: persistent PCM 48000 stereo, period %d\n",BLOCK);
#ifndef AUDIO_OFFLINE
 fcntl(STDIN_FILENO,F_SETFL,fcntl(STDIN_FILENO,F_GETFL)|O_NONBLOCK);
#endif
 int closing=0,tail=0,errors=0,gain_level=10,gain_tick=0,gain_current=655360;
#ifndef AUDIO_OFFLINE
 gain_level=stock_gain();gain_current=gain_level*65536;
#endif
 for(;;){
  unsigned char commands[64];ssize_t n=closing?-1:read(STDIN_FILENO,commands,sizeof commands);
  if(n==0)closing=1;
  if(n<0&&errno!=EAGAIN&&errno!=EINTR&&!closing)closing=1;
  for(ssize_t c=0;c<n;c++)if(commands[c]<7){for(int i=0;i<VOICES;i++)if(voices[i].sample<0){voices[i]=(Voice){commands[c],0};break;}}
  #ifndef AUDIO_OFFLINE
  if(gain_tick++%10==0)gain_level=stock_gain();
#endif
  int16_t out[BLOCK*2];int active=0;
  for(int f=0;f<BLOCK;f++){
   int target_gain=gain_level*65536;if(gain_current<target_gain){gain_current+=256;if(gain_current>target_gain)gain_current=target_gain;}else if(gain_current>target_gain){gain_current-=256;if(gain_current<target_gain)gain_current=target_gain;}
   int mixed[2]={0};
   for(int i=0;i<VOICES;i++)if(voices[i].sample>=0){Sample*s=&samples[voices[i].sample];size_t p=voices[i].pos;
    if(p>=s->frames){voices[i].sample=-1;continue;}
    active=1;/* two millisecond edge ramp prevents discontinuities */
    size_t edge=p<s->frames-1-p?p:s->frames-1-p;int gain=edge<96?(int)edge:96;
    for(int ch=0;ch<2;ch++)mixed[ch]+=s->data[p*2+ch]*gain/96;
    voices[i].pos++;
   }
   for(int ch=0;ch<2;ch++){int v=(int)((int64_t)mixed[ch]*gain_current/655360);out[f*2+ch]=(int16_t)(v>32767?32767:v<-32768?-32768:v);}
  }
  int sent=0,written=0;while(sent<BLOCK&&(written=pcm_writei(pcm,out+sent*2,BLOCK-sent))>0)sent+=written;
  if(sent<BLOCK){fprintf(stderr,"audio write: %s\n",pcm_get_error(pcm));if(++errors>3)break;pcm_prepare(pcm);}else errors=0;
  if(closing&&!active){if(++tail>=5)break;}else tail=0;
 }
 pcm_close(pcm);for(int i=0;i<7;i++)free(samples[i].data);return errors?1:0;
}
