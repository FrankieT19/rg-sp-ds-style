typedef struct {char path[PATHLEN];unsigned used;Pic pic;} ArtSlot;
static ArtSlot art_slots[8];static unsigned art_clock;
static Pic resolve_art(const Entry*e) {
    Pic pic={0};char parent[PATHLEN],stem[256],path[PATHLEN],rel[PATHLEN];
    strcopy(parent,e->path,sizeof parent);char*slash=strrchr(parent,'/');if(!slash)return pic;*slash=0;
    strcopy(stem,e->name,sizeof stem);char*ext=strrchr(stem,'.');if(ext&&!e->dir)*ext=0;
    if(e->dir||e->app)return pic;
    /* Stock H700 artwork commonly retains the ROM extension: game.gba.png.
     * Support that before the scraper convention game.png. */
    const char*folders[]={"Imgs","imgs","images","media",""};
    const char*formats[]={"png","bmp","jpg","jpeg"};
    for(int d=0;d<5;d++)for(int full=1;full>=0;full--)for(int k=0;k<4;k++){
        snprintf(rel,sizeof rel,"%s%s%s.%s",folders[d],d==4?"":"/",full?e->name:stem,formats[k]);
        if(join(path,sizeof path,parent,rel)&&exists(path)){pic=loadpic(path);if(pic.p)return pic;}
    }
    return pic;
}
/* Folder Art accepts a short system folder name or its full display title,
 * case-insensitively on Linux as well as FAT. It never renames ROM folders. */
static Pic folder_art(const char *name){
 Pic pic={0};char root[PATHLEN],path[PATHLEN];join(root,sizeof root,base,"Folder Art");DIR*d=opendir(root);if(!d)return pic;struct dirent*de;
 while((de=readdir(d))){char stem[256];strcopy(stem,de->d_name,sizeof stem);char*ext=strrchr(stem,'.');if(!ext)continue;if(strcasecmp(ext,".png")&&strcasecmp(ext,".jpg")&&strcasecmp(ext,".jpeg")&&strcasecmp(ext,".bmp"))continue;*ext=0;
 if(strcasecmp(stem,name)&&strcasecmp(system_full_title(stem),system_full_title(name)))continue;
 if(join(path,sizeof path,root,de->d_name)){pic=loadpic(path);if(pic.p)break;}
 }closedir(d);return pic;
}
/* Walk parent names to identify a system even in nested collections. */
static Pic default_art(const Entry *e){
 Pic pic={0};if(e->app)return pic;char parent[PATHLEN],path[PATHLEN],rel[PATHLEN];strcopy(parent,e->path,sizeof parent);
 if(!e->dir){char*q=strrchr(parent,'/');if(q)*q=0;}
 for(int depth=0;depth<20&&*parent;depth++){
  const char*name=filename(parent);pic=folder_art(name);if(pic.p)return pic;char upper[256];strcopy(upper,name,sizeof upper);for(char*q=upper;*q;q++)if(*q>='a'&&*q<='z')*q-=32;
  snprintf(rel,sizeof rel,"assets/systems/%s/%s.png","wide",upper);
  if(join(path,sizeof path,base,rel)){pic=loadpic(path);if(pic.p){pic.builtin=1;return pic;}}
  char*q=strrchr(parent,'/');if(!q)break;*q=0;
 }
 const char*ext=strrchr(e->name,'.');const char*system=ext&&!strcasecmp(ext,".gba")?"GBA":ext&&!strcasecmp(ext,".gb")?"GB":ext&&!strcasecmp(ext,".gbc")?"GBC":ext&&!strcasecmp(ext,".nes")?"NES":NULL;
 if(system){snprintf(rel,sizeof rel,"assets/systems/%s/%s.png","wide",system);if(join(path,sizeof path,base,rel))pic=loadpic(path);pic.builtin=1;}
 return pic;
}
static Pic getart(const Entry*e) {
    art_clock++;int slot=0;
    for(int i=0;i<8;i++){
        if(art_slots[i].used&&!strcmp(art_slots[i].path,e->path)){art_slots[i].used=art_clock;return art_slots[i].pic;}
        if(art_slots[i].used<art_slots[slot].used)slot=i;
    }
    ArtSlot*a=&art_slots[slot];stbi_image_free(a->pic.p);a->pic=resolve_art(e);if(!a->pic.p){a->pic=default_art(e);a->pic.builtin=1;}
    if(a->pic.p&&(a->pic.w>1024||a->pic.h>1024)){int w=a->pic.w,h=a->pic.h;if(w>=h){h=h*1024/w;w=1024;}else{w=w*1024/h;h=1024;}if(w<1)w=1;if(h<1)h=1;unsigned char*small=malloc((size_t)w*h*4);if(small){for(int y=0;y<h;y++)for(int x=0;x<w;x++)memcpy(small+4*(y*w+x),a->pic.p+4*((y*a->pic.h/h)*a->pic.w+x*a->pic.w/w),4);stbi_image_free(a->pic.p);a->pic=(Pic){small,w,h,a->pic.builtin};}}
    strcopy(a->path,e->path,sizeof a->path);a->used=art_clock;return a->pic;
}
