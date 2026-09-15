/* Native RG SP renderer using imported DS Style v7.4 geometry and glyphs.
 * Hardware-independent title algorithms are in original_text.h (Apache-2.0).
 */
enum { SET_CLOCK,SET_FOLDERS,SET_NAMES,SET_CLEAN,SET_COLOUR,SET_VIEW,SET_GBAART,SET_BORDER,SET_ROUND,SET_VSIDE,SET_HSIDE,SET_HFIT,SET_ARTPOS,SET_HOME,SET_LCD,SET_STOCK,SET_AUTOBOOT,SET_REBOOT,SET_SHUTDOWN,SET_APPS,SET_UISOUND,SET_STARTSOUND,SET_SYSICON,SET_DARK,SET_PT,SET_BOOT,SET_HOME_ENABLED,SET_SOURCE,SET_QUICK,SET_LANGUAGE,SET_CONTROLLER,SET_RESET_CONTROLS,SET_APPS_GAMES,SET_BIND_FIRST,SET_COUNT=SET_BIND_FIRST+CONTROL_ACTIONS };
static int settings_category=-1,category_choice;
static const int browser_items[]={SET_VIEW,SET_FOLDERS,SET_CLEAN,SET_NAMES,SET_SYSICON,SET_APPS_GAMES};
static const int artwork_items[]={SET_ARTPOS,SET_HSIDE,SET_HFIT,SET_VSIDE,SET_GBAART,SET_BORDER,SET_ROUND};
static const int appearance_items[]={SET_COLOUR,SET_DARK,SET_LCD,SET_PT,SET_CLOCK,SET_LANGUAGE};
static const int startup_items[]={SET_BOOT,SET_HOME_ENABLED,SET_SOURCE,SET_QUICK,SET_HOME,SET_AUTOBOOT};
static const int sound_items[]={SET_UISOUND,SET_STARTSOUND};
static const int hardware_items[]={SET_APPS,SET_STOCK,SET_REBOOT,SET_SHUTDOWN};
static const int controls_items[]={SET_CONTROLLER,SET_RESET_CONTROLS,SET_BIND_FIRST,SET_BIND_FIRST+1,SET_BIND_FIRST+2,SET_BIND_FIRST+3,SET_BIND_FIRST+4,SET_BIND_FIRST+5,SET_BIND_FIRST+6,SET_BIND_FIRST+7,SET_BIND_FIRST+8,SET_BIND_FIRST+9,SET_BIND_FIRST+10};
static const char *category_names[]={"Browsing","Artwork","Appearance","Startup","Sound","System","Controls","Help","About"};
static const int *settings_items(int *n){const int*a=hardware_items;*n=sizeof hardware_items/sizeof*a;
#define GROUP(c,g) case c:a=g;*n=sizeof g/sizeof*a;break
 switch(settings_category){GROUP(0,browser_items);GROUP(1,artwork_items);GROUP(2,appearance_items);GROUP(3,startup_items);GROUP(4,sound_items);GROUP(6,controls_items);default:break;}
#undef GROUP
 return a;
}
static int setting_value(void){int n;const int*a=settings_items(&n);if(setting<0||setting>=n)setting=0;return a[setting];}
static int autoboot_enabled(void){
    char a[PATHLEN],b[PATHLEN];const char*root=getenv("DS_STYLE_TEST_ROOT");if(!root)root="";
    snprintf(a,sizeof a,"%s/mnt/mmc/dmenu.bin",root);snprintf(b,sizeof b,"%s/mnt/mmc/.dsstyle-v1-boot/installed",root);
    FILE*f=fopen(a,"rb"),*g=fopen(b,"rb");if(!f||!g){if(f)fclose(f);if(g)fclose(g);return 0;}
    int x,y,same=1;do{x=fgetc(f);y=fgetc(g);if(x!=y){same=0;break;}}while(x!=EOF);if(ferror(f)||ferror(g))same=0;fclose(f);fclose(g);return same;
}
static Pic reset_button,power_button;
static Pic ui_background(int index){return dark_mode&&dark_backgrounds[index].p?dark_backgrounds[index]:backgrounds[index];}
/* Grey pairs sampled from the original START settings button. Recolour at
 * draw time, so the user's edited pixel geometry and PNG files stay intact. */
static void power_icon(Pic pic,int x){
 if(!dark_mode){blit(pic,x,143,14,14);return;}
 const unsigned char light[]={0,73,121,162,195,211,251};
 const unsigned char dark[]={123,97,134,93,60,44,4};
 for(int y=0;y<14;y++)for(int i=0;i<14;i++){
  if(!pic.p)continue;unsigned char*q=pic.p+4*((y*pic.h/14)*pic.w+i*pic.w/14);if(q[3]<128)continue;
  unsigned c=(q[0]<<16)|(q[1]<<8)|q[2];if(q[0]==q[1]&&q[1]==q[2])for(int k=0;k<7;k++)if(q[0]==light[k]){c=dark[k]*0x010101;break;}dot(x+i,y+143,c);
 }
}
static Pic help_bg,missing_art, popup_bg, iconsets[THEME_COUNT][2];
static Pic platform_icons[23];
static const char *platform_icon_names[]={"GB","GBC","FC","GG","SMS","PCE","WS","MSX","TXT","other","folder","gba","PS","NDS","PSP","SFC","N64","MD","DREAMCAST","SATURN","disc","cart","apps"};
static const char *border_names[]={"Off","Accent","Black","Grey","White"};
static const char *round_names[]={"Off","Full","No Start"};
static const char *hside_names[]={"Centre","Top","Bottom"};
static const char *vside_names[]={"Centre","Left","Right"};
static const char *art_positions[]={"Top","Centre","Bottom"};
static int pt_valid,pt_building;
static uint32_t pt_frame[720*480];
static void pt_prepare(void);
static int ui_scale_cache;
static unsigned char lcd_table[32*32][256];
static uint64_t home_animation_at;
static uint64_t preview_frame_time; /* Freeze explicitly rendered snapshots only. */
static float home_from_x[4],home_from_y[4],home_last_x[4],home_last_y[4];
static uint64_t ui_millis(void) {
 if(preview_frame_time)return preview_frame_time;
#ifdef _WIN32
    return GetTickCount64();
#else
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000+t.tv_nsec/1000000;
#endif
}
static int home_animating(void){return home_animation_at&&ui_millis()-home_animation_at<200;}

