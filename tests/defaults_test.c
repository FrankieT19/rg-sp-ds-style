#define main dsstyle_frontend_main
#include "../source/dsstyle.c"
#undef main
#include <assert.h>

static void check_defaults(void) {
    assert(viewmode==2 && list_folders==1 && clean_list==1);
    assert(full_system_names==1 && system_icons==1 && apps_in_games==0);
    assert(art_position==0 && horizontal_side==0 && horizontal_fit==1);
    assert(vertical_side==0 && gba_art==0 && art_border==3 && rounded_corners==2);
    assert(colour==0 && dark_mode==0 && lcd_grid==0 && pixel_transparency==0);
    assert(clock_12==1 && language==0 && boot_destination==0 && home_enabled==1);
    assert(home_favourites==0 && quick_key==0 && home_button==0);
    assert(ui_sounds==1 && startup_sound==1);
}
int main(int argc,char**argv) {
    assert(argc==2);preview=1;strcopy(base,argv[1],sizeof base);
    check_defaults();
    preferences(0);extra_preferences();sound_preferences(0);home_state(0);extra_config(0);
    check_defaults();
    /* Existing preferences override the release defaults, including style.txt. */
    viewmode=1;list_folders=2;clean_list=0;full_system_names=0;clock_12=0;
    art_border=4;rounded_corners=1;gba_art=1;horizontal_fit=0;colour=2;
    ui_sounds=0;startup_sound=0;home_button=2;language=7;boot_destination=1;
    home_enabled=0;quick_key=3;apps_in_games=1;home_favourites=1;
    preferences(1);sound_preferences(1);home_state(1);extra_config(1);
    viewmode=2;list_folders=1;clean_list=1;full_system_names=1;clock_12=1;
    art_border=3;rounded_corners=2;gba_art=0;horizontal_fit=1;colour=0;
    ui_sounds=1;startup_sound=1;home_button=0;language=0;boot_destination=0;
    home_enabled=1;quick_key=0;apps_in_games=0;home_favourites=0;
    preferences(0);extra_preferences();sound_preferences(0);home_state(0);extra_config(0);
    assert(viewmode==1 && list_folders==2 && clean_list==0 && full_system_names==0);
    assert(clock_12==0 && art_border==4 && rounded_corners==1 && gba_art==1);
    assert(horizontal_fit==0 && colour==2 && ui_sounds==0 && startup_sound==0);
    assert(home_button==2 && language==7 && boot_destination==1 && home_enabled==0);
    assert(quick_key==3 && apps_in_games==1 && home_favourites==1);
    puts("Release defaults and saved-preference preservation passed.");
    return 0;
}
