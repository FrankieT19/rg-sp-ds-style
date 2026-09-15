/* DS Style RG SP v2.5.0 -- new portable frontend; no cartridge code executed.
 * Rendering uses the user's unchanged DS Style assets and bitmap glyph data.
 * Linux: framebuffer + evdev, no SDL or new emulator dependency.
 * Windows: GDI preview, same browsing and rendering code, launches disabled.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <errno.h>
#include "fb_layout.h"
#include "volume_steps.h"
#include "original_layout.h"
#include "original_text.h"
#include <math.h>
#include <sys/stat.h>
#include <dirent.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <poll.h>
#include <linux/fb.h>
#include <linux/input.h>
#define MKDIR(p) mkdir(p,0755)
#endif
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_JPEG
#define STBI_MAX_DIMENSIONS 4096
#include "vendor/stb_image.h"

#define W 240
#define H 160
#define PATHLEN 2048
#define MAX_ENTRIES 4096
#define MAX_FAV 512
enum { UP=1, DOWN, LEFT, RIGHT, ACCEPT, BACK, VIEW, FAV, RECENT, MENU, PGUP, PGDN, START, HISTORY, FAVOURITES };
typedef struct { char name[256], path[PATHLEN]; int dir, app, stock_route; } Entry;
typedef struct { unsigned char *p; int w,h,builtin; } Pic;
static uint32_t pixels[W*H];
typedef struct {float x,y;int w,h;} MotionRect;
static MotionRect motion_rects[8];static int motion_count;
static unsigned char motion_owner[W*H];
typedef struct {Pic pic;int x,y,w,h;} NativeArt;
static NativeArt native_art[8];static unsigned char art_owner[W*H];static int native_art_count;

static unsigned char font[256*12];
static unsigned char latin_font[sizeof(latin_codepoints)/sizeof(*latin_codepoints)*12];
static int home_favourites,home_button,hardware_kind,hardware_level,launch_mode_popup,apps_return_settings;
static char launch_system[256];
#ifndef _WIN32
static int hardware_dirty;
static uint64_t last_activity;
#endif
static uint64_t hardware_until;
static int list_folders=1,full_system_names=1,clock_12=1,gba_art,horizontal_fit=1;
static int rounded_corners=2,vertical_side,horizontal_side,list_top,settings_top,stacktop[64],power_confirm;
static char username[80]="DS Style";
static int lcd_grid,clean_list=1,art_position,art_border=3,home_recent,help_page;
static char base[PATHLEN]=".", roots[2][PATHLEN]={"/mnt/mmc/Roms","/mnt/sdcard/Roms"};
static char here[PATHLEN], stack[64][PATHLEN], notice[96];
static int stacksel[64], depth, page, previous, choice, count, homechoice, viewmode=2, colour, running=1;
static int startup_splash;
static int preview, demo, section, setting, exitcode, display_test_seconds;
static Entry entries[MAX_ENTRIES];
static char favourites[MAX_FAV][PATHLEN], recents[50][PATHLEN];
static int favcount, recentcount, launch_route_override;
static int dark_mode,pixel_transparency,apps_in_games,apps_return_systems;
static Pic dark_backgrounds[5];
static Pic backgrounds[5], bars[THEME_COUNT];

static const char *viewnames[]={"List","List + Art","Horizontal","Vertical"};
static void strcopy(char *d,const char *s,size_t n){ if(n) { snprintf(d,n,"%s",s); } }
static int join(char *d,size_t n,const char *a,const char *b){return snprintf(d,n,"%s/%s",a,b)<(int)n;}
static int exists(const char *p){struct stat s;return stat(p,&s)==0;}
static int isdir(const char *p){struct stat s;return stat(p,&s)==0 && S_ISDIR(s.st_mode);}
static const char *filename(const char *p){const char *s=strrchr(p,'/');return s?s+1:p;}
static void statepath(char *p,const char *n){char s[PATHLEN];join(s,sizeof s,base,"state");MKDIR(s);join(p,PATHLEN,s,n);}
static int atomic_text(const char *path,const char *data){char tmp[PATHLEN];if(snprintf(tmp,sizeof tmp,"%s.tmp",path)>=(int)sizeof tmp)return 0;FILE*f=fopen(tmp,"wb");if(!f)return 0;int ok=fwrite(data,1,strlen(data),f)==strlen(data);if(fclose(f))ok=0;if(!ok){remove(tmp);return 0;}
#ifdef _WIN32
 return MoveFileExA(tmp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
 return rename(tmp,path)==0;
#endif
}
static void message(const char*s){strcopy(notice,s,sizeof notice);}
static unsigned image_generation;
static Pic loadpic(const char*p){Pic a={0};int c;image_generation++;a.p=stbi_load(p,&a.w,&a.h,&c,4);return a;}
static void dot(int x,int y,uint32_t c){if(x>=0&&x<W&&y>=0&&y<H){pixels[y*W+x]=c;art_owner[y*W+x]=0;motion_owner[y*W+x]=0;}}
static void rect(int x,int y,int w,int h,uint32_t c){for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++)dot(xx,yy,c);}
static void border(int x,int y,int w,int h,uint32_t c){rect(x,y,w,1,c);rect(x,y+h-1,w,1,c);rect(x,y,1,h,c);rect(x+w-1,y,1,h,c);}
static void blit(Pic a,int x,int y,int w,int h){if(!a.p||w<1||h<1)return;for(int j=0;j<h;j++)for(int i=0;i<w;i++){unsigned char*p=a.p+4*((j*a.h/h)*a.w+i*a.w/w);if(p[3]>127)dot(x+i,y+j,(p[0]<<16)|(p[1]<<8)|p[2]);}}
/* Imported small icons use black as their transparent colour key. */
static void icon(Pic a,int x,int y,int w,int h){if(!a.p)return;for(int j=0;j<h;j++)for(int i=0;i<w;i++){unsigned char*p=a.p+4*((j*a.h/h)*a.w+i*a.w/w);if(p[3]>127&&(p[0]||p[1]||p[2]))dot(x+i,y+j,(p[0]<<16)|(p[1]<<8)|p[2]);}}
/* ASCII matches DS Style's 8x12 bitmap, advanced by six logical pixels.
 * UTF-8 filenames remain intact on disk; unsupported codepoints render as ?. */
static int nextchar(const char **s) {
    unsigned char c=(unsigned char)*(*s)++;if(c<128)return c;
    int n=c>=0xc2&&c<=0xdf?1:c>=0xe0&&c<=0xef?2:c>=0xf0&&c<=0xf4?3:0;
    if(!n)return '?';unsigned value=c&((1u<<(6-n))-1);
    for(int i=0;i<n;i++){unsigned char d=(unsigned char)**s;if((d&0xc0)!=0x80)return '?';(*s)++;value=(value<<6)|(d&63);}
    return value>0x10ffff||(value>=0xd800&&value<=0xdfff)?'?':(int)value;
}
 #include "locale.h"