static int glyph_count(const char *s){int n=0;while(*s){nextchar(&s);n++;}return n;}
static void centered(int x,int y,int width,const char *s,uint32_t colour) {
    int tx=x+(width-glyph_count(s)*6)/2;if(tx<x)tx=x;
    text(tx,y,s,colour,width/6);
}
static void ui_load(void) {
    char path[PATHLEN],rel[128];
    for(int i=0;i<THEME_COUNT;i++) {
        snprintf(rel,sizeof rel,"assets/themes/%s/%s.bmp",theme_ids[i],theme_ids[i]);
        join(path,sizeof path,base,rel);bars[i]=loadpic(path);
        for(int j=0;j<2;j++) {
            snprintf(rel,sizeof rel,"assets/themes/%s/icon_%s.bmp",theme_ids[i],j?"gba":"folder");
            join(path,sizeof path,base,rel);iconsets[i][j]=loadpic(path);
        }
    }
    for(int i=0;i<23;i++) {
        snprintf(rel,sizeof rel,"assets/icons/icon_%s.png",platform_icon_names[i]);join(path,sizeof path,base,rel);
        platform_icons[i]=loadpic(path);
        if(!platform_icons[i].p){snprintf(rel,sizeof rel,"assets/icons/icon_%s.bmp",platform_icon_names[i]);join(path,sizeof path,base,rel);platform_icons[i]=loadpic(path);}
    }
    join(path,sizeof path,base,"assets/RESET.png");reset_button=loadpic(path);
    join(path,sizeof path,base,"assets/POWER.png");power_button=loadpic(path);
    join(path,sizeof path,base,"assets/NOTFOUND.png");missing_art=loadpic(path);
    join(path,sizeof path,base,"assets/HELP.bmp");help_bg=loadpic(path);
    join(path,sizeof path,base,"assets/MENU.bmp");popup_bg=loadpic(path);
    join(path,sizeof path,base,"assets/font-latin.bin");FILE*f=fopen(path,"rb");if(f){fread(latin_font,1,sizeof latin_font,f);fclose(f);}
}
static Pic entry_icon(const Entry *e) {
    if(e->app&&platform_icons[22].p)return platform_icons[22];
    if(e->dir&&(!systems_view()||!system_icons))return iconsets[colour][0];
    char parent[PATHLEN];strcopy(parent,e->path,sizeof parent);if(!e->dir){char*q=strrchr(parent,'/');if(q)*q=0;}
    for(int depth=0;depth<20&&*parent;depth++){
        const char*n=filename(parent);if(!strcasecmp(n,"GBA"))return iconsets[colour][1];
        for(int i=0;i<23;i++)if(!strcasecmp(n,platform_icon_names[i])&&platform_icons[i].p)return platform_icons[i];
        const char*aliases[]={"PSX","PLAYSTATION","SNES","GENESIS","MEGADRIVE","DC","NES","FAMICOM","GAMEGEAR","MASTERSYSTEM","PCENGINE","WONDERSWAN"};
        const int ids[]={12,12,15,17,17,18,2,2,3,4,5,6};
        for(int i=0;i<12;i++)if(!strcasecmp(n,aliases[i]))return platform_icons[ids[i]];
        const char*optical[]={"3DO","MDCD","SEGACD","MEGACD","PCECD","NEOCD","AMIGACD32","PS2","GC","WII","SATURN",NULL};
        for(int i=0;optical[i];i++)if(!strcasecmp(n,optical[i]))return platform_icons[20];
        char*q=strrchr(parent,'/');if(!q)break;*q=0;
    }
    const char*ext=strrchr(e->name,'.');int i=-1;
    if(ext){const char*exts[]={".gb",".gbc",".nes",".gg",".sms",".pce",".ws",".msx",".txt"};for(int k=0;k<9;k++)if(!strcasecmp(ext,exts[k]))i=k;
        if(!strcasecmp(ext,".gba")||!strcasecmp(ext,".agb"))return iconsets[colour][1];
        if(!strcasecmp(ext,".chd")||!strcasecmp(ext,".cue")||!strcasecmp(ext,".iso")||!strcasecmp(ext,".gdi")||!strcasecmp(ext,".pbp"))i=20;
    }
    return platform_icons[i>=0?i:21];
}
static void battery_icon(void){
 static uint64_t checked;static int level=-1,charging;
 if(demo)level=75;
#ifndef _WIN32
 else if(!home_animating()&&(!checked||ui_millis()-checked>10000)){checked=ui_millis();FILE*f=fopen("/sys/class/power_supply/axp2202-battery/capacity","r");int v;if(f){if(fscanf(f,"%d",&v)==1&&v>=0&&v<=100)level=v;fclose(f);}f=fopen("/sys/class/power_supply/axp2202-battery/status","r");char status[32];if(f){if(fgets(status,sizeof status,f))charging=!strncmp(status,"Charging",8);fclose(f);}}
#else
 (void)checked;
#endif
 border(171,6,12,7,0xffffff);rect(183,8,1,3,0xffffff);
 if(level>=0)rect(173,8,(level*8+99)/100,3,0xffffff);else dot(176,9,0xffffff);
 if(charging){dot(178,6,0);dot(177,7,0);dot(176,8,0);dot(177,9,0);dot(176,10,0);dot(175,11,0);}
}
/* Link state only; Stock OS owns all Wi-Fi configuration. */
static void wifi_icon(void){
 int connected=0;const char*test=getenv("DS_STYLE_PREVIEW_WIFI");if(preview&&test)connected=atoi(test)==1;
#ifndef _WIN32
 static uint64_t checked;static int cached;if(!home_animating()&&(ui_millis()-checked>2000||!checked)){checked=ui_millis();cached=0;FILE*f=fopen("/proc/net/wireless","r");if(f){char line[256];while(fgets(line,sizeof line,f)){char name[32];if(sscanf(line," %31[^:]:",name)!=1)continue;char path[128];snprintf(path,sizeof path,"/sys/class/net/%s/carrier",name);FILE*g=fopen(path,"r");int active=0;if(g){fscanf(g,"%d",&active);fclose(g);}if(active==1)cached=1;}fclose(f);}}connected=cached;
#endif
 if(!connected)return;
 /* Stepped DS-style radio arcs; one logical pixel equals three panel pixels. */
 rect(155,5,9,1,0xffffff);dot(154,6,0xffffff);dot(164,6,0xffffff);dot(153,7,0xffffff);dot(165,7,0xffffff);
 rect(157,8,5,1,0xffffff);dot(156,9,0xffffff);dot(162,9,0xffffff);rect(159,11,1,2,0xffffff);
}
static void ui_titlebar(const char *title) {
    blit(bars[colour],0,0,240,19);
    text(3,3,username,0xffffff,11);
    if(title&&*title){char short_title[64];strcopy(short_title,title,sizeof short_title);if(strlen(short_title)>15)short_title[15]=0;centered(73,3,94,short_title,0xffffff);}
    time_t now=time(NULL);struct tm *tm=localtime(&now);char value[16]="00:00:00";
    if(tm){if(clock_12)snprintf(value,sizeof value,"%2d:%02d %s",tm->tm_hour%12?tm->tm_hour%12:12,tm->tm_min,tm->tm_hour>=12?"PM":"AM");else strftime(value,sizeof value,"%H:%M:%S",tm);}
    if(demo)strcopy(value,clock_12?"12:34 PM":"12:34:56",sizeof value);
    if(page==1&&section!=3){snprintf(value,sizeof value,"%d/%d",count?choice+1:0,count);int x=235-(int)strlen(value)*6;if(x<184)x=184;text(x,3,value,0xffffff,9);}else text(189,3,value,0xffffff,8);if(page==0||page==2){battery_icon();wifi_icon();}
}
static void art_size(Pic pic,int slotw,int sloth,int role,int *w,int *h){
 *h=sloth;*w=pic.h?pic.w*sloth/pic.h:slotw;if(*w<1)*w=1;
 int limit=slotw;
 if(role!=1&&viewmode==2&&!horizontal_fit)limit=238;
 if(*w>limit){*w=limit;*h=pic.w?pic.h*limit/pic.w:sloth;}if(*h<1)*h=1;
 if(role!=1&&viewmode==2&&(*w&1)&&*w<limit)(*w)++;
}
static void ui_art(const Entry *e,int x,int y,int w,int h,int role) {
 Pic pic=getart(e);int home=role==1,side=role==2;
 if(!pic.p&&!e->dir&&!e->app)pic=missing_art;
 if(!pic.p){int zoom=home||side?1:2;icon(entry_icon(e),x+(w-16*zoom)/2,y+(h-14*zoom)/2,16*zoom,14*zoom);return;}
 int dw,dh;art_size(pic,w,h,role,&dw,&dh);
 if(side&&viewmode==3){x=vertical_side==1?7:vertical_side==2?91-dw:49-dw/2;}else if(role==3)x+=w-dw;else x+=(w-dw)/2;
 y+=(h-dh)/2;w=dw;h=dh;
 int rounded=rounded_corners&&(rounded_corners==1||!home),owner=0;
 if(!gba_art&&native_art_count<8){owner=++native_art_count;native_art[owner-1]=(NativeArt){pic,x,y,w,h};}
 for(int j=0;j<h;j++)for(int i=0;i<w;i++){
  int edge=j<h-1-j?j:h-1-j,inset=rounded?(edge==0?5:edge==1?3:edge==2?2:edge<5?1:0):0;if(i<inset||i>=w-inset)continue;
  unsigned char*q=pic.p+4*((j*pic.h/h)*pic.w+i*pic.w/w);if(q[3]>127){dot(x+i,y+j,(q[0]<<16)|(q[1]<<8)|q[2]);if(owner&&x+i>=0&&x+i<W&&y+j>=0&&y+j<H)art_owner[(y+j)*W+x+i]=(unsigned char)owner;}
 }
 if(art_border&&!home){uint32_t c=art_border==1?(side?0x848484:theme_accent[colour]):art_border==2?0:art_border==3?0x848484:0xffffff;
  if(!rounded)border(x-1,y-1,w+2,h+2,c);
  else for(int j=-1;j<=h;j++)for(int i=-1;i<=w;i++){
   int edge=j<h-1-j?j:h-1-j,inset=edge==0?5:edge==1?3:edge==2?2:edge<5?1:0;if(j>=0&&j<h&&i>=inset&&i<w-inset)continue;
   int adjacent=0;for(int yy=j-1;yy<=j+1;yy++)if(yy>=0&&yy<h){int ee=yy<h-1-yy?yy:h-1-yy,ii=ee==0?5:ee==1?3:ee==2?2:ee<5?1:0;if(i+1>=ii&&i-1<w-ii)adjacent=1;}if(adjacent)dot(x+i,y+j,c);
  }
 }
}
static void heart(int x,int y) {
    const unsigned char rows[]={0x6c,0xfe,0xfe,0x7c,0x38,0x10};
    for(int j=0;j<6;j++)for(int i=0;i<8;i++)if(rows[j]&(128>>i))dot(x+i,y+j,dark_mode?0xffffff:0);
}
static void home_corner_position(int item,int c,int *px,int *py) {
    const int boxes[][4]={{25,43,190,47},{25,92,95,45},{120,92,95,45},{111,145,18,11},{182,143,14,14},{201,143,14,14}};
        int x=boxes[item][0]+(c%2?boxes[item][2]-1:0);
        int y=boxes[item][1]+(c>=2?boxes[item][3]-1:0);
        /* The original renderer's START.bmp alignment corrections. */
        if(item==0&&c>=2)y--;
        if(item==1){if(c<2)y--;if(c%2)x--;}
        if(item==2){if(c<2)y--;if(c%2==0)x++;}
        if(item>=3){x+=c%2?1:-1;if(c<2)y-=2;}
        else{if(c%2)x++;if(c>=2)y++;}
        *px=x;*py=y;
}
/* 200 ms eased glide at physical pixel precision, continuously retargetable. */
static void start_home_animation(int old){
 int active=home_animating();for(int c=0;c<4;c++){int x,y;home_corner_position(old,c,&x,&y);home_from_x[c]=active?home_last_x[c]:x;home_from_y[c]=active?home_last_y[c]:y;}home_animation_at=ui_millis();
}
static void motion_rect(float x,float y,int w,int h){
 if(motion_count>=8)return;motion_rects[motion_count++]=(MotionRect){x,y,w,h};
 for(int yy=(int)floorf(y);yy<(int)ceilf(y+h);yy++)for(int xx=(int)floorf(x);xx<(int)ceilf(x+w);xx++)if(xx>=0&&xx<W&&yy>=0&&yy<H)motion_owner[yy*W+xx]=1;
}
static void home_corners(void) {
 int moving=home_animating();float t=moving?(float)(ui_millis()-home_animation_at)/200.0f:1.0f;if(t>1)t=1;t=t*t*(3.0f-2.0f*t);
 if(!moving)home_animation_at=0;
 if(homechoice==3&&!moving){rect(109,142,23,3,theme_accent[colour]);rect(109,155,23,3,theme_accent[colour]);rect(109,142,3,16,theme_accent[colour]);rect(129,142,3,16,theme_accent[colour]);return;}
 for(int c=0;c<4;c++) {
  int sx=c%2?-1:1,sy=c>=2?-1:1,tx,ty;home_corner_position(homechoice,c,&tx,&ty);
  float x=moving?home_from_x[c]+(tx-home_from_x[c])*t:tx,y=moving?home_from_y[c]+(ty-home_from_y[c])*t:ty;
  home_last_x[c]=x;home_last_y[c]=y;
  if(moving){motion_rect(sx<0?x-8:x,sy<0?y-2:y,9,3);motion_rect(sx<0?x-2:x,sy<0?y-8:y,3,9);}
  else {rect(sx<0?(int)x-8:(int)x,sy<0?(int)y-2:(int)y,9,3,theme_accent[colour]);rect(sx<0?(int)x-2:(int)x,sy<0?(int)y-8:(int)y,3,9,theme_accent[colour]);}
 }
}
static void ui_home(void) {
    blit(ui_background(0),0,0,W,H);ui_titlebar("");
    Entry last={0};char title[128],lines[3][32];
    int hn=home_favourites?favcount:recentcount;char(*ha)[PATHLEN]=home_favourites?favourites:recents;
    if(hn){if(home_recent>=hn)home_recent=0;strcopy(last.path,ha[home_recent],sizeof last.path);strcopy(last.name,filename(last.path),sizeof last.name);Launcher_CleanTitle(last.name,title,sizeof title);}
    else strcopy(title,tr(home_favourites?"No favourite game":"No recent game"),sizeof title);
    ui_art(&last,LAUNCHER_START_LAST_THUMB_X,LAUNCHER_START_LAST_THUMB_Y,56,37,1);
    int n=Launcher_SplitStartTitle(title,lines);
    int y=n==3?49:66-(n*11)/2-(n==1?1:0);
    for(int i=0;i<n;i++)centered(LAUNCHER_START_LAST_TEXT_X,y+i*11,LAUNCHER_START_LAST_TEXT_W,lines[i],0);
    centered(LAUNCHER_START_SD_TEXT_X,LAUNCHER_START_SD_TEXT_Y,LAUNCHER_START_SD_TEXT_W,tr("Games"),0);
    centered(LAUNCHER_START_NOR_TEXT_X,LAUNCHER_START_NOR_TEXT_Y,LAUNCHER_START_NOR_TEXT_W,tr(home_button==1?"Favs":home_button==2?"Recents":"Apps"),0);
    power_icon(reset_button,182);power_icon(power_button,201);
    home_corners();
}
static void ui_settings(void) {
    static const char *labels[]={"Clock format","List folders","System names","Clean list","Colour","View","GBA res. art","Art border","Round corners","Vert. side","Horiz. side","Horiz. fit","List artwork","Home button","LCD grid","Stock OS","Autoboot","Reboot","Shutdown","Apps","UI sounds","Startup sound","System icons","Dark mode","Pixel transp.","Boot to","Home screen","Home source","Quick start","Language","Controller","Reset controls","Apps in Games"};
    blit(ui_background(help_page?1:4),0,0,W,H);ui_titlebar(tr(help_page?"Help":settings_category<0?"Settings":category_names[settings_category]));
    if(help_page){if(help_page==1){const int actions[]={ACCEPT,BACK,VIEW,FAV,RECENT,PGUP,PGDN,START,MENU};const char*desc[]={"Open / change","Back / cancel","Search / help","Set favourite","View / source","Previous tab","Next tab","Launch mode","Settings"};for(int i=0;i<9;i++){text(8,24+i*14,control_label(actions[i]),0,11);text(82,24+i*14,tr(desc[i]),0,25);}}
      else{const char*labels[]={control_label(HISTORY),control_label(FAVOURITES),"D-pad","VOL","MENU + VOL","START"};const char*desc[]={"Recents","Favourites","Navigate","Volume","Brightness","Startup escape"};for(int i=0;i<6;i++){text(8,24+i*14,labels[i],0,11);text(82,24+i*14,tr(desc[i]),0,25);}}
      text(220,146,help_page==1?"1/2":"2/2",0,3);return;}
    int total;const int *items=settings_items(&total);if(settings_category<0)total=9;
    int selected=settings_category<0?category_choice:setting;
    if(selected<settings_top)settings_top=selected;if(selected>=settings_top+9)settings_top=selected-8;
    for(int row=0;row<9&&settings_top+row<total;row++){
        int pos=settings_top+row,id=settings_category<0?-1:items[pos],y=24+row*14;const char*value=">";
        switch(id){case SET_APPS_GAMES:value=apps_in_games?"On":"Off";break;case SET_BOOT:value=boot_names[boot_destination];break;case SET_HOME_ENABLED:value=home_enabled?"On":"Off";break;case SET_SOURCE:value=home_favourites?"Favourites":"Recents";break;case SET_QUICK:value=quick_names[quick_key];break;case SET_LANGUAGE:value=language_names[language];break;case SET_CONTROLLER:value=controllers[controller_selected].name;break;case SET_RESET_CONTROLS:value=">";break;case SET_PT:value=pixel_transparency?"On":"Off";break;case SET_DARK:value=dark_mode?"On":"Off";break;case SET_UISOUND:value=ui_sounds?"On":"Off";break;case SET_STARTSOUND:value=startup_sound?"On":"Off";break;case SET_SYSICON:value=system_icons?"Systems":"Folders";break;case SET_CLOCK:value=clock_12?"12 hour":"24 hour";break;case SET_VIEW:value=viewnames[viewmode];break;case SET_COLOUR:value=theme_names[colour];break;case SET_HOME:value=home_button==1?"Favs":home_button==2?"Recents":"Apps";break;case SET_FOLDERS:value=list_folders==2?"List + Art":list_folders?"List":"Off";break;case SET_NAMES:value=full_system_names?"Full":"Short";break;case SET_CLEAN:value=clean_list?"On":"Off";break;case SET_GBAART:value=gba_art?"On":"Off";break;case SET_BORDER:value=border_names[art_border];break;case SET_ROUND:value=round_names[rounded_corners];break;case SET_VSIDE:value=vside_names[vertical_side];break;case SET_HSIDE:value=hside_names[horizontal_side];break;case SET_ARTPOS:value=art_positions[art_position];break;case SET_LCD:value=lcd_grid?"On":"Off";break;case SET_AUTOBOOT:value=autoboot_enabled()?"On":"Off";break;case SET_HFIT:value=horizontal_fit?"Contain":"Overlap";break;}
        if(id>=SET_BIND_FIRST)value=control_label(control_actions[id-SET_BIND_FIRST]);text(23,y,tr(id<0?category_names[pos]:id>=SET_BIND_FIRST?control_names[id-SET_BIND_FIRST]:labels[id]),0,14);if(pos==selected)rect(112,y,112,13,theme_accent[colour]);text(119,y,id==SET_CONTROLLER||id>=SET_BIND_FIRST?value:tr(value),pos==selected?0xffffff:0,17);
    }
    if(settings_top)text(230,25,"^",0,1);if(settings_top+9<total)text(230,137,"v",0,1);
}
static int effective_view(void){if(section==3||section==4)return 0;if(list_folders&&count){int all=1;for(int i=0;i<count;i++)if(!entries[i].dir){all=0;break;}if(all)return list_folders==2?1:0;}return viewmode;}
static uint64_t marquee_at,marquee_last;static int marquee_needed;
/* One selected title, sampled at physical pixels. The clip is invariant
 * during the initial pause, scrolling, wrap and subsequent cycles. */
