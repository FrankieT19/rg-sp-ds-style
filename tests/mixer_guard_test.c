#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define MIXER_GUARD_TEST
#define O_RDWR 2
#define O_CLOEXEC 0
#define SNDRV_CTL_ELEM_TYPE_INTEGER 2
#define SNDRV_CTL_ELEM_ACCESS_WRITE 2
#define SNDRV_CTL_IOCTL_ELEM_LIST 1
#define SNDRV_CTL_IOCTL_ELEM_INFO 2
#define SNDRV_CTL_IOCTL_ELEM_READ 3
#define SNDRV_CTL_IOCTL_ELEM_WRITE 4
struct snd_ctl_elem_id{unsigned numid;unsigned char name[64];};
struct snd_ctl_elem_info{struct snd_ctl_elem_id id;unsigned type,count,access;union{struct{long min,max;}integer;}value;};
struct snd_ctl_elem_value{struct snd_ctl_elem_id id;union{struct{long value[128];}integer;}value;};
struct snd_ctl_elem_list{unsigned count,used,space;struct snd_ctl_elem_id*pids;};
static const char*names[]={"digital volume","DAC Playback Volume","Mic Capture Volume","Headphone Switch","Audio Route"};
static long levels[]={12,20,30,1,2};static int writes,changed_schema;
static int mock_open(const char*p,int flags){(void)p;(void)flags;return 3;}
static int mock_close(int fd){(void)fd;return 0;}
static int mock_ioctl(int fd,int cmd,void*ptr){(void)fd;
 if(cmd==1){struct snd_ctl_elem_list*l=ptr;l->count=5;if(l->pids){l->used=5;for(int i=0;i<5;i++){l->pids[i].numid=i;strcpy((char*)l->pids[i].name,names[i]);}}return 0;}
 if(cmd==2){struct snd_ctl_elem_info*i=ptr;unsigned n=i->id.numid;i->type=2;i->count=1;i->access=2;i->value.integer.min=0;i->value.integer.max=changed_schema?31:63;strcpy((char*)i->id.name,names[n]);return 0;}
 struct snd_ctl_elem_value*v=ptr;if(cmd==3)v->value.integer.value[0]=levels[v->id.numid];else{levels[v->id.numid]=v->value.integer.value[0];writes++;}return 0;
}
#define open mock_open
#define close mock_close
#define ioctl mock_ioctl
#include "../source/mixer_guard.h"
int main(void){
 ui_mixer_save();assert(ui_mixer_count==2);levels[0]=60;levels[1]=1;levels[2]=42;levels[3]=0;levels[4]=3;ui_mixer_restore();
 assert(writes==2&&levels[0]==12&&levels[1]==20&&levels[2]==42&&levels[3]==0&&levels[4]==3);
 ui_mixer_save();ui_mixer_restore();assert(writes==2);
 ui_mixer_save();levels[0]=50;changed_schema=1;ui_mixer_restore();assert(writes==2&&levels[0]==50);
 assert(!ui_playback_gain("ADC Volume")&&!ui_playback_gain("Mic Boost Volume"));
 puts("Mixer gains restored; capture/routing untouched; unchanged values and changed schemas not written");return 0;
}
