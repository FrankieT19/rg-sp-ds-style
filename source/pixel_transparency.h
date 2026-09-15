/* CPU adaptation of Matt Akins' Pixel Transparency v2.2 equations:
 * https://github.com/libretro/slang-shaders/tree/master/handheld/shaders/pixel_transparency
 * Fixed neutral backing, bright-pixel transparency, polarizer, highlights and
 * Unblurred shadow for lower CPU cost. X/Y shadow offset = 1 (output pixel at 720x480).
 * Motion/shimmer are disabled. LCD is applied first, when independently on.
 * Rendering is cached once per frame, never recursively sampled per pixel. */
static int pt_shadow_blur=0,pt_previous_blur;
static uint32_t pt_original[720*480];
static float pt_luma[720*480],pt_paper[720*480];static int pt_paper_ready;
static float pt_fract(float v){return v-floorf(v);}
static float pt_hash(float x,float y){float a=pt_fract(x*.1031f),b=pt_fract(y*.1031f),c=a,d=a*(b+33.33f)+b*(c+33.33f)+c*(a+33.33f);a+=d;b+=d;c+=d;return pt_fract((a+b)*c);}
static float pt_clamp(float v){return v<0?0:v>1?1:v;}
static float pt_at(int x,int y){if(x<0)x=0;if(y<0)y=0;if(x>719)x=719;if(y>479)y=479;return pt_luma[y*720+x];}
/* Exact per-pixel invalidation inside dirty tiles; two-pixel shadow halo. */
static uint32_t pt_logical[W*H];
static unsigned char pt_old_art[W*H],pt_old_motion[W*H];
static NativeArt pt_old_native[8];static unsigned pt_old_generation;
static int pt_old_scroll,pt_old_scroll_y;
static unsigned long pt_resampled; /* Host profiling: actual source pixels sampled. */
static unsigned char pt_changed[720*480];
static unsigned char pt_dirty[45*30];static int pt_cached,pt_previous_lcd;
static void pt_mark(int x,int y){if(x>=720||y>=480)return;pt_changed[y*720+x]=1;pt_dirty[(y/16)*45+x/16]=1;}
static void pt_prepare(void){
 if(pt_valid)return;pt_building=1;int lcd=lcd_grid;lcd_grid=0;
 int all=!pt_cached||pt_previous_lcd!=lcd||pt_previous_blur!=pt_shadow_blur;memset(pt_dirty,all?1:0,sizeof pt_dirty);memset(pt_changed,all?1:0,sizeof pt_changed);
 /* Reuse the pre-rendered source and filtered output for static cells,
  * including full-resolution art. Only moving/changed cells are sampled. */
 int art_changed=image_generation!=pt_old_generation||memcmp(native_art,pt_old_native,sizeof native_art);
 pt_resampled=0;
 for(int ly=0;ly<H;ly++)for(int lx=0;lx<W;lx++){
  int logical=ly*W+lx;
  int scrolling=(scroll_active&&ly>=scroll_y&&ly<scroll_y+12)||(pt_old_scroll&&ly>=pt_old_scroll_y&&ly<pt_old_scroll_y+12);
  if(pt_cached&&pixels[logical]==pt_logical[logical]&&!scrolling&&!motion_owner[logical]&&!pt_old_motion[logical]&&art_owner[logical]==pt_old_art[logical]&&(!art_owner[logical]||!art_changed))continue;
  int special=scrolling||art_owner[logical]||motion_owner[logical];
  for(int y=ly*3;y<ly*3+3;y++)for(int x=lx*3;x<lx*3+3;x++){
   int i=y*720+x;uint32_t c=special?output_pixel(x,y,3):pixels[logical];pt_resampled++;
   if(!pt_cached||c!=pt_original[i]){
    pt_original[i]=c;pt_luma[i]=(((c>>16)&255)*.2126f+((c>>8)&255)*.7152f+(c&255)*.0722f)/255.0f;
    if(!all){if(pt_shadow_blur){for(int dy=0;dy<3;dy++)for(int dx=0;dx<3;dx++)pt_mark(x+dx,y+dy);}else{pt_mark(x,y);pt_mark(x+1,y+1);if(!x)pt_mark(0,y+1);if(!y)pt_mark(x+1,0);}}
   }
  }
 }
 memcpy(pt_logical,pixels,sizeof pt_logical);memcpy(pt_old_art,art_owner,sizeof pt_old_art);memcpy(pt_old_motion,motion_owner,sizeof pt_old_motion);memcpy(pt_old_native,native_art,sizeof native_art);pt_old_generation=image_generation;pt_old_scroll=scroll_active;pt_old_scroll_y=scroll_y;
 lcd_grid=lcd;
 if(!pt_paper_ready){for(int y=0;y<480;y++)for(int x=0;x<720;x++){float a=(x+.5f)/720*128,b=(y+.5f)/480*128;pt_paper[y*720+x]=.48f+(pt_hash(a,b)*.5f+pt_hash(a*2,b*2)*.25f+pt_hash(a*4,b*4)*.125f-.4375f)*.065f;}pt_paper_ready=1;}
 if(lcd)prepare_lcd(3);
 for(int ty=0;ty<30;ty++)for(int tx=0;tx<45;tx++){
  if(!pt_dirty[ty*45+tx])continue;
  for(int y=ty*16;y<ty*16+16;y++)for(int x=tx*16;x<tx*16+16;x++){
  int i=y*720+x;if(!pt_changed[i])continue;
  float sample=pt_at(x-1,y-1);if(pt_shadow_blur){sample=0;for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)sample+=pt_at(x-1+dx,y-1+dy)*(dx?1:2)*(dy?1:2);sample/=16;}
  float shadow=(1-sample)*.5f,t=pt_clamp(shadow/.05f);shadow*=t*t*(3-2*t);
  float paper=pt_paper[i]*(1-.8f*shadow),alpha=pt_clamp(pt_luma[i]*.533f),out[3];
  /* Keep 70% of the filter's colour cast; source colours are untouched. */
  const float backing[3]={.675f,.675f+.7f*(.73f/.766f*.675f-.675f),.675f+.7f*(.763f/.766f*.675f-.675f)},polarizer[3]={1+.7f*(.94f-1),1,1+.7f*(.865f-1)};
  for(int ch=0;ch<3;ch++){unsigned v=(pt_original[i]>>(16-ch*8))&255;float source=(lcd?lcd_table[(y%3)*3+x%3][v]:v)/255.0f,bg=pt_clamp(backing[ch]+paper*2-1);out[ch]=(source*(1-alpha)+bg*alpha)*polarizer[ch];}
  float lift=1+.05f*(out[0]*.2126f+out[1]*.7152f+out[2]*.0722f);uint32_t rgb=0;for(int ch=0;ch<3;ch++)rgb=(rgb<<8)|(unsigned)(pt_clamp(out[ch]*lift)*255+.5f);pt_frame[i]=rgb;
 }
 }
 pt_cached=1;pt_previous_lcd=lcd;pt_previous_blur=pt_shadow_blur;pt_building=0;pt_valid=1;
}