static unsigned char scroll_bits[12][3200];
static int scroll_x,scroll_y,scroll_right,scroll_period,scroll_active;
static float scroll_offset;
static uint32_t scroll_colour;
static void list_marquee(int x,int y,const char*name,int right,uint32_t colour){
 int length=glyph_count(name),width=right-x;
 if(right>W)right=W;
 if(length*6<=width){text(x,y,name,colour,width/6);return;}
 marquee_needed=1;scroll_active=1;scroll_x=x;scroll_y=y;scroll_right=right;scroll_colour=colour;
 scroll_period=length*6+18;if(scroll_period>3200)scroll_period=3200;
 uint64_t elapsed=ui_millis()-marquee_at;
 if(preview&&getenv("DS_STYLE_PREVIEW_SCROLL_MS"))elapsed=strtoull(getenv("DS_STYLE_PREVIEW_SCROLL_MS"),NULL,10);
 scroll_offset=elapsed<=333?0:fmodf((elapsed-333)*0.030f,(float)scroll_period);
 memset(scroll_bits,0,sizeof scroll_bits);
 int col=0;const char*q=name;while(*q&&col+8<scroll_period){int k=nextchar(&q);const unsigned char*g=font+'?'*12;
  if(k<128)g=font+k*12;else for(size_t i=0;i<sizeof latin_codepoints/sizeof *latin_codepoints;i++)if(latin_codepoints[i]==(unsigned)k){g=latin_font+i*12;break;}
  for(int yy=0;yy<12;yy++)for(int xx=0;xx<8;xx++)if(g[yy]&(128>>xx))scroll_bits[yy][col+xx]=1;
  col+=6;
 }
}
static void ui_browser(void) {
    int mode=effective_view();
    blit(ui_background(mode==2?2:mode==3?3:1),0,0,W,H);
    char heading[64];strcopy(heading,*here?filename(here):"Games",sizeof heading);
    ui_titlebar(section==1?tr("Favourites"):section==2?tr("Recents"):section==3?tr("Apps"):systems_view()&&!search_active?tr("Systems"):heading);
    if(search_active){char label[80];snprintf(label,sizeof label,"%s: %s",tr("Search"),search_query);ui_titlebar(label);}
    if(!count){const char*msg=section==1?"No favourites":section==2?"No recent games":"No games found";if(mode==2)centered(39,128,162,tr(msg),0);else if(mode==3)centered(97,84,133,tr(msg),0);else centered(17,75,207,tr(msg),0);return;}
    if(mode<2) {
        int top=list_top,ay=art_position==0?27:art_position==1?60:92;
        Pic selected_art=mode==1?getart(&entries[choice]):(Pic){0};int withart=mode==1&&selected_art.p&&(entries[choice].dir||!selected_art.builtin);int artw=90,arth=60;if(withart)art_size(selected_art,90,60,3,&artw,&arth);int artleft=232-artw,arttop=ay+(60-arth)/2;
        for(int i=top;i<count&&i<top+10;i++) {
            int y=20+(i-top)*14,columns=clean_list?37:32;
            int overlaps=withart&&y+12>arttop&&y<arttop+arth;
            if(i==choice)rect(17,y,223,13,theme_accent[colour]);
            icon(entry_icon(&entries[i]),0,y,16,14);
            char name[520];
            if(clean_list&&!entries[i].dir&&!entries[i].app)Launcher_CleanTitle(entries[i].name,name,sizeof name);
            else strcopy(name,entries[i].dir?system_title(entries[i].name):entries[i].name,sizeof name);
            if(!entries[i].dir&&!entries[i].app&&favindex(entries[i].path)>=0){size_t len=strlen(name);snprintf(name+len,sizeof name-len," <3");}
            if(i==choice)list_marquee(17,y,name,overlaps?artleft-3:17+columns*6,0xffffff);
            else text(17,y,name,0,columns);
            if(entries[i].dir&&!clean_list&&columns==32)text(221,y,"DIR",i==choice?0xffffff:0,3);
            else if(!entries[i].app&&!clean_list&&columns==32){struct stat st;if(!stat(entries[i].path,&st)){unsigned long long size=(unsigned long long)st.st_size;char unit='B';if(size>=1048576){size/=1048576;unit='M';}else if(size>=1024){size/=1024;unit='K';}char value[32];snprintf(value,sizeof value,"%4llu%c",size,unit);text(208,y,value,i==choice?0xffffff:0,5);}}
        }
        if(withart)ui_art(&entries[choice],142,ay,90,60,3);
    } else {
        int tx,ty,tw,th;
        if(mode==2) {
            if(choice>0)ui_art(&entries[choice-1],LAUNCHER_HORZ_LEFT_X,(horizontal_side==1?27:horizontal_side==2?67:47),60,40,2);
            if(choice+1<count)ui_art(&entries[choice+1],LAUNCHER_HORZ_RIGHT_X,(horizontal_side==1?27:horizontal_side==2?67:47),60,40,2);
            ui_art(&entries[choice],LAUNCHER_HORZ_THUMB_X,LAUNCHER_HORZ_THUMB_Y,120,80,0);
            tx=LAUNCHER_HORZ_TITLE_X;ty=LAUNCHER_HORZ_TITLE_Y;tw=LAUNCHER_HORZ_TITLE_W;th=LAUNCHER_HORZ_TITLE_H;
        } else {
            if(choice>0)ui_art(&entries[choice-1],25,LAUNCHER_VERT_PREV_Y,48,32,2);
            if(choice+1<count)ui_art(&entries[choice+1],25,LAUNCHER_VERT_NEXT_Y,48,32,2);
            ui_art(&entries[choice],LAUNCHER_VERT_THUMB_X,LAUNCHER_VERT_THUMB_Y,84,56,0);
            tx=LAUNCHER_VERT_TITLE_X;ty=LAUNCHER_VERT_TITLE_Y;tw=LAUNCHER_VERT_TITLE_W;th=LAUNCHER_VERT_TITLE_H;
        }
        char title[128],lines[3][32];
        if(entries[choice].dir||entries[choice].app)strcopy(title,entries[choice].name,sizeof title);
        else Launcher_CleanTitle(entries[choice].dir?system_title(entries[choice].name):entries[choice].name,title,sizeof title);
        int n=Launcher_SplitTitle(title,lines),y=ty+(th-n*12)/2;if(y<ty+2)y=ty+2;
        for(int i=0;i<n;i++){int x=tx+(tw-(int)strlen(lines[i])*6)/2;if(x<tx+4)x=tx+4;text(x,y+i*12,lines[i],0,31);}
        if(favindex(entries[choice].path)>=0)heart(mode==2?45:97,mode==2?118:64);
    }
}
static void hardware_popup(void){
 if(!hardware_until||ui_millis()>=hardware_until)return;
 rect(48,65,144,30,dark_mode?0x0d0d0d:0xffffff);border(48,65,144,30,dark_mode?0xffffff:0);border(50,67,140,26,theme_accent[colour]);
 if(hardware_kind==1){rect(59,77,3,6,dark_mode?0xffffff:0);for(int i=0;i<4;i++)rect(62+i,76-i,1,8+2*i,dark_mode?0xffffff:0);rect(69,77,1,6,dark_mode?0xffffff:0);dot(68,76,dark_mode?0xffffff:0);dot(68,83,dark_mode?0xffffff:0);}
 else{uint32_t ink=dark_mode?0xffffff:0;border(61,77,5,5,ink);rect(63,73,1,2,ink);rect(63,84,1,2,ink);rect(57,79,2,1,ink);rect(68,79,2,1,ink);}
 if(hardware_level<0)text(78,74,"Unavailable",0,16);else{border(78,76,103,8,dark_mode?0xffffff:0);rect(80,78,99*hardware_level/100,4,theme_accent[colour]);}
}
#include "about_snake.h"
static void draw(void) {
    pt_valid=0;scroll_active=0;marquee_needed=0;marquee_last=ui_millis();native_art_count=0;motion_count=0;memset(motion_owner,0,sizeof motion_owner);memset(art_owner,0,sizeof art_owner);
    rect(0,0,W,H,0xffffff);
    if(about_page>=0)about_draw();else if(page==0)ui_home();else if(page==2)ui_settings();else ui_browser();
    if(setting_help!=-1||capture_action>=0||search_keyboard||notice[0]||power_confirm||launch_mode_popup||launching||(hardware_until&&ui_millis()<hardware_until))scroll_active=0;
    hardware_popup();
    if(launching){for(int y=66;y<94;y++)rect(67,y,106,1,y%2?(dark_mode?0x0d0d0d:0xffffff):(dark_mode?0x1e1e1e:0xe7e7e7));border(67,66,106,28,dark_mode?0x7b7b7b:0x494949);centered(75,73,90,tr("Launching"),0);}
    else if(launch_mode_popup){char accept[64],back[64];snprintf(accept,sizeof accept,"%s: RetroArch",control_label(ACCEPT));snprintf(back,sizeof back,"%s: Game Rooms",control_label(BACK));adaptive_popup(tr("Set launch mode"),accept,back);}
    else if(power_confirm){char accept[64],back[64];snprintf(accept,sizeof accept,"%s: %s",control_label(ACCEPT),tr("Yes"));snprintf(back,sizeof back,"%s: %s",control_label(BACK),tr("Cancel"));adaptive_popup(tr(power_confirm==1?"Reboot?":power_confirm==2?"Shutdown?":"Change favourite?"),accept,back);}
    else if(notice[0]) {
        char lines[3][32];int n=Launcher_SplitTitle(tr(notice),lines),w=0;
        for(int i=0;i<n;i++){int len=glyph_count(lines[i])*6;if(len>w)w=len;}
        if(w<90)w=90;w+=16;int h=n*12+16,x=(W-w)/2,y=(H-h)/2;
        for(int yy=y;yy<y+h;yy++)rect(x,yy,w,1,yy%2?(dark_mode?0x0d0d0d:0xffffff):(dark_mode?0x1e1e1e:0xe7e7e7));
        border(x,y,w,h,dark_mode?0x7b7b7b:0x494949);
        for(int i=0;i<n;i++)centered(x+8,y+7+i*12,w-16,lines[i],0);


    }
    extra_draw();
}
/* Independent CPU implementation of the lcd1x intensity equation, sampled at
 * physical pixel centres. No shader, shader loader or emulator setting is used.
 * Defaults: horizontal modulation 16, vertical modulation 4; no RGB stripes.
 */
