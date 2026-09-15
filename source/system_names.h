/* Display aliases only; ROM paths and stock launch identifiers stay intact. */
static const char *system_full_title(const char *name){
 static const char *names[][2]={
 {"PS","PlayStation"},{"PSX","PlayStation"},{"PSP","PlayStation Portable"},
 {"GBA","Game Boy Advance"},{"GB","Game Boy"},{"GBC","Game Boy Color"},
 {"FC","Nintendo Entertainment System"},{"NES","Nintendo Entertainment System"},{"FAMICOM","Famicom"},
 {"SFC","Super Nintendo"},{"SNES","Super Nintendo"},{"N64","Nintendo 64"},{"NDS","Nintendo DS"},{"FDS","Famicom Disk System"},
 {"MD","Mega Drive"},{"GENESIS","Genesis"},{"MDCD","Mega CD"},{"SEGA32X","Sega 32X"},{"SMS","Master System"},{"GG","Game Gear"},
 {"PCE","PC Engine"},{"PCECD","PC Engine CD"},{"NGP","Neo Geo Pocket"},{"NGPC","Neo Geo Pocket Color"},{"NEOCD","Neo Geo CD"},{"NEOGEO","Neo Geo"},
 {"WS","WonderSwan"},{"WSC","WonderSwan Color"},{"VB","Virtual Boy"},{"GW","Game & Watch"},{"POKE","Pokemon Mini"},
 {"A2600","Atari 2600"},{"A5200","Atari 5200"},{"A7800","Atari 7800"},{"A800","Atari 800"},{"ATARIST","Atari ST"},{"LYNX","Atari Lynx"},
 {"C64","Commodore 64"},{"VIC20","Commodore VIC-20"},{"MSX","MSX"},{"AMIGA","Amiga"},{"DREAMCAST","Dreamcast"},{"SATURN","Sega Saturn"},
 {"CPS1","Capcom Play System"},{"CPS2","Capcom Play System II"},{"CPS3","Capcom Play System III"},{"FBNEO","FinalBurn Neo"},{"MAME","Arcade (MAME)"},
 {"NAOMI","Sega NAOMI"},{"ATOMISWAVE","Atomiswave"},{"PICO","PICO-8"},{"SCUMMVM","ScummVM"},{"EASYRPG","EasyRPG"},{"DOS","DOS"},{"PORTS","Ports"},
 {"SG-1000","Sega SG-1000"},{"COLECO","ColecoVision"},{"SCV","Super Cassette Vision"}
 };
 for(unsigned i=0;i<sizeof names/sizeof names[0];i++)if(!strcasecmp(name,names[i][0]))return names[i][1];return name;
}

static const char *system_title(const char *name){return full_system_names?system_full_title(name):name;}