static void text(int x,int y,const char*s,uint32_t c,int max) {
    if(dark_mode&&c==0)c=0xffffff;
    for(int n=0;*s&&n<max;n++,x+=6){int k=nextchar(&s);const unsigned char*g=font+'?'*12;
        if(k<128)g=font+k*12;else for(size_t i=0;i<sizeof(latin_codepoints)/sizeof(*latin_codepoints);i++)if(latin_codepoints[i]==(unsigned)k){g=latin_font+i*12;break;}
        for(int j=0;j<12;j++)for(int i=0;i<8;i++)if(g[j]&(128>>i))dot(x+i,y+j,c);
    }
}
static void loadassets(void){const char*names[]={"START","SD_LIST","SD_HORIZONTAL","SD_VERTICAL","SET"};char p[PATHLEN],n[80];for(int i=0;i<5;i++){snprintf(n,sizeof n,"assets/%s.bmp",names[i]);join(p,sizeof p,base,n);backgrounds[i]=loadpic(p);snprintf(n,sizeof n,"assets/dark/%s.bmp",names[i]);join(p,sizeof p,base,n);dark_backgrounds[i]=loadpic(p);}join(p,sizeof p,base,"assets/font.bin");FILE*f=fopen(p,"rb");if(f){fread(font,1,sizeof font,f);fclose(f);}}
static void loadlines(const char*n,char a[][PATHLEN],int *cnt,int max){char p[PATHLEN];statepath(p,n);FILE*f=fopen(p,"rb");if(!f)return;while(*cnt<max&&fgets(a[*cnt],PATHLEN,f)){a[*cnt][strcspn(a[*cnt],"\r\n")]=0;if(a[*cnt][0])(*cnt)++;}fclose(f);}
static void savelines(const char*n,char a[][PATHLEN],int cnt){char p[PATHLEN];statepath(p,n);size_t cap=(size_t)cnt*(PATHLEN+1)+1;char*b=calloc(cap,1);if(!b)return;size_t len=0;for(int i=0;i<cnt;i++)len+=(size_t)snprintf(b+len,cap-len,"%s\n",a[i]);if(!atomic_text(p,b))message("Could not save preferences");free(b);}
static void preferences(int save) {
    char p[PATHLEN],v[100];statepath(p,"preferences.txt");
    if(save){char more[80],mp[PATHLEN];statepath(mp,"interface.txt");snprintf(more,sizeof more,"%d %d %d %d %d\n",clock_12,full_system_names,gba_art,horizontal_fit,home_button);atomic_text(mp,more);char extra[160],ep[PATHLEN];statepath(ep,"style.txt");snprintf(extra,sizeof extra,"%d %d %d %d %d %d %d\n",art_border,2,0,rounded_corners,vertical_side,horizontal_side,list_folders);atomic_text(ep,extra);snprintf(v,sizeof v,"v2 %d %d %d %d %d %d\n",viewmode,colour,lcd_grid,clean_list,art_position,art_border);atomic_text(p,v);return;}
    FILE*f=fopen(p,"r");if(!f)return;if(!fgets(v,sizeof v,f)){fclose(f);return;}fclose(f);
    int a,b,c,d,e,g;
    if(sscanf(v,"v2 %d %d %d %d %d %d",&a,&b,&c,&d,&e,&g)==6){viewmode=a>=0&&a<4?a:1;colour=b>=0&&b<THEME_COUNT?b:0;lcd_grid=c==1;clean_list=d==1;art_position=e>=0&&e<3?e:2;art_border=g==1?2:0;}
    else if(sscanf(v,"%d %d",&a,&b)==2){const int old[]={2,12,4,15};viewmode=a>=0&&a<4?a:1;colour=b>=0&&b<4?old[b]:0;}
}
static int ui_sounds=1,startup_sound=1,system_icons=1,launching;
static void sound_preferences(int save){char p[PATHLEN],v[80];statepath(p,"experience.txt");if(save){snprintf(v,sizeof v,"%d %d %d %d %d\n",ui_sounds,startup_sound,system_icons,dark_mode,pixel_transparency);atomic_text(p,v);return;}FILE*f=fopen(p,"r");if(f){int a,b,c;if(fscanf(f,"%d %d %d",&a,&b,&c)==3){ui_sounds=a==1;startup_sound=b==1;system_icons=c==1;int d;if(fscanf(f,"%d",&d)==1)dark_mode=d==1;int pt;if(fscanf(f,"%d",&pt)==1)pixel_transparency=pt==1;}fclose(f);}}
static void extra_preferences(void){char path[PATHLEN];statepath(path,"interface.txt");FILE*more=fopen(path,"r");if(more){int a,b,c;if(fscanf(more,"%d %d %d",&a,&b,&c)==3){clock_12=a==1;full_system_names=b==1;gba_art=c==1;int fit;if(fscanf(more,"%d",&fit)==1)horizontal_fit=fit==1;int hb;if(fscanf(more,"%d",&hb)==1&&hb>=0&&hb<=2)home_button=hb;}fclose(more);}statepath(path,"style.txt");FILE*f=fopen(path,"r");if(f){int a,b,c,d,e,g;if(fscanf(f,"%d %d %d %d %d %d",&a,&b,&c,&d,&e,&g)==6){art_border=a>=0&&a<5?a:0;rounded_corners=d>=0&&d<3?d:0;vertical_side=e>=0&&e<3?e:0;horizontal_side=g>=0&&g<3?g:0;int lf;if(fscanf(f,"%d",&lf)==1)list_folders=lf>=0&&lf<=2?lf:1;}fclose(f);}join(path,sizeof path,base,"username.txt");f=fopen(path,"r");if(f){char v[80];if(fgets(v,sizeof v,f)){v[strcspn(v,"\r\n")]=0;if(*v)strcopy(username,v,sizeof username);}fclose(f);}}
static int favindex(const char*p){for(int i=0;i<favcount;i++)if(!strcmp(p,favourites[i]))return i;return -1;}
static int card_for(const char *path);
#include "stock_favourites.h"
#include "stock_history.h"
static void togglefav(void){if(!count||entries[choice].dir||entries[choice].app)return;const char*p=entries[choice].path;int n=favindex(p);int shared=stock_toggle(p,n>=0);if(shared<0){message("Stock favourites changed; reopen");return;}if(n>=0){memmove(favourites[n],favourites[n+1],(size_t)(favcount-n-1)*PATHLEN);favcount--;message("Removed from favourites");}else if(favcount<MAX_FAV){strcopy(favourites[favcount++],p,PATHLEN);message("Added to favourites");}else message("Favourite list is full");save_favourites();}
static void addrecent(const char*p){history_add(p);int n=-1;for(int i=0;i<recentcount;i++)if(!strcmp(p,recents[i]))n=i;if(n>=0){memmove(recents[n],recents[n+1],(size_t)(recentcount-n-1)*PATHLEN);recentcount--;}if(recentcount==50)recentcount--;memmove(recents[1],recents[0],(size_t)recentcount*PATHLEN);strcopy(recents[0],p,PATHLEN);recentcount++;savelines("recent.txt",recents,recentcount);}
static void addentry(const char*n,const char*p,int dir,int app){if(count>=MAX_ENTRIES)return;Entry*e=&entries[count++];strcopy(e->name,n,sizeof e->name);strcopy(e->path,p,sizeof e->path);e->dir=dir;e->app=app;e->stock_route=0;}
static int cmpentry(const void*a,const void*b){const Entry*x=a,*y=b;if(x->dir!=y->dir)return y->dir-x->dir;return strcasecmp(x->name,y->name);}
static int gamefile(const char*n){size_t len=strlen(n);if(len>7&&!strcasecmp(n+len-7,".p8.png"))return 1;const char*e=strrchr(n,'.');if(!e)return 0;const char*ext[]={".gba",".agb",".gb",".gbc",".nes",".sfc",".smc",".md",".gen",".sms",".gg",".pce",".chd",".cue",".m3u",".iso",".cso",".zip",".7z",".nds",".bin",".pbp",".img",".gdi",".a26",".a78",".ngp",".ngc",".ws",".wsc",".col",".int",".z64",".n64",".v64",".a52",".a78",".lnx",".vb",".vboy",".wsc",".ngc",".nib",".d64",".d71",".d81",".t64",".prg",".crt",".tap",".adf",".adz",".hdf",".ipf",".st",".stx",".msa",".dim",".dsk",".atr",".xfd",".xex",".car",".cas",".a8s",".rom",".32x",".sg",".sgx",".ccd",".m3u8",".p8",".p8.png",".pak",".wad",".dos",".scummvm",".sh",NULL};for(int i=0;ext[i];i++)if(!strcasecmp(e,ext[i]))return 1;return 0;}
static int under_root(const char *path,int i) {
    size_t n=strlen(roots[i]);
    return n && !strncmp(path,roots[i],n) && (path[n]==0 || path[n]=='/');
}
static int card_for(const char *path) {
    for(int i=0;i<2;i++)if(under_root(path,i))return i;
    return -1;
}
static int hidden_folder(const char*n){return n[0]=='.'||!strcasecmp(n,"Imgs")||!strcasecmp(n,"images")||!strcasecmp(n,"media")||!strcasecmp(n,"APPS")||!strcasecmp(n,"PortMaster")||!strcasecmp(n,"BIOS");}
typedef struct {char path[PATHLEN];int found;} RomPresence;
static RomPresence rom_presence[512];static int rom_presence_count;
static int has_roms(const char*path,int level);
static int scan_roms(const char*path,int level) {
    if(level>63)return 0;DIR*d=opendir(path);if(!d)return 0;struct dirent*de;int found=0;
    while(!found&&(de=readdir(d))){if(hidden_folder(de->d_name))continue;char full[PATHLEN];struct stat st;if(!join(full,sizeof full,path,de->d_name))continue;
#ifdef _WIN32
        if(stat(full,&st))continue;
#else
        if(lstat(full,&st)||S_ISLNK(st.st_mode))continue;
#endif
        if(S_ISDIR(st.st_mode))found=has_roms(full,level+1);else if(S_ISREG(st.st_mode)&&gamefile(de->d_name))found=1;
    }closedir(d);return found;
}
static int has_roms(const char*path,int level){for(int i=0;i<rom_presence_count;i++)if(!strcmp(path,rom_presence[i].path))return rom_presence[i].found;int found=scan_roms(path,level);if(rom_presence_count<512){RomPresence*p=&rom_presence[rom_presence_count++];strcopy(p->path,path,sizeof p->path);p->found=found;}return found;}
static int systems_view(void){return page==1&&section==0&&(!strcmp(here,roots[0])||(roots[1][0]&&!strcmp(here,roots[1])));}
static void home_state(int save){
 char path[PATHLEN];statepath(path,"home.txt");
 if(save){char data[PATHLEN+8];snprintf(data,sizeof data,"%d\n%s\n",home_favourites,home_favourites&&favcount?favourites[home_recent<favcount?home_recent:0]:"");atomic_text(path,data);return;}
 FILE*f=fopen(path,"r");if(!f)return;char line[PATHLEN];if(fgets(line,sizeof line,f))home_favourites=atoi(line)==1;
 if(home_favourites&&fgets(line,sizeof line,f)){line[strcspn(line,"\r\n")]=0;for(int i=0;i<favcount;i++)if(!strcmp(line,favourites[i])){home_recent=i;break;}}fclose(f);
}
static void set_launch_mode(int room){
 for(const char*q=launch_system;*q;q++)if(!((*q>='A'&&*q<='Z')||(*q>='a'&&*q<='z')||(*q>='0'&&*q<='9')||*q=='-'||*q=='_')){message("Unsupported system folder name");return;}
 char dir[PATHLEN],path[PATHLEN],file[300];statepath(dir,"launch-modes");MKDIR(dir);snprintf(file,sizeof file,"%s.txt",launch_system);join(path,sizeof path,dir,file);if(!atomic_text(path,room?"gameroom\n":"retroarch\n"))message("Could not save launch mode");
}
static void position_selection(int delta) {
    if(!count)return;choice+=delta;if(choice<0)choice=0;if(choice>=count)choice=count-1;
    if(delta>=10||delta<=-10){list_top+=delta;int max=count>10?count-10:0;if(list_top>max)list_top=max;if(list_top<0)list_top=0;if(choice<list_top)list_top=choice;if(choice>=list_top+10)list_top=choice-9;}
    else{if(choice<list_top)list_top=choice;if(choice>=list_top+10)list_top=choice-9;}
}
static void browse(const char*p) {
    /* Snapshot the requested path before changing any browser state. */
    char requested[PATHLEN];strcopy(requested,p,sizeof requested);
    strcopy(here,requested,sizeof here);count=0;choice=0;list_top=0;section=0;page=1;
    fprintf(stderr,"browse: path=%s\n",*here?here:"<card selection>");
    if(!*here) {
        int cards[2],nc=0;
        for(int i=0;i<2;i++)if(roots[i][0]&&has_roms(roots[i],0))cards[nc++]=i;
        if(nc==1){browse(roots[cards[0]]);return;}
        for(int j=0;j<nc;j++){int i=cards[j];addentry(i?"SD2 Games":"SD1 Games",roots[i],1,0);}
        section=4;
        if(apps_in_games)addentry("Apps","@apps",1,4);
        return;
    }
    DIR*d=opendir(here);
    if(!d){fprintf(stderr,"browse: opendir failed: %s\n",strerror(errno));message("Cannot read folder; see launch.log");return;}
    int scanned=0,dirs=0,games=0,ignored=0,read_error=0;
    for(;;) {
        errno=0;struct dirent*de=readdir(d);
        if(!de){read_error=errno;break;}
        scanned++;
        if(de->d_name[0]=='.'||strpbrk(de->d_name,"\r\n\t")){ignored++;continue;}
        char full[PATHLEN];if(!join(full,sizeof full,here,de->d_name)){ignored++;continue;}
        int dir=isdir(full);
        if(dir&&hidden_folder(de->d_name)){ignored++;continue;}
        if((dir&&has_roms(full,0))||(!dir&&gamefile(de->d_name))){addentry(de->d_name,full,dir,0);if(dir)dirs++;else games++;}
        else ignored++;
    }
    closedir(d);qsort(entries,(size_t)count,sizeof(Entry),cmpentry);
    if(apps_in_games&&systems_view())addentry("Apps","@apps",1,4);
    fprintf(stderr,"browse: SD%d scanned=%d folders=%d games=%d ignored=%d shown=%d read_error=%d\n",card_for(here)+1,scanned,dirs,games,ignored,count,read_error);
    if(read_error){fprintf(stderr,"browse: readdir failed: %s\n",strerror(read_error));message("Folder read error; see launch.log");}
    else if(count==MAX_ENTRIES)message("Folder limited to 4096 entries");
}
typedef struct {int page,section,choice,top,depth,setting,previous;char here[PATHLEN],stack[64][PATHLEN];int sel[64],tops[64];} ReturnState;
static ReturnState collection_return;static int collection_return_valid;
static void collection(int recent){
 shared_refresh();
 if(!(page==1&&(section==1||section==2))){ReturnState*v=&collection_return;v->page=page;v->section=section;v->choice=choice;v->top=list_top;v->depth=depth;v->setting=setting;v->previous=previous;strcopy(v->here,here,PATHLEN);memcpy(v->stack,stack,sizeof stack);memcpy(v->sel,stacksel,sizeof stacksel);memcpy(v->tops,stacktop,sizeof stacktop);collection_return_valid=1;}
 page=1;section=recent?2:1;count=choice=depth=list_top=0;here[0]=0;char(*a)[PATHLEN]=recent?recents:favourites;int n=recent?recentcount:favcount;for(int i=0;i<n;i++)if(exists(a[i])){addentry(filename(a[i]),a[i],0,0);entries[count-1].stock_route=shared_route(a[i],recent);}
}
static void apps(void){apps_return_systems=0;apps_return_settings=0;page=1;section=3;count=choice=depth=list_top=0;here[0]=0;if(preview||exists("/mnt/vendor/deep/retro/retroarch"))addentry("RetroArch","retroarch",0,1);if(preview||exists("/mnt/vendor/deep/ppsspp/PPSSPPSDL"))addentry("PPSSPP","ppsspp",0,1);addentry("Stock OS","stock",0,1);for(int i=0;i<2;i++){if(!roots[i][0])continue;char p[PATHLEN];join(p,sizeof p,roots[i],"APPS");DIR*d=opendir(p);if(!d)continue;struct dirent*de;while((de=readdir(d))){const char*ext=strrchr(de->d_name,'.');if(!ext||strcasecmp(ext,".sh")||strstr(de->d_name,"DS Style")||!strcasecmp(de->d_name,"DSStyle.sh")||strpbrk(de->d_name,"\r\n\t"))continue;char f[PATHLEN];if(join(f,sizeof f,p,de->d_name)&&!isdir(f))addentry(de->d_name,f,0,2);}closedir(d);}}
static void collection_back(void){
 if(!collection_return_valid){page=0;return;}ReturnState v=collection_return;collection_return_valid=0;
 if(v.page==1||(v.page==2&&v.previous==1)){if(v.section==3){int origin=apps_return_systems;apps();apps_return_systems=origin;}else browse(v.here);choice=v.choice<count?v.choice:count?count-1:0;list_top=v.top;depth=v.depth;memcpy(stack,v.stack,sizeof stack);memcpy(stacksel,v.sel,sizeof stacksel);memcpy(stacktop,v.tops,sizeof stacktop);}
 page=v.page;section=v.section;strcopy(here,v.here,PATHLEN);setting=v.setting;previous=v.previous;

}
typedef struct {int valid,choice,top,depth,section;char here[PATHLEN],stack[64][PATHLEN];int selected[64],tops[64];} TabState;
static TabState tabs[2];
static void remember_tab(void){int t=section==3?1:0;if(page!=1||(section!=0&&section!=3&&section!=4))return;TabState*v=&tabs[t];v->valid=1;v->choice=choice;v->top=list_top;v->depth=depth;v->section=section;strcopy(v->here,here,PATHLEN);memcpy(v->stack,stack,sizeof stack);memcpy(v->selected,stacksel,sizeof stacksel);memcpy(v->tops,stacktop,sizeof stacktop);}
static void restore_tab(int t){TabState*v=&tabs[t];if(t)apps();else browse(v->valid?v->here:"");if(v->valid){choice=v->choice<count?v->choice:count?count-1:0;list_top=v->top<=choice?v->top:choice;depth=v->depth;memcpy(stack,v->stack,sizeof stack);memcpy(stacksel,v->selected,sizeof stacksel);memcpy(stacktop,v->tops,sizeof stacktop);}}
#include "system_names.h"
#include "artwork.h"
#include "extra_state.h"
#include "controller_state.h"
#include "ui.h"
#include "extra_ui.h"