static void prepare_lcd(int scale) {
    if(ui_scale_cache==scale)return;ui_scale_cache=scale;
    for(int y=0;y<scale;y++)for(int x=0;x<scale;x++) {
        double gx=(4.0-cos(6.283185307179586*(x+0.5)/scale))/5.0;
        double gy=(16.0-cos(6.283185307179586*(y+0.5)/scale))/17.0;
        for(int c=0;c<256;c++)lcd_table[y*scale+x][c]=(unsigned char)(c*gx*gy+0.5);
    }
}
static uint32_t output_pixel(int x,int y,int scale) {
    if(pixel_transparency&&!pt_building&&scale==3){pt_prepare();return pt_frame[y*720+x];}
    int logical=(y/scale)*W+x/scale;uint32_t rgb=pixels[logical];
    int owner=art_owner[logical];if(owner){NativeArt*a=&native_art[owner-1];
        int px=((x-a->x*scale)*a->pic.w)/(a->w*scale),py=((y-a->y*scale)*a->pic.h)/(a->h*scale);
        if(px>=0&&px<a->pic.w&&py>=0&&py<a->pic.h){unsigned char*q=a->pic.p+4*(py*a->pic.w+px);rgb=(q[0]<<16)|(q[1]<<8)|q[2];}
    }
    if(motion_owner[logical]){float px=(x+0.5f)/scale,py=(y+0.5f)/scale;for(int i=0;i<motion_count;i++){MotionRect*r=&motion_rects[i];if(px>=r->x&&px<r->x+r->w&&py>=r->y&&py<r->y+r->h){rgb=theme_accent[colour];break;}}}
    if(scroll_active&&x>=scroll_x*scale&&x<scroll_right*scale&&y>=scroll_y*scale&&y<(scroll_y+12)*scale){
     int col=(int)floorf((x+0.5f)/scale-scroll_x+scroll_offset)%scroll_period;
     if(scroll_bits[y/scale-scroll_y][col])rgb=scroll_colour;
    }
    if(!lcd_grid||scale<2||scale>32)return rgb;
    prepare_lcd(scale);unsigned char *map=lcd_table[(y%scale)*scale+x%scale];
    return (map[(rgb>>16)&255]<<16)|(map[(rgb>>8)&255]<<8)|map[rgb&255];
}

