#define _GNU_SOURCE
/* DS Style UI-only PCM worker. Holds hw:0,0 until stdin closes, then finishes
 * queued sounds and releases it before launching any stock emulator. */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "audio_curve.h"
#ifndef AUDIO_OFFLINE
#include "stock_stream.h"
static int stock_gain(const char*base){int level=0;FILE*f=fopen("/sys/class/power_supply/axp2202-battery/openbor_volume","r");if(f){if(fscanf(f,"%d",&level)!=1)level=0;fclose(f);}level=level<0?0:level>10?10:level;return volume_saved(base,level);}
#else
#include <tinyalsa/asoundlib.h>
#endif
#ifdef AUDIO_OFFLINE
#include "../tests/audio_mock.h"
#endif
enum { BLOCK=256, VOICES=16 };
typedef struct { int16_t *data; size_t frames; } Sample;
typedef struct { int sample; size_t pos; } Voice;
static const char *names[]={"accept","back","launch","menu","move","startup","tab"};

static const int event_gain[]={200,350,200,200,200,105,200};
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
 if(argc!=2)return 2;audio_curves();Sample samples[7];Voice voices[VOICES];
 for(int i=0;i<7;i++){samples[i]=load(argv[1],names[i]);fprintf(stderr,"audio sample %s: %zu frames\n",names[i],samples[i].frames);}
 for(int i=0;i<VOICES;i++)voices[i].sample=-1;
 struct pcm_config cfg={.channels=2,.rate=48000,.period_size=BLOCK,.period_count=4,.format=PCM_FORMAT_S16_LE};
 struct pcm *pcm=pcm_open(0,0,PCM_OUT,&cfg);
 if(!pcm_is_ready(pcm)){fprintf(stderr,"audio open: %s\n",pcm_get_error(pcm));pcm_close(pcm);return 1;}
#ifndef AUDIO_OFFLINE
 /* Stock tinyplay owns the PCM ABI and leaves mixer configuration intact. */
#endif
 fprintf(stderr,"audio: continuous stock tinyplay stream 48000 stereo, period %d\n",BLOCK);
#ifndef AUDIO_OFFLINE
 fcntl(STDIN_FILENO,F_SETFL,fcntl(STDIN_FILENO,F_GETFL)|O_NONBLOCK);
#endif
 int16_t warm[256*2]={0};for(int i=0;i<24;i++)if(pcm_writei(pcm,warm,256)!=256){pcm_close(pcm);return 1;}
 int closing=0,tail=0,errors=0,gain_level=20,gain_tick=0,gain_current=655360,boot_current=655360;
#ifndef AUDIO_OFFLINE
 gain_level=stock_gain(argv[1]);gain_current=navigation_curve[gain_level]*65536;
#else
 if(getenv("AUDIO_LEVEL"))gain_level=atoi(getenv("AUDIO_LEVEL"));if(gain_level<0)gain_level=0;if(gain_level>20)gain_level=20;gain_current=navigation_curve[gain_level]*65536;
#endif
 boot_current=startup_curve[gain_level]*65536;fprintf(stderr,"audio: UI volume %d/20; stock %d/10\n",gain_level,volume_coarse(gain_level));
 for(;;){
  unsigned char commands[64];ssize_t n=closing?-1:read(STDIN_FILENO,commands,sizeof commands);
  if(n==0)closing=1;
  if(n<0&&errno!=EAGAIN&&errno!=EINTR&&!closing)closing=1;
  for(ssize_t c=0;c<n;c++)if(commands[c]<7){for(int i=0;i<VOICES;i++)if(voices[i].sample<0){voices[i]=(Voice){commands[c],0};break;}}
  #ifndef AUDIO_OFFLINE
  if(gain_tick++%10==0)gain_level=stock_gain(argv[1]);
#endif
  int16_t out[BLOCK*2];int active=0;
  for(int f=0;f<BLOCK;f++){
   int target_gain=navigation_curve[gain_level]*65536;if(gain_current<target_gain){gain_current+=25600;if(gain_current>target_gain)gain_current=target_gain;}else if(gain_current>target_gain){gain_current-=25600;if(gain_current<target_gain)gain_current=target_gain;}
   int boot_target=startup_curve[gain_level]*65536;if(boot_current<boot_target){boot_current+=25600;if(boot_current>boot_target)boot_current=boot_target;}else if(boot_current>boot_target){boot_current-=25600;if(boot_current<boot_target)boot_current=boot_target;}
   int mixed[2]={0},boot[2]={0};
   for(int i=0;i<VOICES;i++)if(voices[i].sample>=0){Sample*s=&samples[voices[i].sample];size_t p=voices[i].pos;
    if(p>=s->frames){voices[i].sample=-1;continue;}
    active=1;/* two millisecond edge ramp prevents discontinuities */
    size_t edge=p<s->frames-1-p?p:s->frames-1-p;int gain=edge<96?(int)edge:96;
    for(int ch=0;ch<2;ch++){int v=(s->data[p*2+ch]*gain/96)*event_gain[voices[i].sample]/200;if(voices[i].sample==5)boot[ch]+=v;else mixed[ch]+=v;}
    voices[i].pos++;
   }
   for(int ch=0;ch<2;ch++){int v=(int)(((int64_t)mixed[ch]*gain_current+(int64_t)boot[ch]*boot_current)/65536000);out[f*2+ch]=(int16_t)(v>32767?32767:v<-32768?-32768:v);}
  }
  int sent=0,written=0;while(sent<BLOCK&&(written=pcm_writei(pcm,out+sent*2,BLOCK-sent))>0)sent+=written;
  if(sent<BLOCK){fprintf(stderr,"audio write: %s\n",pcm_get_error(pcm));if(++errors>3)break;pcm_prepare(pcm);}else errors=0;
  if(closing&&!active){if(++tail>=64)break;}else tail=0;
 }
 pcm_close(pcm);for(int i=0;i<7;i++)free(samples[i].data);return errors?1:0;
}
