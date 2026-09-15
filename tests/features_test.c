#define main dsstyle_frontend_main

#include "../source/dsstyle.c"

#undef main

#include <assert.h>

static void touch(const char*p){FILE*f=fopen(p,"wb");assert(f);fputs("ROM",f);fclose(f);}

int main(int argc,char**argv){assert(argc==2);preview=1;strcopy(base,argv[1],sizeof base);join(roots[0],PATHLEN,base,"roms0");join(roots[1],PATHLEN,base,"roms1");MKDIR(roots[0]);MKDIR(roots[1]);controller_load();

 /* Duplicate bindings swap; fixed directions cannot be assigned. */

 int old=controllers[0].code[0];controller_bind(0,0,controllers[0].code[1]);assert(controllers[0].code[0]==305&&controllers[0].code[1]==old);assert(controller_valid(&controllers[0]));controller_bind(0,0,103);assert(controllers[0].code[0]==305);assert(controller_key(0,103)==UP&&controller_key(0,545)==DOWN);controller_defaults(0);controller_save();controller_load();assert(controllers[0].code[0]==304);

 controllers[1].id=42;controllers[1].external=1;controller_count=2;controller_defaults(1);controller_bind(1,0,308);assert(controller_valid(&controllers[1])&&controllers[0].code[0]==304);controller_save();controller_load();assert(controller_count==2&&controllers[1].code[0]==308);controller_selected=0;

 controllers[1].available[288]=controllers[1].available[289]=1;controller_defaults(1);assert(controllers[1].code[0]==288&&controllers[1].code[1]==289&&controller_valid(&controllers[1]));

 puts("mapping swaps, fixed D-pad, per-device persistence and reset passed");

 count=choice=list_top=0;addentry("Étoile.gba","/test1",0,0);addentry("Mario.gba","/test2",0,0);choice=1;search_begin();int revision=extra_revision;search_input(UP);assert(extra_revision==revision);key_cell=40;search_input(ACCEPT);assert(extra_revision==revision);strcpy(search_query,"ETO");search_filter();assert(count==1&&!strcmp(entries[0].name,"Étoile.gba"));search_keyboard=0;assert(search_input(BACK));assert(count==2&&choice==1&&!search_active);search_begin();key_cell=41;search_input(ACCEPT);assert(!search_keyboard&&search_active);search_end(1);puts("accent-insensitive filtering and Back restore passed");

 char system[PATHLEN],sub[PATHLEN],rom[PATHLEN],file[PATHLEN];join(system,PATHLEN,roots[1],"GBA");MKDIR(system);join(sub,PATHLEN,system,"RPG");MKDIR(sub);join(rom,PATHLEN,sub,"Golden Sun.gba");touch(rom);

 join(file,PATHLEN,base,"favorite.bin");_putenv_s("DS_STYLE_STOCK_FAV_TEST",file);stock_import();assert(stock_size==14);assert(stock_toggle(rom,0)==1);assert(shared_route(rom,0)==2);char saved[PATHLEN];favcount=0;stock_import();assert(favcount==1&&!strcmp(favourites[0],rom));int ordinal;assert(stock_row("Golden Sun.gba:GBA:RPG:3:1:1",saved,&ordinal)&&!strcmp(saved,rom));assert(!stock_row("bad.gba:GBA:../bad:3:1:1",saved,&ordinal));assert(stock_toggle(rom,1)==1);favcount=0;stock_import();assert(favcount==0);puts("nested stock favourites, empty file, CRC, traversal and deletion passed");

 join(file,PATHLEN,base,"history.bin");_putenv_s("DS_STYLE_STOCK_HISTORY_TEST",file);launch_route_override=1;assert(history_add(rom));assert(shared_route(rom,1)==1);launch_route_override=0;recentcount=0;history_import();assert(recentcount==1&&!strcmp(recents[0],rom));assert(history_add(rom));recentcount=0;history_import();assert(recentcount==1);for(int i=0;i<12;i++){char name[32];snprintf(name,sizeof name,"Game%d.gba",i);join(rom,PATHLEN,system,name);touch(rom);assert(history_add(rom));}history_import();assert(recentcount==10&&strstr(recents[0],"Game11.gba"));FILE*f=fopen(file,"ab");fputc(1,f);fclose(f);assert(!history_add(rom));puts("shared stock history order, deduplication, ten-entry cap and invalid CRC passed");

 boot_destination=4;home_enabled=1;notice[0]=0;startup_destination(1,1);assert(page==0&&!notice[0]);home_enabled=0;startup_destination(1,0);assert(page==1);boot_destination=0;startup_destination(0,0);assert(page==1);home_enabled=1;boot_destination=4;boot_warning_at=ui_millis()-1;setting_help=-1;extra_tick();assert(setting_help==-2);extra_action(BACK);assert(setting_help==-1);puts("START overrides quick/last-game, disabled Home and delayed warning passed");

 for(int l=0;l<8;l++){language=l;assert(*tr("Settings"));for(int id=0;id<SET_BIND_FIRST;id++)assert(*tr(setting_explanation(id)));}language=0;puts("all setting help and eight language tables passed");


 language=0;assert(!strcmp(tr("Colour"),"Colour"));language=7;assert(!strcmp(tr("Colour"),"Color"));assert(!strcmp(tr("Favourites"),"Favorites"));extra_config(1);language=0;extra_config(0);assert(language==7);language=0;
 page=2;settings_category=-1;setting_help=-1;extra_action(VIEW);assert(setting_help==-1);
 char sys0[PATHLEN],pk0[PATHLEN],pk1[PATHLEN];join(sys0,PATHLEN,roots[0],"GB");MKDIR(sys0);join(pk0,PATHLEN,sys0,"Pokemon Blue.gb");touch(pk0);join(pk1,PATHLEN,sub,"Pokemon Emerald.gba");touch(pk1);
 browse(roots[0]);int original=count;search_begin();assert(search_global);strcpy(search_query,"pokemon");search_filter();assert(count==2&&!entries[0].dir&&!entries[1].dir);assert(strcmp(entries[0].path,entries[1].path));search_end(1);assert(count==original&&systems_view());
 puts("UK/US persistence, submenu help no-op, cross-card nested game search and restoration passed");

 char ap0[PATHLEN],ap1[PATHLEN],appfile[PATHLEN];join(ap0,PATHLEN,roots[0],"APPS");MKDIR(ap0);join(ap1,PATHLEN,roots[1],"APPS");MKDIR(ap1);
 join(appfile,PATHLEN,ap0,"Clock.sh");touch(appfile);join(appfile,PATHLEN,ap1,"Reader.sh");touch(appfile);join(appfile,PATHLEN,ap0,"DS Style.sh");touch(appfile);join(appfile,PATHLEN,ap1,"DSStyle.sh");touch(appfile);join(appfile,PATHLEN,ap0,"NotAnApp.sh");MKDIR(appfile);
 apps();int installed=0;for(int i=0;i<count;i++)if(entries[i].app==2){installed++;assert(!strcmp(entries[i].name,"Clock.sh")||!strcmp(entries[i].name,"Reader.sh"));}assert(installed==2);
 apps_in_games=0;browse(roots[0]);for(int i=0;i<count;i++)assert(entries[i].app!=4);
 apps_in_games=1;extra_config(1);apps_in_games=0;extra_config(0);assert(apps_in_games);browse(roots[0]);assert(entries[count-1].app==4);choice=count-1;int origin_choice=choice;launch_selected();assert(section==3&&apps_return_systems);action_impl(BACK);assert(systems_view()&&choice==origin_choice);
 browse(sys0);for(int i=0;i<count;i++)assert(entries[i].app!=4);
 puts("Apps on both cards, DS Style exclusion, regular launcher filtering, opt-in persistence and Systems return passed");

 settings_category=6;int total;const int*ci=settings_items(&total);for(int i=0;i<total;i++)assert(ci[i]!=SET_APPS_GAMES);assert(CONTROL_CAPTURE_MS==5000);preview_frame_time=1000;capture_action=0;capture_until=6000;extra_tick();assert(capture_action==0);preview_frame_time=5999;extra_tick();assert(capture_action==0);preview_frame_time=6000;extra_tick();assert(capture_action==-1);preview_frame_time=0;
 snake_high=0;snake_high_loaded=1;launcher_snake_length=13;snake_high_update();assert(snake_high==9);snake_high_save();snake_high=0;snake_high_loaded=0;snake_high_load();assert(snake_high==9);launcher_snake_length=4;snake_high_save();snake_high=0;snake_high_loaded=0;snake_high_load();assert(snake_high==9);
 puts("Controls exclude Apps, binding timeout is five seconds, Snake record persists and never decreases");
 return 0;

}

