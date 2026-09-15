/* Half-step UI control; the stock node remains a legal integer 0..10. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "volume_steps.h"
#ifdef _WIN32
#include <windows.h>
#define REPLACE(a,b) (!MoveFileExA(a,b,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
#else
#define REPLACE rename
#endif
int main(int argc,char**argv){
 if(argc!=4)return 2;int up=!strcmp(argv[1],"up"),down=!strcmp(argv[1],"down"),get=!strcmp(argv[1],"get");if(!up&&!down&&!get)return 2;
 const char*root=getenv("DS_STYLE_TEST_ROOT");if(!root)root="";char node[2048],path[2048],tmp[2060];snprintf(node,sizeof node,"%s/sys/class/power_supply/axp2202-battery/openbor_volume",root);
 int previous=atoi(argv[2]),current=-1;FILE*f=fopen(node,"r");if(f){if(fscanf(f,"%d",&current)!=1)current=-1;fclose(f);}if(current<0||current>10)return 1;
 int step=volume_saved(argv[3],current);
 if(!get){
  /* A stock listener may already have processed this same key: replace its
   * whole step with our half step, rather than adding a second increment. */
  if(previous>=0&&previous<=20&&(current==volume_coarse(previous)||current==volume_coarse(previous)+(up?1:-1)))step=previous;
  step+=up?1:-1;if(step<0)step=0;if(step>20)step=20;
  int next=volume_coarse(step);f=fopen(node,"w");if(!f)return 1;int ok=fprintf(f,"%d",next)>0;if(fclose(f))ok=0;if(!ok)return 1;
  snprintf(path,sizeof path,"%s/state/volume-step.txt",argv[3]);snprintf(tmp,sizeof tmp,"%s.tmp",path);f=fopen(tmp,"w");if(!f)return 1;ok=fprintf(f,"%d %d\n",step,next)>0;if(fclose(f))ok=0;if(!ok||REPLACE(tmp,path))return 1;
 }
 printf("%d\n",step);return 0;
}