/* Native 3x scanline, shared by the Linux framebuffer and host equivalence
 * check. Flat UI pixels are replicated; artwork and moving corners retain
 * physical-pixel sampling. No frame skipping or fixed GBA animation steps. */
static void output_row3(uint32_t*out,int y,uint32_t alpha){
 if(pixel_transparency&&!pt_building){pt_prepare();for(int x=0;x<720;x++)out[x]=pt_frame[y*720+x]|alpha;return;}
 int logical=(y/3)*W,scroll_row=scroll_active&&y/3>=scroll_y&&y/3<scroll_y+12;
 /* The static interface makes up most of a frame. LCD previously forced
  * every physical pixel through the general artwork/motion sampler. Cache
  * the three LCD phases per flat-colour run instead, with identical pixels. */
 uint32_t last=0xffffffff,phase[3]={0};
 if(lcd_grid)prepare_lcd(3);
 for(int x=0;x<W;x++){
  if(!art_owner[logical+x]&&!motion_owner[logical+x]&&!(scroll_row&&x>=scroll_x&&x<scroll_right)){
   uint32_t c=pixels[logical+x];
   if(!lcd_grid){c|=alpha;out[x*3]=out[x*3+1]=out[x*3+2]=c;}
   else{
    if(c!=last){last=c;for(int sub=0;sub<3;sub++){unsigned char*map=lcd_table[(y%3)*3+sub];phase[sub]=((uint32_t)map[(c>>16)&255]<<16)|((uint32_t)map[(c>>8)&255]<<8)|map[c&255]|alpha;}}
    out[x*3]=phase[0];out[x*3+1]=phase[1];out[x*3+2]=phase[2];
   }
  }else for(int sub=0;sub<3;sub++)out[x*3+sub]=output_pixel(x*3+sub,y,3)|alpha;
 }
}
/* Host-only diagnostic, not a panel-FPS claim. Both paths process all pixels;
 * equal checksums and output_equivalent guard the cached implementation. */
