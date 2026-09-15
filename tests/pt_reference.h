/* CPU adaptation of Matt Akins' Pixel Transparency v2.2 equations:
 * https://github.com/libretro/slang-shaders/tree/master/handheld/shaders/pixel_transparency
 * Fixed neutral backing, bright-pixel transparency, polarizer, highlights and
 * Unblurred shadow for lower CPU cost. X/Y shadow offset = 1 (output pixel at 720x480).
 * Motion/shimmer are disabled. LCD is applied first, when independently on.
 * Rendering is cached once per frame, never recursively sampled per pixel. */
static uint32_t ref_pt_original[720*480];
static float ref_pt_luma[720*480],ref_pt_paper[720*480];static int ref_pt_paper_ready;
static float ref_pt_fract(float v){return v-floorf(v);}
static float ref_pt_hash(float x,float y){float a=ref_pt_fract(x*.1031f),b=ref_pt_fract(y*.1031f),c=a,d=a*(b+33.33f)+b*(c+33.33f)+c*(a+33.33f);a+=d;b+=d;c+=d;return ref_pt_fract((a+b)*c);}
static float ref_pt_clamp(float v){return v<0?0:v>1?1:v;}
static float ref_pt_at(int x,int y){if(x<0)x=0;if(y<0)y=0;if(x>719)x=719;if(y>479)y=479;return ref_pt_luma[y*720+x];}
static void ref_pt_prepare(void){
 if(ref_pt_valid)return;ref_pt_building=1;int lcd=lcd_grid;lcd_grid=0;
 for(int y=0;y<480;y++)output_row3(ref_pt_original+y*720,y,0);lcd_grid=lcd;
 for(int i=0;i<720*480;i++){uint32_t c=ref_pt_original[i];ref_pt_luma[i]=(((c>>16)&255)*.2126f+((c>>8)&255)*.7152f+(c&255)*.0722f)/255.0f;}
 if(!ref_pt_paper_ready){for(int y=0;y<480;y++)for(int x=0;x<720;x++){float a=(x+.5f)/720*128,b=(y+.5f)/480*128;ref_pt_paper[y*720+x]=.48f+(ref_pt_hash(a,b)*.5f+ref_pt_hash(a*2,b*2)*.25f+ref_pt_hash(a*4,b*4)*.125f-.4375f)*.065f;}ref_pt_paper_ready=1;}
 if(lcd)prepare_lcd(3);
 for(int y=0;y<480;y++)for(int x=0;x<720;x++){
  int i=y*720+x;
  float sample=ref_pt_at(x-1,y-1);if(pt_shadow_blur){sample=0;for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)sample+=ref_pt_at(x-1+dx,y-1+dy)*(dx?1:2)*(dy?1:2);sample/=16;}
  float shadow=(1-sample)*.5f,t=ref_pt_clamp(shadow/.05f);shadow*=t*t*(3-2*t);
  float paper=ref_pt_paper[i]*(1-.8f*shadow),alpha=ref_pt_clamp(ref_pt_luma[i]*.533f),out[3];
  /* Keep 70% of the filter's colour cast; source colours are untouched. */
  const float backing[3]={.675f,.675f+.7f*(.73f/.766f*.675f-.675f),.675f+.7f*(.763f/.766f*.675f-.675f)},polarizer[3]={1+.7f*(.94f-1),1,1+.7f*(.865f-1)};
  for(int ch=0;ch<3;ch++){unsigned v=(ref_pt_original[i]>>(16-ch*8))&255;float source=(lcd?lcd_table[(y%3)*3+x%3][v]:v)/255.0f,bg=ref_pt_clamp(backing[ch]+paper*2-1);out[ch]=(source*(1-alpha)+bg*alpha)*polarizer[ch];}
  float lift=1+.05f*(out[0]*.2126f+out[1]*.7152f+out[2]*.0722f);uint32_t rgb=0;for(int ch=0;ch<3;ch++)rgb=(rgb<<8)|(unsigned)(ref_pt_clamp(out[ch]*lift)*255+.5f);ref_pt_frame[i]=rgb;
 }
 ref_pt_building=0;ref_pt_valid=1;
}
