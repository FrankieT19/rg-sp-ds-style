#include <math.h>
#include "volume_steps.h"
static int navigation_curve[21],startup_curve[21];
static void audio_curves(void){
 unsigned q[11]={0,84,211,581,1460,3050,5181,6522,8211,10337,16384};
#ifndef AUDIO_OFFLINE
 /* Checked stock accessor loads Q14 gain at 0x440cc; never execute firmware. */
 FILE*f=fopen("/mnt/vendor/bin/dmenu.bin","rb");if(f){
  const unsigned char signature[]={29,75,30,74,30,104,3,241,8,5,82,248,38,96};unsigned char actual[sizeof signature];unsigned live[11];
  if(!fseek(f,0x25a18,SEEK_SET)&&fread(actual,1,sizeof actual,f)==sizeof actual&&!memcmp(actual,signature,sizeof actual)&&!fseek(f,0x340cc,SEEK_SET)&&fread(live,4,11,f)==11){
   int valid=live[0]==0&&live[10]==16384;for(int i=1;i<11;i++)valid=valid&&live[i]>live[i-1]&&live[i]<=16384;if(valid)memcpy(q,live,sizeof q);
  }fclose(f);
 }
#endif
 const int old[11]={0,12,30,55,90,135,185,245,315,400,500};
 for(int s=0;s<=20;s++){
  int lo=s/2;double gain=q[lo]/16384.0;
  if(s&1)gain=lo?sqrt(gain*q[lo+1]/16384.0):q[1]/32768.0;
  /* Quieter perceptual UI curve: gentle low end; peak reduced 25%.
   * Stock/game percentage is never remapped. Intermediates use dB spacing. */
  navigation_curve[s]=s?(int)(375*pow(gain,.65)+.5):0;
  startup_curve[s]=(s&1)?(old[lo]+old[lo+1])/2:old[lo];
 }
}