#ifndef _WIN32
static int fb=-1,input[16],lockfd=-1;static unsigned char*fbmem;static size_t fbsize;
static struct fb_var_screeninfo vi,original_vi;static struct fb_fix_screeninfo fi;
static volatile sig_atomic_t interrupted;
static int menu_held,menu_chord,volume_previous=-1;
#include "hardware_repeat.h"
static HardwareRepeat hardware_repeat;
static int held_direction,pan_committed,present_count;static uint64_t repeat_at;
static uint64_t millis(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000+t.tv_nsec/1000000;}
static void on_signal(int sig){(void)sig;interrupted=1;}
static void display_snapshot(const char *stage) {
    if(!getenv("DS_STYLE_DEBUG"))return;
    fprintf(stderr,"display snapshot: %s\n",stage);
    const char *paths[]={"/sys/class/graphics/fb0/blank","/sys/class/disp/disp/attr/sys",NULL};
    for(int i=0;paths[i];i++) {
        FILE *f=fopen(paths[i],"r");if(!f)continue;
        fprintf(stderr,"%s:\n",paths[i]);char buf[512];size_t n,total=0;
        while(total<16384 && (n=fread(buf,1,sizeof buf,f))>0){fwrite(buf,1,n,stderr);total+=n;}
        fputc('\n',stderr);fclose(f);
    }
}
static void close_display(void) {
    held_direction=0;hardware_repeat_clear(&hardware_repeat);
    if(fb>=0 && pan_committed) {
        original_vi.activate=FB_ACTIVATE_NOW;
        if(ioctl(fb,FBIOPAN_DISPLAY,&original_vi)<0)
            fprintf(stderr,"restore original fb offset: %s\n",strerror(errno));
    }
    pan_committed=0;
    if(fbmem){munmap(fbmem,fbsize);fbmem=NULL;}
    if(fb>=0){close(fb);fb=-1;}
    for(int i=0;i<16;i++){if(input[i]>=0)close(input[i]);input[i]=-1;}
}
static int display_cable(void){char b[64];const char*paths[]={"/sys/class/switch/hdmi/state","/sys/class/extcon/hdmi/state"};for(int i=0;i<2;i++){FILE*f=fopen(paths[i],"r");if(f){int ok=fgets(b,sizeof b,f)!=NULL;fclose(f);if(ok){if(b[0]=='1'||strstr(b,"HDMI=1"))return 1;if(b[0]=='0'||strstr(b,"HDMI=0"))return 0;}}}return -1;}
static void display_route(const char*mode){char path[PATHLEN];join(path,sizeof path,base,"bin/dsstyle-display");pid_t pid=fork();if(pid==0){execl(path,path,mode,(char*)NULL);_exit(127);}if(pid>0){int status;while(waitpid(pid,&status,0)<0&&errno==EINTR){}}}
static int display_connected=-2;static uint64_t display_check_at;
static void controller_discover(void);
static int open_display(void) {
    display_route("enter");display_connected=display_cable();
    for(int i=0;i<16;i++)input[i]=-1;
    present_count=0;pan_committed=0;menu_held=menu_chord=0;
    fprintf(stderr,"startup: opening framebuffer\n");
    fb=open("/dev/fb0",O_RDWR|O_CLOEXEC);
    if(fb<0){fprintf(stderr,"open fb0: %s\n",strerror(errno));return 0;}
    if(ioctl(fb,FBIOGET_VSCREENINFO,&vi)||ioctl(fb,FBIOGET_FSCREENINFO,&fi)) {
        fprintf(stderr,"get fb geometry: %s\n",strerror(errno));goto fail;
    }
    original_vi=vi;fbsize=fi.smem_len;
    fprintf(stderr,"fb: visible=%ux%u virtual=%ux%u offset=%u,%u bpp=%u stride=%u bytes=%zu visual=%u type=%u ypanstep=%u\n",
        vi.xres,vi.yres,vi.xres_virtual,vi.yres_virtual,vi.xoffset,vi.yoffset,
        vi.bits_per_pixel,fi.line_length,fbsize,fi.visual,fi.type,fi.ypanstep);
    fprintf(stderr,"fb channels R=%u/%u G=%u/%u B=%u/%u A=%u/%u\n",
        vi.red.offset,vi.red.length,vi.green.offset,vi.green.length,
        vi.blue.offset,vi.blue.length,vi.transp.offset,vi.transp.length);
    if(vi.xres<W||vi.yres<H||vi.xres>4096||vi.yres>4096||
       fi.visual!=FB_VISUAL_TRUECOLOR||fi.type!=FB_TYPE_PACKED_PIXELS||
       !fb_window_fits(vi.xres,vi.yres,vi.xres_virtual,vi.yres_virtual,
          vi.xoffset,vi.yoffset,vi.bits_per_pixel,fi.line_length,fbsize)) {
        fprintf(stderr,"unsupported or unsafe framebuffer layout\n");goto fail;
    }
    struct fb_bitfield fields[]={vi.red,vi.green,vi.blue,vi.transp};
    for(int i=0;i<4;i++)if(fields[i].length>8||fields[i].offset+fields[i].length>vi.bits_per_pixel||fields[i].msb_right){fprintf(stderr,"unsupported channel layout\n");goto fail;}
    fbmem=mmap(NULL,fbsize,PROT_READ|PROT_WRITE,MAP_SHARED,fb,0);
    if(fbmem==MAP_FAILED){fbmem=NULL;fprintf(stderr,"mmap fb0: %s\n",strerror(errno));goto fail;}
    fprintf(stderr,"startup: framebuffer mapped; opening inputs\n");
    int got=0;FILE*vf=fopen("/sys/class/power_supply/axp2202-battery/openbor_volume","r");if(vf){int v;if(fscanf(vf,"%d",&v)==1)volume_previous=volume_saved(base,v);fclose(vf);}
    controller_discover();for(int i=0;i<16;i++)if(input[i]>=0)got++;
    if(!got){fprintf(stderr,"no readable input devices\n");goto fail;}
    display_snapshot("before first presentation");
    return 1;
fail:close_display();return 0;
}
static uint32_t channel(unsigned c,struct fb_bitfield f){if(!f.length)return 0;return ((c*((1u<<f.length)-1)+127)/255)<<f.offset;}
static void present(void) {
    uint32_t next_y=fb_next_y(vi.yres,vi.yres_virtual,vi.yoffset,fi.ypanstep);
    if(!fb_window_fits(vi.xres,vi.yres,vi.xres_virtual,vi.yres_virtual,
                      vi.xoffset,next_y,vi.bits_per_pixel,fi.line_length,fbsize))next_y=vi.yoffset;
    int scale=(int)vi.xres/W;if((int)vi.yres/H<scale)scale=(int)vi.yres/H;
    int ox=((int)vi.xres-W*scale)/2,oy=((int)vi.yres-H*scale)/2;
    if(!present_count)fprintf(stderr,"startup: writing first frame at yoffset=%u\n",next_y);
    static unsigned char *row;static size_t capacity;size_t bytes=(size_t)vi.xres*(vi.bits_per_pixel/8);
    if(capacity<bytes){unsigned char*n=realloc(row,bytes);if(!n){running=0;exitcode=6;return;}row=n;capacity=bytes;}
    int native32=vi.bits_per_pixel==32&&vi.red.offset==16&&vi.green.offset==8&&vi.blue.offset==0&&vi.red.length==8&&vi.green.length==8&&vi.blue.length==8;
    uint32_t alpha=channel(255,vi.transp);
    for(unsigned y=0;y<vi.yres;y++){
      if(native32&&scale==3&&!ox&&!oy&&vi.xres==720&&vi.yres==480){
        output_row3((uint32_t*)row,y,alpha);
      }else for(unsigned x=0;x<vi.xres;x++) {
        int lx=((int)x-ox)/scale,ly=((int)y-oy)/scale;
        uint32_t rgb=((int)x<ox||(int)y<oy||lx>=W||ly>=H)?0:output_pixel((int)x-ox,(int)y-oy,scale);
        uint32_t c=native32?rgb|alpha:channel((rgb>>16)&255,vi.red)|channel((rgb>>8)&255,vi.green)|channel(rgb&255,vi.blue)|alpha;
        unsigned char*q=row+x*(vi.bits_per_pixel/8);if(vi.bits_per_pixel==16){uint16_t v=(uint16_t)c;memcpy(q,&v,2);}else memcpy(q,&c,4);
      }
      static unsigned char*shadow;static size_t shadow_size;static unsigned slot_y[2];static int valid[2];
      size_t page_bytes=bytes*vi.yres;if(shadow_size!=page_bytes*2){free(shadow);shadow=calloc(2,page_bytes);shadow_size=shadow?page_bytes*2:0;valid[0]=valid[1]=0;}
      if(!present_count)valid[0]=valid[1]=0;
      int slot=next_y?1:0;if(slot_y[slot]!=next_y){valid[slot]=0;slot_y[slot]=next_y;}
      unsigned char*dest=fbmem+(y+next_y)*fi.line_length+vi.xoffset*(vi.bits_per_pixel/8);
      unsigned char*old=shadow?shadow+slot*page_bytes+y*bytes:NULL;
      if(!old||!valid[slot]){memcpy(dest,row,bytes);if(old)memcpy(old,row,bytes);}
      else if(memcmp(old,row,bytes)){size_t lo=0,hi=bytes;while(lo<hi&&old[lo]==row[lo])lo++;while(hi>lo&&old[hi-1]==row[hi-1])hi--;memcpy(dest+lo,row+lo,hi-lo);memcpy(old+lo,row+lo,hi-lo);}
      if(launching)fb_launch_row(fbmem,fbsize,row,vi.xres,vi.yres,vi.xres_virtual,vi.yres_virtual,vi.xoffset,y,vi.bits_per_pixel,fi.line_length);
      if(y+1==vi.yres)valid[slot]=1;
    }
    /* An mmap write alone does not ask the BSP display engine to show this page. */
    if(!present_count)fprintf(stderr,"startup: flushing frame, then FBIOPAN_DISPLAY\n");
    if(!present_count && msync(fbmem,fbsize,MS_SYNC)<0)
        fprintf(stderr,"msync fb0: %s (continuing to pan)\n",strerror(errno));
    struct fb_var_screeninfo request=vi;request.yoffset=next_y;request.activate=FB_ACTIVATE_NOW;
    if(ioctl(fb,FBIOPAN_DISPLAY,&request)<0) {
        fprintf(stderr,"FBIOPAN_DISPLAY failed: %s\n",strerror(errno));
        running=0;exitcode=6;return;
    }
    vi.yoffset=next_y;pan_committed=1;present_count++;
    if(present_count==1){fprintf(stderr,"startup: first presentation accepted by driver (visibility requires user check)\n");display_snapshot("after first presentation");}
}
static void hardware_adjust(int brightness,int up){
 char exe[PATHLEN],previous[16];join(exe,sizeof exe,base,brightness?"bin/dsstyle-brightness":"bin/dsstyle-volume");snprintf(previous,sizeof previous,"%d",volume_previous);
 int pipefd[2];if(pipe(pipefd))return;pid_t pid=fork();if(pid==0){dup2(pipefd[1],STDOUT_FILENO);close(pipefd[0]);close(pipefd[1]);if(brightness)execl(exe,exe,up?"up":"down",(char*)NULL);else execl(exe,exe,up?"up":"down",previous,base,(char*)NULL);_exit(127);}
 close(pipefd[1]);char output[32]={0};struct pollfd ready={pipefd[0],POLLIN,0};int available=poll(&ready,1,2000);if(available<=0&&pid>0)kill(pid,SIGKILL);ssize_t n=available>0?read(pipefd[0],output,sizeof output-1):-1;close(pipefd[0]);int status=0;if(pid>0)while(waitpid(pid,&status,0)<0&&errno==EINTR){}
 if(pid>0&&n>0&&WIFEXITED(status)&&WEXITSTATUS(status)==0){int level=atoi(output);hardware_kind=brightness?2:1;hardware_level=level*100/(brightness?6:20);hardware_until=millis()+1400;hardware_dirty=1;if(!brightness)volume_previous=level;}else{hardware_kind=brightness?2:1;hardware_level=-1;hardware_until=millis()+1400;hardware_dirty=1;fprintf(stderr,"Hardware adjustment failed: %s status=%d\n",exe,status);}
}
#include "controller_native.h"
static int rawkey(unsigned code){switch(code){case 103:return UP;case 108:return DOWN;case 105:return LEFT;case 106:return RIGHT;case 304:return ACCEPT;case 305:return BACK;case 307:return VIEW;case 306:return FAV;case 310:return RECENT;case 311:return START;case 312:return MENU;case 308:return PGUP;case 309:return PGDN;case 314:return HISTORY;case 315:return FAVOURITES;default:return 0;}}
static int pollkey(void) {
    if(capture_action>=0&&millis()>=capture_until){capture_action=-1;extra_revision++;hardware_dirty=1;}
    for (int i=0; i<16; i++) {
        if (input[i]<0) continue;
        struct input_event e;
        while (read(input[i],&e,sizeof e)==sizeof e) {
            int k=0;int profile=input_profile[i];
            if(e.type==EV_KEY||e.type==EV_ABS)last_activity=millis();
            if(profile&&e.type==EV_ABS&&(e.code==ABS_Z||e.code==ABS_RZ)){struct input_absinfo axis;if(ioctl(input[i],EVIOCGABS(e.code),&axis)<0)continue;int t=e.code==ABS_RZ,pressed=e.value>((long long)axis.minimum+axis.maximum)/2;if(pressed==input_trigger[i][t])continue;input_trigger[i][t]=pressed;e.type=EV_KEY;e.code=t?313:312;e.value=pressed;}

            if(capture_action>=0&&e.type==EV_ABS&&(e.code==ABS_HAT0X||e.code==ABS_X)&&controller_axis(input[i],&e)<0){capture_action=-1;hardware_dirty=1;continue;}
            if(capture_action>=0&&e.type==EV_KEY&&e.value==1){
                if(e.code==105||e.code==546||e.code==KEY_ESC){capture_action=-1;}
                else if(profile==capture_profile&&bindable(e.code)){controller_bind(profile,capture_action,e.code);capture_action=-1;}
                hardware_dirty=1;continue;
            }
            if(capture_action>=0)continue;
            if(e.type==EV_KEY&&e.value==1&&!(page==2&&settings_category==6))controller_selected=profile;
            if(e.type==EV_KEY||e.type==EV_ABS)last_activity=millis();
            if (e.type==EV_KEY) {
                if(profile==0&&e.code==312){if(e.value==1){menu_held=1;menu_chord=0;}else if(e.value==0){menu_held=0;if(!menu_chord)return controller_key(0,312);}continue;}
                if(e.code==KEY_VOLUMEUP||e.code==KEY_VOLUMEDOWN){if(menu_held)menu_chord=1;if(hardware_repeat_event(&hardware_repeat,e.code,e.value,menu_held,millis()))hardware_adjust(hardware_repeat.brightness,e.code==KEY_VOLUMEUP);continue;}
                k=controller_key(profile,e.code);
                if (k>=UP && k<=RIGHT) {
                    if (e.value==0) {
                        if (held_direction==k) held_direction=0;
                        continue;
                    }
                    if (e.value==2) continue;
                } else if (e.value!=1) continue;
            }
            if (e.type==EV_ABS && (e.code==ABS_HAT0X || e.code==ABS_HAT0Y || e.code==ABS_X || e.code==ABS_Y)) {
                e.value=controller_axis(input[i],&e);
                if(e.code==ABS_X)e.code=ABS_HAT0X;if(e.code==ABS_Y)e.code=ABS_HAT0Y;
                if (!e.value) {
                    if ((e.code==ABS_HAT0X && (held_direction==LEFT || held_direction==RIGHT)) ||
                        (e.code==ABS_HAT0Y && (held_direction==UP || held_direction==DOWN)))
                        held_direction=0;
                    continue;
                }
                k=e.code==ABS_HAT0X ? (e.value<0?LEFT:RIGHT) : (e.value<0?UP:DOWN);
            }
            if (k) {
                if (k>=UP && k<=RIGHT) { held_direction=k; repeat_at=millis()+350; }
                return k;
            }
        }
    }
    for(int i=0;i<16;i++)if(input[i]>=0){struct pollfd p={input[i],0,0};if(poll(&p,1,0)>0&&(p.revents&(POLLHUP|POLLERR|POLLNVAL))){close(input[i]);input[i]=-1;held_direction=0;hardware_repeat_clear(&hardware_repeat);}}
    if(hardware_repeat_due(&hardware_repeat,millis())) {
        last_activity=millis();
        hardware_adjust(hardware_repeat.brightness,hardware_repeat.key==KEY_VOLUMEUP);
        hardware_repeat_reschedule(&hardware_repeat,millis());
    }
    if(!home_animating()&&millis()-input_scan_at>1000)controller_discover();
    if (held_direction && millis()>=repeat_at) { repeat_at=millis()+100; return held_direction; }
    return 0;
}
#endif

