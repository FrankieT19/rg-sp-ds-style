/* CPU adaptation of Matt Akins' Pixel Transparency v2.2 equations:
 * https://github.com/libretro/slang-shaders/tree/master/handheld/shaders/pixel_transparency
 * Fixed neutral backing, bright-pixel transparency, polarizer, highlights and
 * 3x3 weighted shadow blur. X/Y shadow offset = 1 (output pixel at 720x480).
 * Motion/shimmer are disabled. LCD is applied first, when independently on.
 * Rendering is cached once per frame, never recursively sampled per pixel. */
static uint32_t old_pt_original[720*480];
static float old_pt_luma[720*480],old_pt_paper[720*480];static int old_pt_paper_ready;
static float old_pt_fract(float v){return v-floorf(v);}
static float old_pt_hash(float x,float y){float a=old_pt_fract(x*.1031f),b=old_pt_fract(y*.1031f),c=a,d=a*(b+33.33f)+b*(c+33.33f)+c*(a+33.33f);a+=d;b+=d;c+=d;return old_pt_fract((a+b)*c);}
static float old_pt_clamp(float v){return v<0?0:v>1?1:v;}
static float old_pt_at(int x,int y){if(x<0)x=0;if(y<0)y=0;if(x>719)x=719;if(y>479)y=479;return old_pt_luma[y*720+x];}
/* A source pixel can affect its own output and shadows up to two pixels
 * right/down. Dirty 16x16 tiles include that exact dependency halo. */
static unsigned char old_pt_dirty[45*30];static int old_pt_cached,old_pt_previous_lcd;
static void old_pt_prepare(void){
 if(old_pt_valid)return;old_pt_building=1;int lcd=lcd_grid;lcd_grid=0;
 int all=!old_pt_cached||old_pt_previous_lcd!=lcd;memset(old_pt_dirty,all?1:0,sizeof old_pt_dirty);
 uint32_t row[720];
 for(int y=0;y<480;y++){
  output_row3(row,y,0);
  for(int x=0;x<720;x++){int i=y*720+x;uint32_t c=row[x];if(!old_pt_cached||c!=old_pt_original[i]){
   old_pt_original[i]=c;old_pt_luma[i]=(((c>>16)&255)*.2126f+((c>>8)&255)*.7152f+(c&255)*.0722f)/255.0f;
   if(!all){int right=(x+2<720?x+2:719)/16,bottom=(y+2<480?y+2:479)/16;
    old_pt_dirty[(y/16)*45+x/16]=1;old_pt_dirty[(y/16)*45+right]=1;old_pt_dirty[bottom*45+x/16]=1;old_pt_dirty[bottom*45+right]=1;}
  }}
 }
 lcd_grid=lcd;
 if(!old_pt_paper_ready){for(int y=0;y<480;y++)for(int x=0;x<720;x++){float a=(x+.5f)/720*128,b=(y+.5f)/480*128;old_pt_paper[y*720+x]=.48f+(old_pt_hash(a,b)*.5f+old_pt_hash(a*2,b*2)*.25f+old_pt_hash(a*4,b*4)*.125f-.4375f)*.065f;}old_pt_paper_ready=1;}
 if(lcd)prepare_lcd(3);
 for(int ty=0;ty<30;ty++)for(int tx=0;tx<45;tx++){
  if(!old_pt_dirty[ty*45+tx])continue;
  for(int y=ty*16;y<ty*16+16;y++)for(int x=tx*16;x<tx*16+16;x++){
  int i=y*720+x;float blur=0;for(int yy=-1;yy<=1;yy++)for(int xx=-1;xx<=1;xx++)blur+=old_pt_at(x-1+xx,y-1+yy)*(xx?1:2)*(yy?1:2);
  float shadow=(1-blur/16)*.5f,t=old_pt_clamp(shadow/.05f);shadow*=t*t*(3-2*t);
  float paper=old_pt_paper[i]*(1-.8f*shadow),alpha=old_pt_clamp(old_pt_luma[i]*.533f),out[3];
  const float backing[3]={.675f,.73f/.766f*.675f,.763f/.766f*.675f},polarizer[3]={.94f,1,.865f};
  for(int ch=0;ch<3;ch++){unsigned v=(old_pt_original[i]>>(16-ch*8))&255;float source=(lcd?lcd_table[(y%3)*3+x%3][v]:v)/255.0f,bg=old_pt_clamp(backing[ch]+paper*2-1);out[ch]=(source*(1-alpha)+bg*alpha)*polarizer[ch];}
  float lift=1+.05f*(out[0]*.2126f+out[1]*.7152f+out[2]*.0722f);uint32_t rgb=0;for(int ch=0;ch<3;ch++)rgb=(rgb<<8)|(unsigned)(old_pt_clamp(out[ch]*lift)*255+.5f);old_pt_frame[i]=rgb;
 }
 }
 old_pt_cached=1;old_pt_previous_lcd=lcd;old_pt_building=0;old_pt_valid=1;
}