static void output_benchmark(void){
 int saved=lcd_grid;lcd_grid=1;prepare_lcd(3);uint32_t row[W*3];uint64_t sums[2]={0};
 for(int mode=0;mode<2;mode++){uint64_t at=ui_millis();
  for(int frame=0;frame<100;frame++)for(int y=0;y<H*3;y++){
   if(mode)output_row3(row,y,0xff000000);else for(int x=0;x<W*3;x++)row[x]=output_pixel(x,y,3)|0xff000000;
   for(int x=0;x<W*3;x++)sums[mode]+=row[x];
  }
  printf("lcd_benchmark path=%s frames=100 ms=%llu checksum=%llu\n",mode?"cached":"reference",(unsigned long long)(ui_millis()-at),(unsigned long long)sums[mode]);
 }
 if(sums[0]!=sums[1])fprintf(stderr,"LCD benchmark checksum mismatch\n");lcd_grid=saved;
}
static int output_equivalent(void){int saved=lcd_grid;uint32_t row[W*3];draw();
 for(int mode=0;mode<2;mode++){lcd_grid=mode;pt_valid=0;for(int y=0;y<H*3;y++){output_row3(row,y,0xff000000);for(int x=0;x<W*3;x++)if(row[x]!=(output_pixel(x,y,3)|0xff000000)){lcd_grid=saved;return 0;}}}
 lcd_grid=saved;return 1;
}
#include "pixel_transparency.h"