static int sound_launched;
#ifndef _WIN32
static pid_t sound_pid=-1;
static int sound_fd=-1;
#endif
static void sound_stop(void){
#ifndef _WIN32
 if(sound_fd>=0){close(sound_fd);sound_fd=-1;}
 if(sound_pid>0){uint64_t deadline=millis()+15000;int status;
  while(waitpid(sound_pid,&status,WNOHANG)==0){if(millis()>=deadline){kill(sound_pid,SIGTERM);while(waitpid(sound_pid,&status,0)<0&&errno==EINTR){}break;}struct timespec t={0,5000000};nanosleep(&t,NULL);}
  sound_pid=-1;
 }
#endif
}
static void sound_play(const char *name,int wait){
 if(!strcmp(name,"startup")?!startup_sound:!ui_sounds)return;if(preview){fprintf(stderr,"sound: %s\n",name);return;}
#ifndef _WIN32
 const char *names[]={"accept","back","launch","menu","move","startup","tab"};int id=-1;
 for(int i=0;i<7;i++)if(!strcmp(name,names[i]))id=i;if(id<0)return;
 if(sound_pid>0&&waitpid(sound_pid,NULL,WNOHANG)!=0){close(sound_fd);sound_fd=-1;sound_pid=-1;}
 if(sound_pid<0){char exe[PATHLEN];join(exe,sizeof exe,base,"bin/dsstyle-audio");int fds[2];if(pipe(fds))return;
  signal(SIGPIPE,SIG_IGN);sound_pid=fork();
  if(sound_pid==0){dup2(fds[0],STDIN_FILENO);close(fds[0]);close(fds[1]);execl(exe,exe,base,(char*)NULL);_exit(127);}
  close(fds[0]);if(sound_pid<0){close(fds[1]);return;}sound_fd=fds[1];fcntl(sound_fd,F_SETFL,O_NONBLOCK);fcntl(sound_fd,F_SETFD,FD_CLOEXEC);
 }
 unsigned char command=(unsigned char)id;if(write(sound_fd,&command,1)!=1)fprintf(stderr,"audio command: %s\n",strerror(errno));
 if(wait)sound_stop();
#else
 (void)wait;
#endif
}
static void stock_session(const char*mode){
#ifndef _WIN32
 if(preview)return;char script[PATHLEN];join(script,sizeof script,base,"scripts/stock-session.sh");pid_t pid=fork();if(pid==0){execl("/bin/sh","sh",script,mode,(char*)NULL);_exit(127);}if(pid>0)while(waitpid(pid,NULL,0)<0&&errno==EINTR){}
#else
 (void)mode;
#endif
}
#include "mixer_guard.h"
static int runhelper(const char*helper,const char*mode,const char*path){
 int game=!strcmp(helper,"scripts/launch.sh"),sleeping=!strcmp(helper,"scripts/sleep-stock.sh");
 if(!strcmp(helper,"scripts/power.sh"))fprintf(stderr,"power: starting %s handoff\n",mode);
#ifndef _WIN32
 held_direction=0;hardware_repeat_clear(&hardware_repeat);menu_held=menu_chord=0;
#endif
 if(game&&!startup_direct){launching=1;draw();
#ifndef _WIN32
 if(!preview)present();
#endif
 sound_launched=1;sound_play("launch",1);}else sound_stop();
 if(preview){launching=0;message("Preview: stock launch disabled");return -1;}
#ifndef _WIN32
 if(!sleeping){if(game&&!strcmp(mode,"game"))history_add(path);if(game)ui_mixer_save();stock_session("leave");display_route("leave");if(game)pan_committed=0;close_display();}
 char script[PATHLEN];join(script,sizeof script,base,helper);pid_t pid=fork();if(pid==0){if(launch_route_override)execl("/bin/sh","sh",script,mode,path,launch_route_override==1?"stock-room":"stock-ra",(char*)NULL);else execl("/bin/sh","sh",script,mode,path,(char*)NULL);_exit(127);}int status=0;
 if(pid<0)status=127<<8;else while(waitpid(pid,&status,0)<0){if(errno!=EINTR){status=127<<8;break;}}
 launching=0;if(interrupted){running=0;return -1;}
 if(!sleeping){if(game)ui_mixer_restore();shared_refresh();stock_session("enter");if(!open_display()){running=0;exitcode=1;return -1;}}
 else {for(int i=0;i<16;i++)if(input[i]>=0){struct input_event ev;while(read(input[i],&ev,sizeof ev)==sizeof ev){}}}
 held_direction=0;hardware_repeat_clear(&hardware_repeat);menu_held=menu_chord=0;last_activity=millis();int rc=WIFEXITED(status)?WEXITSTATUS(status):128;
 if(rc){char msg[90];snprintf(msg,sizeof msg,"Launch failed (%d); see launch.log",rc);message(msg);}return rc;
#else
 (void)helper;(void)mode;(void)path;(void)sleeping;return -1;
#endif
}

