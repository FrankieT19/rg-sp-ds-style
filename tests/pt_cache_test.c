#define main dsstyle_frontend_main
#include "../source/dsstyle.c"
#undef main
static int ref_pt_valid,ref_pt_building;
static uint32_t ref_pt_frame[720*480];
#include "pt_reference.h"
static int old_pt_valid,old_pt_building;
static uint32_t old_pt_frame[720*480];
#include "pt_v29_baseline.h"
static void scene(int frame){
 page=0;draw();rect((frame*7)%236,24+(frame%3),4,5,frame&1?0x384858:0x88ccee);
 if(frame%11==0)rect(238,158,2,2,0x123456);
 if(frame%13==0)rect(0,0,2,2,0xffffff);
 if(frame%17==0)rect(0,70,1,1,0xff0088);
 if(frame%19==0)rect(70,0,1,1,0x00ff88);
}
int main(int argc,char**argv){
 if(argc!=2)return 2;if(pt_shadow_blur){fprintf(stderr,"Production blur must be disabled\n");return 1;}strcopy(base,argv[1],sizeof base);demo=1;loadassets();ui_load();pixel_transparency=0;
 for(int f=0;f<144;f++){
  pt_shadow_blur=f<72;
  dark_mode=(f/18)%2;lcd_grid=(f/9)%2;scene(f);ref_pt_valid=0;pt_prepare();ref_pt_prepare();
  if(memcmp(pt_frame,ref_pt_frame,sizeof pt_frame)){for(int i=0;i<720*480;i++)if(pt_frame[i]!=ref_pt_frame[i]){fprintf(stderr,"Mismatch frame=%d x=%d y=%d %06x != %06x\n",f,i%720,i/720,pt_frame[i],ref_pt_frame[i]);break;}return 1;}
 }
 printf("144 blur-on/off light/dark/LCD frames, edge pixels and shadow halo: exact match\n");
 pt_shadow_blur=1;dark_mode=0;lcd_grid=1;unsigned long long times[3];
 for(int pass=0;pass<3;pass++){
  scene(0);if(pass==1)pt_prepare();else if(pass==2){old_pt_valid=0;old_pt_prepare();}else{ref_pt_valid=0;ref_pt_prepare();}
  uint64_t start=ui_millis();for(int f=0;f<150;f++){scene(f);if(pass==1){pt_prepare();}else if(pass==2){old_pt_valid=0;old_pt_prepare();}else{ref_pt_valid=0;ref_pt_prepare();}}times[pass]=(unsigned long long)(ui_millis()-start);
 }
 printf("benchmark frames=150 reference_ms=%llu cached_ms=%llu\n",times[0],times[1]);printf("v29_comparison frames=150 old_cached_ms=%llu new_cached_ms=%llu\n",times[2],times[1]);for(int blur=0;blur<=1;blur++){
 pt_shadow_blur=blur;scene(0);pt_prepare();uint64_t start=ui_millis();
 for(int f=0;f<1500;f++){scene(f);pt_prepare();}
 printf("blur_comparison blur=%d frames=1500 ms=%llu\n",blur,(unsigned long long)(ui_millis()-start));
 }return 0;
}
