static const char*setting_explanation(int id){
 static const char*help[]={
 "Choose a 12 or 24 hour clock.",
 "Use list views for folders. Game folders keep your chosen view.",
 "Show full system names. The top bar keeps short names.",
 "Hide file extensions, tags and file sizes in lists.",
 "Choose the accent colour used by bars and selections.",
 "Choose List, List + Art, Horizontal or Vertical.",
 "Render artwork at GBA resolution for a pixel look.",
 "Choose the colour of the artwork outline, or turn it off.",
 "Round artwork corners. No Start keeps home artwork square.",
 "Align the smaller images in vertical view.",
 "Align the smaller images in horizontal view.",
 "Contain wide images, or let them overlap in horizontal view.",
 "Place list artwork at the top, centre or bottom.",
 "Choose Apps, Favourites or Recents for the home button.",
 "Add an LCD grid to the screen.",
 "Return to Stock OS. Stock settings remain available there.",
 "Start DS Style automatically when the device boots.",
 "Restart the device using the stock power route.",
 "Turn off the device using the stock power route.",
 "Open installed applications.",
 "Play sounds when a menu action takes place.",
 "Play a startup sound on device boot.",
 "Use console icons or folder icons for systems.",
 "Toggle dark backgrounds.",
 "Add a pixel transparency effect.",
 "Choose where DS Style starts. Hold START at startup to skip automatic game launch.",
 "Turn off the home screen. Home boot then opens Games.",
 "Choose Recents or Favourites for the home game.",
 "Hold this button at startup to launch the home game.",
 "Choose the interface language. Game filenames stay unchanged.",
 "Choose a controller to edit. Connected devices are detected automatically.",
 "Restore this controller's default buttons. D-pad navigation stays fixed.",
 "Show an Apps shortcut in the Systems list."
 };
 if(id<0)return "Hold START while DS Style starts to return to the menu when Last game boot is enabled.";
 if(id>=SET_BIND_FIRST)return "Press a new button to bind. Conflicts swap buttons. D-pad Left cancels. Binding times out after 5 seconds.";
 return id<(int)(sizeof help/sizeof*help)?help[id]:"Use left or right to change this setting.";
}
static void setting_help_open(int id){setting_help=id;extra_revision++;}
static void extra_box(const char*body){
 const char*s=tr(body);char lines[9][100]={{0}};int n=0;size_t used=0;int width=0;
 while(*s&&n<9){const char*end=strchr(s,' ');size_t len=end?(size_t)(end-s):strlen(s);char word[100];if(len>=sizeof word)len=sizeof word-1;memcpy(word,s,len);word[len]=0;int cells=glyph_count(word);
 if(used&&width+1+cells>33){n++;used=0;width=0;if(n==9)break;}
 if(used){lines[n][used++]=' ';width++;}if(used+len<sizeof lines[n]){memcpy(lines[n]+used,word,len);used+=len;lines[n][used]=0;width+=cells;}s+=len;if(*s==' ')s++;
 }
 if(n<9)n++;int h=n*12+20,y=(H-h)/2;if(y<3)y=3;rect(12,y,216,h,dark_mode?0x151515:0xffffff);border(12,y,216,h,dark_mode?0x7b7b7b:0x494949);for(int i=0;i<n;i++)text(21,y+9+i*12,lines[i],0,33);
}
static void extra_draw(void){
 if(search_keyboard){rect(8,25,224,128,dark_mode?0x151515:0xffffff);border(8,25,224,128,theme_accent[colour]);text(16,30,tr("Search"),0,20);text(16,44,search_query+(strlen(search_query)>34?strlen(search_query)-34:0),0,34);border(14,42,212,16,0x848484);
 for(int i=0;i<40;i++){int x=15+i%10*21,y=63+i/10*16;if(i==key_cell)rect(x,y,19,14,theme_accent[colour]);char c[2]={keyboard_keys[i]==' '?'_':keyboard_keys[i],0};text(x+6,y+1,c,i==key_cell?0xffffff:0,1);}
 if(key_cell==40)rect(15,129,99,16,theme_accent[colour]);if(key_cell==41)rect(120,129,104,16,theme_accent[colour]);text(22,131,tr("Delete"),key_cell==40?0xffffff:0,14);text(128,131,tr("Results"),key_cell==41?0xffffff:0,14);
 }
 if(setting_help!=-1)extra_box(setting_explanation(setting_help));
 if(capture_action>=0)extra_box("Press a new button to bind. Conflicts swap buttons. D-pad Left cancels. Binding times out after 5 seconds.");
}
static int extra_action(int k){
 if(setting_help!=-1){if(k==ACCEPT||k==BACK||k==VIEW){setting_help=-1;extra_revision++;}return 1;}
 if(capture_action>=0){if(k==BACK)capture_action=-1;return 1;}
 if(notice[0]||power_confirm||launch_mode_popup)return 0;
 if(about_action(k))return 1;
 if(search_input(k))return 1;
 if(search_active&&(k==MENU||k==PGUP||k==PGDN||k==HISTORY||k==FAVOURITES)){search_end(1);}
 if(search_active&&k==ACCEPT&&count&&entries[choice].dir){char path[PATHLEN];strcopy(path,entries[choice].path,sizeof path);search_end(1);for(int i=0;i<count;i++)if(!strcmp(entries[i].path,path)){choice=i;break;}}
 if(k==VIEW){if(page==2&&!help_page){if(settings_category>=0)setting_help_open(setting_value());}else if(page==1)search_begin();return 1;}
 return 0;
}
static void extra_tick(void){if(capture_action>=0&&ui_millis()>=capture_until){capture_action=-1;extra_revision++;
#ifndef _WIN32
 hardware_dirty=1;
#endif
 }
if(snake_tick()){
#ifndef _WIN32
 hardware_dirty=1;
#endif
 }
if(boot_warning_at&&ui_millis()>=boot_warning_at){boot_warning_at=0;if(boot_destination==4){setting_help_open(-2);
#ifndef _WIN32
 hardware_dirty=1;
#endif
 }}}

static void adaptive_popup(const char*title,const char*first,const char*second){int n=second?3:2;const char*lines[]={title,first,second};int w=90;for(int i=0;i<n;i++){int size=glyph_count(lines[i])*6;if(size>w)w=size;}w+=16;if(w>224)w=224;int h=n*14+16,x=(W-w)/2,y=(H-h)/2;rect(x,y,w,h,dark_mode?0x151515:0xffffff);border(x,y,w,h,dark_mode?0x7b7b7b:0x494949);for(int i=0;i<n;i++)centered(x+8,y+8+i*14,w-16,lines[i],0);}