#ifndef _WIN32
#include "stock_hardware.h"
#include "stock_settings.h"
static int stock_idle_ms(void){
 unsigned char b[140];if(!attr_read("/mnt/data/dmenu/dmenu_attr.ini",b)||!stock_sleep_known()||!stock_idle_known())return 0;
 unsigned index=b[28]|b[29]<<8|b[30]<<16|(unsigned)b[31]<<24;
 static const int times[]={60000,120000,300000,600000};return index<4?times[index]:0;
}
static void stock_sleep_poll(void){
 static uint64_t next;static int closed_before,idle_ms=-1;
 uint64_t now=millis();if(now<next)return;next=now+250;
 if(idle_ms<0){idle_ms=stock_idle_ms();fprintf(stderr,"sleep: stock idle timeout %d ms; hall 0=closed\n",idle_ms);}
 int hall=-1;FILE*f=fopen("/sys/class/power_supply/axp2202-battery/hallkey","r");if(f){if(fscanf(f,"%d",&hall)!=1)hall=-1;fclose(f);}
 int lid=hall==0&&!closed_before;closed_before=hall==0;
 if(!lid&&(!idle_ms||now-last_activity<(uint64_t)idle_ms))return;
 if(!exists("/mnt/vendor/ctrl/pwr_new.sh")){last_activity=now;return;}
 fprintf(stderr,"sleep: invoking stock pwr_new.sh auto (%s)\n",lid?"lid":"idle");
 if(snake_active)snake_high_save();runhelper("scripts/sleep-stock.sh","auto","");last_activity=millis();held_direction=0;hardware_repeat_clear(&hardware_repeat);menu_held=menu_chord=0;hardware_dirty=1;idle_ms=-1;
}
#endif
static void launch_selected(void){if(!count)return;Entry e=entries[choice];if(e.app==4){remember_tab();apps();apps_return_systems=1;return;}if(e.app==3){collection(!strcmp(e.path,"recents"));return;}if(e.dir){if(depth>=63){message("Folder nesting limit reached");return;}strcopy(stack[depth],here,PATHLEN);stacktop[depth]=list_top;stacksel[depth++]=choice;browse(e.path);return;}if(e.app==1&&!strcmp(e.path,"stock")){if(preview)message("Preview: return to stock menu");else{exitcode=42;running=0;}return;}char mode[32];strcopy(mode,e.app==1?e.path:e.app==2?"app":"game",sizeof mode);launch_route_override=e.stock_route;int rc=runhelper("scripts/launch.sh",mode,e.path);if(!e.app&&rc==0)addrecent(e.path);launch_route_override=0;}
/* START escape is physical and independent of user remapping. */
static void startup_destination(int escape,int quick){
 int dest=boot_destination;char path[PATHLEN]={0};
 if(escape){dest=home_enabled?0:1;quick=0;}
 if(quick){int n=home_favourites?favcount:recentcount;char(*a)[PATHLEN]=home_favourites?favourites:recents;if(n)strcopy(path,a[home_recent<n?home_recent:0],sizeof path);}
 else if(dest==4&&recentcount)strcopy(path,recents[0],sizeof path);
 if(dest==0&&!home_enabled)dest=1;
 page=0;
 if(*path&&exists(path)){startup_direct=1;launch_route_override=shared_route(path,quick?!home_favourites:1);if(runhelper("scripts/launch.sh","game",path)==0)addrecent(path);launch_route_override=0;startup_direct=0;}else if(dest==4)message("No recent game");
 if(dest==1||dest==4)browse("");else if(dest==2||dest==3){if(!home_enabled)browse("");collection(dest==2);}
}
static void stock_menu(void){if(preview)message("Preview: return to stock menu");else{exitcode=42;running=0;}}
static void action_impl(int k) {
    if(!k)return;if(extra_action(k))return;if(launch_mode_popup){if(k==ACCEPT||k==BACK){set_launch_mode(k==BACK);launch_mode_popup=0;}return;}if(notice[0]){if(k==ACCEPT||k==BACK)notice[0]=0;return;}
    if(power_confirm){int intent=power_confirm;if(k==BACK){power_confirm=0;return;}if(k==ACCEPT){power_confirm=0;if(intent==3)togglefav();else {sound_launched=1;sound_play("accept",1);if(runhelper("scripts/power.sh",intent==1?"reboot":"shutdown","")==0){running=0;exitcode=0;}}}return;}
    if(k==HISTORY||k==FAVOURITES){remember_tab();collection(k!=FAVOURITES);return;}
    if((k==PGUP||k==PGDN)&&page!=0){int current=page==2?2:section==3?1:0;remember_tab();int dest=(current+(k==PGUP?2:1))%3;if(dest==2){previous=page;page=2;}else restore_tab(dest);return;}
    if(page==0){
        int old_home=homechoice;
        if(k==UP)homechoice=homechoice>=3?1:0;
        if(k==DOWN){if(homechoice==0)homechoice=1;else if(homechoice<3)homechoice=3;}
        if(k==LEFT){if(homechoice>0&&homechoice<3)homechoice=1;else if(homechoice>3)homechoice--;}
        if(k==RIGHT){if(homechoice>0&&homechoice<3)homechoice=2;else if(homechoice>=3&&homechoice<5)homechoice++;}
        if(homechoice!=old_home){start_home_animation(old_home);}
        int hn=home_favourites?favcount:recentcount;if(home_recent>=hn)home_recent=0;char(*ha)[PATHLEN]=home_favourites?favourites:recents;
        if(hn&&((k==PGUP||k==PGDN)||(homechoice==0&&(k==LEFT||k==RIGHT))))home_recent=(home_recent+hn+((k==PGUP||k==LEFT)?-1:1))%hn;
        if(k==START)return;
        if(k==ACCEPT){if(homechoice==0){if(hn){char path[PATHLEN];strcopy(path,ha[home_recent],sizeof path);launch_route_override=shared_route(path,!home_favourites);if(runhelper("scripts/launch.sh","game",path)==0){addrecent(path);if(!home_favourites)home_recent=0;}launch_route_override=0;}else collection(!home_favourites);}else if(homechoice==1){restore_tab(0);}else if(homechoice==2){if(home_button)collection(home_button==2);else apps();}else if(homechoice==3){previous=0;page=2;setting=0;settings_category=-1;settings_top=0;}else power_confirm=homechoice==4?1:2;}
        if(k==RECENT){home_favourites=!home_favourites;home_recent=0;}if(k==VIEW)collection(1);if(k==FAV)collection(0);
        if(k==MENU){previous=page;page=2;setting=0;settings_category=-1;settings_top=0;}
    }else if(page==2){
        if(help_page){if(k==BACK)help_page=0;else if(k==LEFT||k==RIGHT){help_page=help_page==1?2:1;extra_revision++;}return;}
        if(settings_category<0){if(k==UP&&category_choice>0)category_choice--;if(k==DOWN&&category_choice<8)category_choice++;if(k==BACK){page=previous;if(page==1&&(systems_view()||section==4)){int selected=choice,top=list_top;browse(here);choice=selected<count?selected:count?count-1:0;list_top=top<=choice?top:choice;}return;}if(k==ACCEPT){if(category_choice==7){help_page=1;return;}if(category_choice==8){about_page=0;extra_revision++;return;}settings_category=category_choice;setting=settings_top=0;}return;}
        int total;settings_items(&total);if(k==UP&&setting>0)setting--;if(k==DOWN&&setting+1<total)setting++;
        if(k==BACK){settings_category=-1;settings_top=0;return;}
        int id=setting_value();if(k==ACCEPT||((k==LEFT||k==RIGHT)&&(id<=SET_LCD||id>=SET_UISOUND))){
            int delta=k==LEFT?-1:1;
            if(id>=SET_BIND_FIRST){if(k==ACCEPT){capture_action=id-SET_BIND_FIRST;capture_profile=controller_selected;capture_until=ui_millis()+CONTROL_CAPTURE_MS;extra_revision++;}return;}
            switch(id){
            case SET_BOOT:boot_destination=(boot_destination+5+delta)%5;boot_warning_at=boot_destination==4?ui_millis()+2000:0;extra_config(1);break;
            case SET_HOME_ENABLED:home_enabled=!home_enabled;extra_config(1);break;
            case SET_SOURCE:home_favourites=!home_favourites;home_recent=0;home_state(1);break;
            case SET_QUICK:quick_key=(quick_key+8+delta)%8;extra_config(1);break;
            case SET_APPS_GAMES:apps_in_games=!apps_in_games;extra_config(1);break;
            case SET_LANGUAGE:language=(language+8+delta)%8;extra_config(1);break;
            case SET_CONTROLLER:controller_selected=(controller_selected+controller_count+delta)%controller_count;break;
            case SET_RESET_CONTROLS:controller_defaults(controller_selected);controller_save();message("Controls reset");break;
            case SET_CLOCK:clock_12=!clock_12;break;case SET_NAMES:full_system_names=!full_system_names;break;case SET_GBAART:gba_art=!gba_art;break;case SET_HFIT:horizontal_fit=!horizontal_fit;break;
            case SET_HOME:home_button=(home_button+3+delta)%3;break;case SET_FOLDERS:list_folders=(list_folders+3+delta)%3;break;case SET_CLEAN:clean_list=!clean_list;break;case SET_COLOUR:colour=(colour+THEME_COUNT+delta)%THEME_COUNT;break;case SET_VIEW:viewmode=(viewmode+4+delta)%4;break;case SET_BORDER:art_border=(art_border+5+delta)%5;break;case SET_ROUND:rounded_corners=(rounded_corners+3+delta)%3;break;case SET_VSIDE:vertical_side=(vertical_side+3+delta)%3;break;case SET_HSIDE:horizontal_side=(horizontal_side+3+delta)%3;break;case SET_ARTPOS:art_position=(art_position+3+delta)%3;break;case SET_LCD:lcd_grid=!lcd_grid;break;
            case SET_PT:pixel_transparency=!pixel_transparency;sound_preferences(1);break;case SET_DARK:dark_mode=!dark_mode;sound_preferences(1);break;case SET_UISOUND:ui_sounds=!ui_sounds;sound_preferences(1);if(!ui_sounds)sound_stop();break;case SET_STARTSOUND:startup_sound=!startup_sound;sound_preferences(1);break;case SET_SYSICON:system_icons=!system_icons;sound_preferences(1);break;case SET_APPS:apps();apps_return_settings=1;break;case SET_STOCK:stock_menu();break;case SET_REBOOT:power_confirm=1;break;case SET_SHUTDOWN:power_confirm=2;break;
            case SET_AUTOBOOT:{int enabled=autoboot_enabled();if(runhelper("scripts/boot-manager.sh",enabled?"disable":"enable","")==0)message(enabled?"Autoboot disabled":"Autoboot enabled");else message("Autoboot failed; see boot-manager.log");break;}

            }
            if(id<=SET_LCD)preferences(1);extra_revision++;
        }
    }else{
        if(k==MENU){remember_tab();previous=page;page=2;setting=0;settings_category=-1;settings_top=0;return;}
        if(k==BACK){if(!section&&depth){depth--;int old=stacksel[depth];browse(stack[depth]);choice=old<count?old:0;list_top=stacktop[depth];}else if(section==1||section==2)collection_back();else if(section==3&&apps_return_systems){apps_return_systems=0;restore_tab(0);}else if(section==3&&apps_return_settings){page=2;apps_return_settings=0;}else{remember_tab();if(home_enabled)page=0;else browse("");}}
        else if(k==ACCEPT)launch_selected();
        else if(k==FAV)togglefav();else if(k==RECENT){viewmode=(viewmode+1)%4;preferences(1);}
        else if(k==START){if(systems_view()&&count&&entries[choice].dir&&!entries[choice].app){strcopy(launch_system,entries[choice].name,sizeof launch_system);launch_mode_popup=1;}}
        else if(k==PGUP){if(section==1||section==2)collection(section==1);else{previous=page;page=2;setting=0;settings_category=-1;settings_top=0;}}
        else if(k==PGDN){if(section==1||section==2)collection(section==1);else apps();}
        else if(count){int horizontal=effective_view()==2;int delta=k==UP?(horizontal?-10:-1):k==DOWN?(horizontal?10:1):k==LEFT?(horizontal?-1:-10):k==RIGHT?(horizontal?1:10):0;position_selection(delta);}
    }
}
static void action(int k){
 int old_extra_revision=extra_revision;int old_home_source=home_favourites,old_home_game=home_recent;
 int old_page=page,old_choice=choice,old_view=viewmode;
 int before[]={pixel_transparency,dark_mode,ui_sounds,startup_sound,system_icons,launch_mode_popup,home_button,page,choice,homechoice,section,viewmode,colour,lcd_grid,clean_list,art_position,art_border,2,0,rounded_corners,vertical_side,horizontal_side,list_folders,clock_12,full_system_names,gba_art,horizontal_fit,notice[0],settings_category,category_choice,setting,help_page,power_confirm,favcount,home_recent,home_favourites,running};
 char oldhere[PATHLEN];strcopy(oldhere,here,sizeof oldhere);int old_snake_active=snake_active,old_about_page=about_page;sound_launched=0;action_impl(k);if(!home_enabled&&page==0)browse("");
 if(old_home_source!=home_favourites||old_home_game!=home_recent)home_state(1);
 if(old_page!=page||old_choice!=choice||old_view!=viewmode||strcmp(oldhere,here))marquee_at=ui_millis();
 int after[]={pixel_transparency,dark_mode,ui_sounds,startup_sound,system_icons,launch_mode_popup,home_button,page,choice,homechoice,section,viewmode,colour,lcd_grid,clean_list,art_position,art_border,2,0,rounded_corners,vertical_side,horizontal_side,list_folders,clock_12,full_system_names,gba_art,horizontal_fit,notice[0],settings_category,category_choice,setting,help_page,power_confirm,favcount,home_recent,home_favourites,running};
 if(!sound_launched&&(old_extra_revision!=extra_revision||memcmp(before,after,sizeof before)||strcmp(oldhere,here)))sound_play(k==START&&snake_active&&!old_snake_active?"menu":k==BACK?"back":old_about_page>=0&&!old_snake_active&&(k==ACCEPT||k==PGUP||k==PGDN||k==LEFT||k==RIGHT)?"move":k==PGUP||k==PGDN||k==HISTORY||k==FAVOURITES?"tab":k==RECENT||k==VIEW||k==FAV?"menu":k>=UP&&k<=RIGHT?"move":"accept",0);
}
static int bmp(const char*p){FILE*f=fopen(p,"wb");if(!f)return 0;unsigned char h[54]={0};int width=W*3,height=H*3,size=54+width*height*3;h[0]='B';h[1]='M';memcpy(h+2,&size,4);h[10]=54;h[14]=40;memcpy(h+18,&width,4);memcpy(h+22,&height,4);h[26]=1;h[28]=24;fwrite(h,1,54,f);for(int y=height-1;y>=0;y--)for(int x=0;x<width;x++){uint32_t c=output_pixel(x,y,3);unsigned char b[3]={(unsigned char)c,(unsigned char)(c>>8),(unsigned char)(c>>16)};fwrite(b,1,3,f);}return fclose(f)==0;}
static void demo_entries(void){count=0;page=1;const char*n[]={"Aster Trail.gba","Garden Quest.gba","Pocket Rally.gba","Skybound.gba","Tiny Workshop.gba"};for(int i=0;i<5;i++){char p[PATHLEN];join(p,sizeof p,roots[0],n[i]);addentry(n[i],p,0,0);}choice=1;}
#ifdef _WIN32
static LRESULT CALLBACK windowproc(HWND h,UINT m,WPARAM w,LPARAM l){(void)l;if(m==WM_DESTROY){PostQuitMessage(0);return 0;}if(m==WM_KEYDOWN){int k=0;switch(w){case VK_UP:k=UP;break;case VK_DOWN:k=DOWN;break;case VK_LEFT:k=LEFT;break;case VK_RIGHT:k=RIGHT;break;case VK_RETURN:case 'A':k=ACCEPT;break;case VK_ESCAPE:case 'B':k=BACK;break;case 'X':k=VIEW;break;case 'Y':k=FAV;break;case VK_TAB:k=RECENT;break;case VK_SPACE:k=START;break;case 'M':k=MENU;break;case '1':k=HISTORY;break;case '2':k=FAVOURITES;break;case VK_PRIOR:k=PGUP;break;case VK_NEXT:k=PGDN;break;}action(k);InvalidateRect(h,NULL,FALSE);return 0;}if(m==WM_TIMER){extra_tick();InvalidateRect(h,NULL,FALSE);return 0;}if(m==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);draw();BITMAPINFO bi={0};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;RECT r;GetClientRect(h,&r);int s=r.right/W;if(r.bottom/H<s)s=r.bottom/H;if(s<1)s=1;if(s>32)s=32;bi.bmiHeader.biWidth=W*s;bi.bmiHeader.biHeight=-H*s;uint32_t*out=malloc((size_t)W*H*s*s*4);PatBlt(dc,0,0,r.right,r.bottom,BLACKNESS);if(out){for(int y=0;y<H*s;y++)for(int x=0;x<W*s;x++)out[y*W*s+x]=output_pixel(x,y,s);StretchDIBits(dc,(r.right-W*s)/2,(r.bottom-H*s)/2,W*s,H*s,0,0,W*s,H*s,out,&bi,DIB_RGB_COLORS,SRCCOPY);free(out);}EndPaint(h,&ps);return 0;}return DefWindowProc(h,m,w,l);}
#endif
int main(int argc,char**argv){setvbuf(stderr,NULL,_IONBF,0);const char*render=NULL,*screen="home",*events=NULL;int selftest=0,lcd_override=-1;
#ifdef _WIN32
preview=1;
#endif
for(int i=1;i<argc;i++){if(!strcmp(argv[i],"--base")&&i+1<argc)strcopy(base,argv[++i],sizeof base);else if(!strcmp(argv[i],"--roms")&&i+1<argc){strcopy(roots[0],argv[++i],PATHLEN);roots[1][0]=0;}else if(!strcmp(argv[i],"--roms2")&&i+1<argc){strcopy(roots[1],argv[++i],PATHLEN);}else if(!strcmp(argv[i],"--display-test")){display_test_seconds=12;}else if(!strcmp(argv[i],"--splash"))startup_splash=1;else if(!strcmp(argv[i],"--preview"))preview=1;else if(!strcmp(argv[i],"--lcd"))lcd_override=1;else if(!strcmp(argv[i],"--demo"))demo=1;else if(!strcmp(argv[i],"--render")&&i+1<argc){render=argv[++i];preview=1;}else if(!strcmp(argv[i],"--screen")&&i+1<argc)screen=argv[++i];else if(!strcmp(argv[i],"--events")&&i+1<argc)events=argv[++i];else if(!strcmp(argv[i],"--self-test"))selftest=1;else{fprintf(stderr,"Unknown/missing argument: %s\n",argv[i]);return 2;}}
fprintf(stderr,"DS Style v1.0 startup: base=%s display_test=%d\n",base,display_test_seconds);loadassets();ui_load();fprintf(stderr,"startup: assets loaded\n");if(!backgrounds[0].p||!font['A'*12+2]){fprintf(stderr,"DS Style assets missing or invalid under %s\n",base);return 3;}loadlines("favourites.txt",favourites,&favcount,MAX_FAV);loadlines("recent.txt",recents,&recentcount,50);stock_import();history_import();preferences(0);extra_preferences();sound_preferences(0);home_state(0);extra_config(0);controller_load();if(lcd_override>=0)lcd_grid=lcd_override;if(strcmp(screen,"home")){if(!strcmp(screen,"settings"))page=2;else if(!strcmp(screen,"apps"))apps();else{if(demo)demo_entries();else browse(roots[0]);viewmode=!strcmp(screen,"list")?0:!strcmp(screen,"art")?1:!strcmp(screen,"horizontal")?2:3;}}if(preview&&startup_splash)hardware_until=0;if(preview&&getenv("DS_STYLE_PREVIEW_BOOT"))startup_destination(getenv("DS_STYLE_PREVIEW_ESCAPE")!=NULL,getenv("DS_STYLE_PREVIEW_QUICK")!=NULL);if(events)for(const char*s=events;*s;s++){int k=*s=='u'?UP:*s=='d'?DOWN:*s=='l'?LEFT:*s=='r'?RIGHT:*s=='a'?ACCEPT:*s=='b'?BACK:*s=='x'?VIEW:*s=='y'?FAV:*s=='m'?MENU:*s=='s'?RECENT:*s=='t'?START:*s=='q'?PGUP:*s=='e'?PGDN:*s=='1'?HISTORY:*s=='2'?FAVOURITES:0;action(k);}
if(selftest){if(preview&&getenv("DS_STYLE_BENCHMARK")){draw();output_benchmark();}if(!output_equivalent()){fprintf(stderr,"Optimised scanline mismatch\n");return 1;}if(!fb_layout_selftest()){fprintf(stderr,"framebuffer geometry failure\n");return 1;}if(!gamefile("demo.GBA")||gamefile("readme.txt")){fprintf(stderr,"extension failure\n");return 1;}memset(pixels,0,sizeof pixels);rect(-2,-2,4,4,0x123456);if(pixels[0]!=0x123456||pixels[2]!=0){fprintf(stderr,"clip failure\n");return 1;}printf("self-test passed; page=%d entries=%d selected=%d view=%d top=%d home=%d quick=%d effective=%d\n",page,count,choice,viewmode,list_top,homechoice,home_recent,effective_view());return 0;}
if(!strcmp(screen,"volume")){page=0;hardware_kind=1;hardware_level=60;hardware_until=ui_millis()+5000;}if(!strcmp(screen,"brightness")){page=0;hardware_kind=2;hardware_level=57;hardware_until=ui_millis()+5000;}if(render&&preview)preview_frame_time=ui_millis();home_animation_at=0;if(preview&&getenv("DS_STYLE_PREVIEW_ANIM_MS"))home_animation_at=ui_millis()-(uint64_t)atoi(getenv("DS_STYLE_PREVIEW_ANIM_MS"));if(preview&&getenv("DS_STYLE_PREVIEW_LAUNCHING"))launching=1;if(preview&&getenv("DS_STYLE_PREVIEW_SCROLL_MS"))marquee_at=ui_millis()-(uint64_t)atoi(getenv("DS_STYLE_PREVIEW_SCROLL_MS"));draw();if(render){printf("render page=%d entries=%d selected=%d view=%d top=%d home=%d quick=%d effective=%d\n",page,count,choice,viewmode,list_top,homechoice,home_recent,effective_view());return bmp(render)?0:1;}
#ifdef _WIN32
WNDCLASSA wc={0};wc.lpfnWndProc=windowproc;wc.hInstance=GetModuleHandle(NULL);wc.lpszClassName="DSStyleV1";wc.hCursor=LoadCursor(NULL,IDC_ARROW);RegisterClassA(&wc);RECT r={0,0,720,480};AdjustWindowRect(&r,WS_OVERLAPPEDWINDOW,FALSE);HWND win=CreateWindowA(wc.lpszClassName,"DS Style RG SP v2 - desktop preview",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,NULL,NULL,wc.hInstance,NULL);SetTimer(win,1,16,NULL);ShowWindow(win,SW_SHOW);MSG msg;while(GetMessage(&msg,NULL,0,0)>0){TranslateMessage(&msg);DispatchMessage(&msg);}return 0;
#else
 fprintf(stderr,"startup: acquiring instance lock\n");char lp[PATHLEN];statepath(lp,"frontend.lock");lockfd=open(lp,O_CREAT|O_RDWR|O_CLOEXEC,0600);if(lockfd<0||flock(lockfd,LOCK_EX|LOCK_NB)){fprintf(stderr,"Another DS Style instance is running\n");return 4;}signal(SIGTERM,on_signal);signal(SIGINT,on_signal);signal(SIGUSR1,on_signal);if(!open_display()){fprintf(stderr,"Cannot open supported framebuffer/input; use stock fallback\n");return 5;}fprintf(stderr,"startup: entering render loop\n");
 int escape_start=startup_held(311)!=0,quick_start=quick_key&&startup_held(quick_codes[quick_key])==1;
 if(startup_splash&&!( !escape_start&&(quick_start||(boot_destination==4&&recentcount)))){char path[PATHLEN];join(path,sizeof path,base,"assets/SPLASH.png");Pic splash=loadpic(path);if(splash.p){blit(splash,0,0,W,H);present();sound_play("startup",1);stbi_image_free(splash.p);}}
 if(boot_destination==4||quick_start){uint64_t until=millis()+600;while(millis()<until&&!interrupted){if(startup_held(311)!=0)escape_start=1;struct timespec ts={0,10000000};nanosleep(&ts,NULL);}}
 if(!display_test_seconds)startup_destination(escape_start,quick_start);
 /* Consume startup button releases so the escape cannot also trigger a menu action. */
 for(int i=0;i<16;i++)if(input[i]>=0){struct input_event e;while(read(input[i],&e,sizeof e)==sizeof e){}}
 uint64_t started=millis();last_activity=started;int ticks=0,last_test_second=-1;unsigned measured_frames=0;uint64_t render_total=0,render_max=0,draw_max=0,present_max=0;
 while(running&&!interrupted) {
    uint64_t frame_started=millis();extra_tick();
    int k=pollkey();
    if(display_test_seconds) {
        int remaining=display_test_seconds-(int)((millis()-started)/1000);
        if(remaining<=0 || k==BACK){fprintf(stderr,"display test finished\n");break;}
        if(k)fprintf(stderr,"display test input=%d\n",k);
        if(remaining!=last_test_second){snprintf(notice,sizeof notice,"Display test: %ds   B returns",remaining);last_test_second=remaining;ticks=0;}
    } else {if(k){last_activity=millis();action(k);}stock_sleep_poll();}
    if(!running)break;
    if(!home_animating()&&frame_started-display_check_at>1000){display_check_at=frame_started;int cable=display_cable();struct fb_var_screeninfo check;int changed=fb>=0&&ioctl(fb,FBIOGET_VSCREENINFO,&check)==0&&(check.xres!=vi.xres||check.yres!=vi.yres||check.bits_per_pixel!=vi.bits_per_pixel);if((cable>=0&&cable!=display_connected)||changed){pan_committed=0;close_display();if(!open_display()){running=0;exitcode=5;break;}hardware_dirty=1;}}
    if(hardware_until&&millis()>=hardware_until){hardware_until=0;hardware_dirty=1;}if(k||hardware_dirty||(page==0&&home_animation_at)||(marquee_needed&&millis()-marquee_last>=16)||ticks++%60==0){uint64_t render_at=millis();draw();uint64_t drawn_at=millis();present();uint64_t shown_at=millis();if(drawn_at-render_at>draw_max)draw_max=drawn_at-render_at;if(shown_at-drawn_at>present_max)present_max=shown_at-drawn_at;uint64_t cost=shown_at-render_at;render_total+=cost;if(cost>render_max)render_max=cost;measured_frames++;hardware_dirty=0;}
    uint64_t elapsed=millis()-frame_started;if(elapsed<16){struct timespec ts={0,(long)(16-elapsed)*1000000};nanosleep(&ts,NULL);}
 }
 fprintf(stderr,"shutdown: releasing display; rc=%d signal=%d\n",exitcode,(int)interrupted);
 if(snake_active)snake_high_save();fprintf(stderr,"frame phases: max_draw_ms=%llu max_present_ms=%llu\n",(unsigned long long)draw_max,(unsigned long long)present_max);fprintf(stderr,"render timing: frames=%u mean_ms=%.2f max_ms=%llu (CPU render+present, not panel FPS)\n",measured_frames,measured_frames?(double)render_total/measured_frames:0.0,(unsigned long long)render_max);sound_stop();display_route("leave");close_display();close(lockfd);return exitcode;
#endif
}
