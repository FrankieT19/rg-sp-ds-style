/* Keep the UI playback-gain baseline across external applications.
 * Never restore routing, jack, capture, mute or the shared stock volume node. */
#include <ctype.h>
static int ui_playback_gain(const char *name){
 char lower[64];size_t i;for(i=0;i<sizeof lower-1&&name[i];i++)lower[i]=(char)tolower((unsigned char)name[i]);lower[i]=0;
 return strstr(lower,"volume")&&!strstr(lower,"capture")&&!strstr(lower,"mic")&&!strstr(lower,"adc")&&!strstr(lower,"boost")&&!strstr(lower,"switch");
}
#if !defined(_WIN32) || defined(MIXER_GUARD_TEST)
#ifndef MIXER_GUARD_TEST
#include <sound/asound.h>
#endif
#define UI_MIXER_MAX 32
typedef struct {struct snd_ctl_elem_info info;struct snd_ctl_elem_value value;} UiMixerGain;
static UiMixerGain ui_mixer_gain[UI_MIXER_MAX];static unsigned ui_mixer_count;
static void ui_mixer_save(void){
 ui_mixer_count=0;int fd=open("/dev/snd/controlC0",O_RDWR|O_CLOEXEC);if(fd<0)return;
 struct snd_ctl_elem_list list={0};if(ioctl(fd,SNDRV_CTL_IOCTL_ELEM_LIST,&list)||!list.count||list.count>512){close(fd);return;}
 struct snd_ctl_elem_id *ids=calloc(list.count,sizeof *ids);if(!ids){close(fd);return;}list.space=list.count;list.pids=ids;
 if(!ioctl(fd,SNDRV_CTL_IOCTL_ELEM_LIST,&list))for(unsigned i=0;i<list.used&&ui_mixer_count<UI_MIXER_MAX;i++){
  UiMixerGain g={0};g.info.id=ids[i];if(ioctl(fd,SNDRV_CTL_IOCTL_ELEM_INFO,&g.info)||g.info.type!=SNDRV_CTL_ELEM_TYPE_INTEGER||!g.info.count||g.info.count>128||!(g.info.access&SNDRV_CTL_ELEM_ACCESS_WRITE)||!ui_playback_gain((char*)g.info.id.name))continue;
  g.value.id=g.info.id;if(!ioctl(fd,SNDRV_CTL_IOCTL_ELEM_READ,&g.value))ui_mixer_gain[ui_mixer_count++]=g;
 }free(ids);close(fd);
}
static void ui_mixer_restore(void){
 if(!ui_mixer_count)return;int fd=open("/dev/snd/controlC0",O_RDWR|O_CLOEXEC);if(fd<0)return;
 for(unsigned i=0;i<ui_mixer_count;i++){
  UiMixerGain*g=&ui_mixer_gain[i];struct snd_ctl_elem_info info={0};info.id=g->info.id;
  if(ioctl(fd,SNDRV_CTL_IOCTL_ELEM_INFO,&info)||info.type!=g->info.type||info.count!=g->info.count||strcmp((char*)info.id.name,(char*)g->info.id.name)||info.value.integer.min!=g->info.value.integer.min||info.value.integer.max!=g->info.value.integer.max||!(info.access&SNDRV_CTL_ELEM_ACCESS_WRITE))continue;
  struct snd_ctl_elem_value now={0};now.id=info.id;if(ioctl(fd,SNDRV_CTL_IOCTL_ELEM_READ,&now))continue;
  int changed=0,valid=1;for(unsigned j=0;j<info.count;j++){long v=g->value.value.integer.value[j];valid=valid&&v>=info.value.integer.min&&v<=info.value.integer.max;changed|=v!=now.value.integer.value[j];}
  if(valid&&changed){if(ioctl(fd,SNDRV_CTL_IOCTL_ELEM_WRITE,&g->value))fprintf(stderr,"audio: mixer restore failed: %s\n",info.id.name);else fprintf(stderr,"audio: restored UI playback gain: %s\n",info.id.name);}
 }close(fd);ui_mixer_count=0;
}
#endif
