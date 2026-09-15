#define main dsstyle_frontend_main
#include "../source/dsstyle.c"
#undef main
#include <assert.h>
#ifdef NDEBUG
#error Tests require enabled assertions
#endif
#include "pt_v216_baseline.h"
static unsigned char fixture[147*97*4];
static void scene217(int frame){
 preview_frame_time=10000+(frame%12)*16;page=0;about_page=-1;homechoice=frame%2?2:1;home_animation_at=10000;
 for(int c=0;c<4;c++){int x,y;home_corner_position(0,c,&x,&y);home_from_x[c]=x;home_from_y[c]=y;}
 draw();native_art_count=1;native_art[0]=(NativeArt){{fixture,147,97,0},32,48,56,37};
 for(int y=48;y<85;y++)for(int x=32;x<88;x++)art_owner[y*W+x]=1;
}
int main(int argc,char**argv){assert(argc==3);strcopy(base,argv[1],sizeof base);preview=demo=1;loadassets();ui_load();pixel_transparency=0;
 for(unsigned i=0;i<sizeof fixture;i++)fixture[i]=(unsigned char)(i*37);
 unsigned long sampled=0;
 for(int f=0;f<180;f++){
  dark_mode=(f/45)%2;lcd_grid=(f/30)%2;pt_shadow_blur=old216_pt_shadow_blur=(f/90)%2;
  if(f%40==0){fixture[33]^=255;image_generation++;}
  scene217(f);if(f%11==0){native_art[0].x++;}if(f%13==0)rect(2,2,3,3,0x123456);
  old216_pt_valid=0;pt_prepare();old216_pt_prepare();sampled+=pt_resampled;
  assert(!memcmp(pt_frame,old216_pt_frame,sizeof pt_frame));
 }
 printf("180 animated native-art, theme, LCD, blur and image-change frames: pixel-identical to v2.16; mean sampled=%lu/345600\n",sampled/180);
 pt_shadow_blur=old216_pt_shadow_blur=0;lcd_grid=1;dark_mode=0;
 for(int mode=0;mode<2;mode++){
  uint64_t start=GetTickCount64();for(int f=0;f<1500;f++){scene217(f);if(mode)pt_prepare();else{old216_pt_valid=0;old216_pt_prepare();}}
  printf("home benchmark %s frames=1500 ms=%llu\n",mode?"v217":"v216",(unsigned long long)(GetTickCount64()-start));
 }
 scene217(0);home_animation_at=0;draw();pt_prepare();draw();pt_prepare();assert(pt_resampled==0);puts("Static warm frame reuses every filtered source pixel");
 page=2;about_page=-1;settings_category=-1;category_choice=8;action_impl(ACCEPT);assert(about_page==0);about_action(RIGHT);assert(about_page==1);about_action(START);assert(snake_active&&launcher_snake_length==4);assert(launcher_snake_x[0]==10&&launcher_snake_y[0]==7);
 about_action(LEFT);assert(snake_dx==1);about_action(UP);assert(snake_dy==-1);uint32_t tx,ty,grew;launcher_snake_food_x=10;launcher_snake_food_y=6;assert(Launcher_SnakeStep(0,-1,&tx,&ty,&grew)&&grew&&launcher_snake_length==5);assert(!Launcher_SnakeStep(0,1,&tx,&ty,&grew));
 snake_over=1;about_action(ACCEPT);assert(!snake_over&&launcher_snake_length==4);snake_next=preview_frame_time+134;assert(!snake_tick());preview_frame_time+=134;assert(snake_tick());about_action(BACK);assert(!snake_active&&about_page==1);about_action(BACK);assert(about_page==-1);
 page=2;about_page=0;draw();char file[PATHLEN];join(file,PATHLEN,argv[2],"about.bmp");assert(bmp(file));
 about_action(START);draw();assert(pixels[33*W+40]==Launcher_SnakeGridColour());assert(pixels[90*W+121]==theme_accent[colour]);join(file,PATHLEN,argv[2],"snake.bmp");assert(bmp(file));snake_over=1;draw();join(file,PATHLEN,argv[2],"snake-over.bmp");assert(bmp(file));
 puts("About entry, Snake rules, timing, reverse rejection, growth/collision, replay and return passed");return 0;
}
