static int old216_pt_valid,old216_pt_building;static uint32_t old216_pt_frame[720*480];
/* CPU adaptation of Matt Akins' Pixel Transparency v2.2 equations:
 * https://github.com/libretro/slang-shaders/tree/master/handheld/shaders/pixel_transparency
 * Fixed neutral backing, bright-pixel transparency, polarizer, highlights and
 * Unblurred shadow for lower CPU cost. X/Y shadow offset = 1 (output pixel at 720x480).
 * Motion/shimmer are disabled. LCD is applied first, when independently on.
 * Rendering is cached once per frame, never recursively sampled per pixel. */
static int old216_pt_shadow_blur=0,old216_pt_previous_blur;
static uint32_t old216_pt_original[720*480];
static float old216_pt_luma[720*480],old216_pt_paper[720*480];static int old216_pt_paper_ready;
static float old216_pt_fract(float v){return v-floorf(v);}
static float old216_pt_hash(float x,float y){float a=old216_pt_fract(x*.1031f),b=old216_pt_fract(y*.1031f),c=a,d=a*(b+33.33f)+b*(c+33.33f)+c*(a+33.33f);a+=d;b+=d;c+=d;return old216_pt_fract((a+b)*c);}
static float old216_pt_clamp(float v){return v<0?0:v>1?1:v;}
static float old216_pt_at(int x,int y){if(x<0)x=0;if(y<0)y=0;if(x>719)x=719;if(y>479)y=479;return old216_pt_luma[y*720+x];}
/* Exact per-pixel invalidation inside dirty tiles; two-pixel shadow halo. */
static uint32_t old216_pt_logical[W*H];static unsigned char old216_pt_was_special[H];
static unsigned char old216_pt_changed[720*480];
static unsigned char old216_pt_dirty[45*30];static int old216_pt_cached,old216_pt_previous_lcd;
static void old216_pt_mark(int x,int y){if(x>=720||y>=480)return;old216_pt_changed[y*720+x]=1;old216_pt_dirty[(y/16)*45+x/16]=1;}
static void old216_pt_prepare(void){
 if(old216_pt_valid)return;old216_pt_building=1;int lcd=lcd_grid;lcd_grid=0;
 int all=!old216_pt_cached||old216_pt_previous_lcd!=lcd||old216_pt_previous_blur!=old216_pt_shadow_blur;memset(old216_pt_dirty,all?1:0,sizeof old216_pt_dirty);memset(old216_pt_changed,all?1:0,sizeof old216_pt_changed);
 uint32_t row[720];
 for(int ly=0;ly<H;ly++){
  int special=scroll_active&&ly>=scroll_y&&ly<scroll_y+12;
  for(int x=0;!special&&x<W;x++)special=art_owner[ly*W+x]||motion_owner[ly*W+x];
  if(old216_pt_cached&&!special&&!old216_pt_was_special[ly]&&!memcmp(pixels+ly*W,old216_pt_logical+ly*W,W*sizeof(uint32_t)))continue;
  memcpy(old216_pt_logical+ly*W,pixels+ly*W,W*sizeof(uint32_t));old216_pt_was_special[ly]=(unsigned char)special;
  for(int y=ly*3;y<ly*3+3;y++){
  output_row3(row,y,0);
  if(old216_pt_cached&&!memcmp(row,old216_pt_original+y*720,sizeof row))continue;
  for(int x=0;x<720;x++){int i=y*720+x;uint32_t c=row[x];if(!old216_pt_cached||c!=old216_pt_original[i]){
   old216_pt_original[i]=c;old216_pt_luma[i]=(((c>>16)&255)*.2126f+((c>>8)&255)*.7152f+(c&255)*.0722f)/255.0f;
   if(!all){if(old216_pt_shadow_blur){for(int dy=0;dy<3;dy++)for(int dx=0;dx<3;dx++)old216_pt_mark(x+dx,y+dy);}else{old216_pt_mark(x,y);old216_pt_mark(x+1,y+1);if(!x)old216_pt_mark(0,y+1);if(!y)old216_pt_mark(x+1,0);}}

  }}
 }}
 lcd_grid=lcd;
 if(!old216_pt_paper_ready){for(int y=0;y<480;y++)for(int x=0;x<720;x++){float a=(x+.5f)/720*128,b=(y+.5f)/480*128;old216_pt_paper[y*720+x]=.48f+(old216_pt_hash(a,b)*.5f+old216_pt_hash(a*2,b*2)*.25f+old216_pt_hash(a*4,b*4)*.125f-.4375f)*.065f;}old216_pt_paper_ready=1;}
 if(lcd)prepare_lcd(3);
 for(int ty=0;ty<30;ty++)for(int tx=0;tx<45;tx++){
  if(!old216_pt_dirty[ty*45+tx])continue;
  for(int y=ty*16;y<ty*16+16;y++)for(int x=tx*16;x<tx*16+16;x++){
  int i=y*720+x;if(!old216_pt_changed[i])continue;
  float sample=old216_pt_at(x-1,y-1);if(old216_pt_shadow_blur){sample=0;for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)sample+=old216_pt_at(x-1+dx,y-1+dy)*(dx?1:2)*(dy?1:2);sample/=16;}
  float shadow=(1-sample)*.5f,t=old216_pt_clamp(shadow/.05f);shadow*=t*t*(3-2*t);
  float paper=old216_pt_paper[i]*(1-.8f*shadow),alpha=old216_pt_clamp(old216_pt_luma[i]*.533f),out[3];
  /* Keep 70% of the filter's colour cast; source colours are untouched. */
  const float backing[3]={.675f,.675f+.7f*(.73f/.766f*.675f-.675f),.675f+.7f*(.763f/.766f*.675f-.675f)},polarizer[3]={1+.7f*(.94f-1),1,1+.7f*(.865f-1)};
  for(int ch=0;ch<3;ch++){unsigned v=(old216_pt_original[i]>>(16-ch*8))&255;float source=(lcd?lcd_table[(y%3)*3+x%3][v]:v)/255.0f,bg=old216_pt_clamp(backing[ch]+paper*2-1);out[ch]=(source*(1-alpha)+bg*alpha)*polarizer[ch];}
  float lift=1+.05f*(out[0]*.2126f+out[1]*.7152f+out[2]*.0722f);uint32_t rgb=0;for(int ch=0;ch<3;ch++)rgb=(rgb<<8)|(unsigned)(old216_pt_clamp(out[ch]*lift)*255+.5f);old216_pt_frame[i]=rgb;
 }
 }
 old216_pt_cached=1;old216_pt_previous_lcd=lcd;old216_pt_previous_blur=old216_pt_shadow_blur;old216_pt_building=0;old216_pt_valid=1;
}
