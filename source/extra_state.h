/* User-facing additions; included after the core browser helpers. */
static int boot_destination,home_enabled=1,quick_key,startup_direct;
static uint64_t boot_warning_at;
static int extra_revision,setting_help=-1;
static const char*boot_names[]={"Home","Games","Recents","Favourites","Last game"};
static const char*quick_names[]={"Off","A","B","X","Y","SELECT","L","R"};
static const int quick_codes[]={0,304,305,307,306,310,308,309};
static void extra_config(int save){
 char path[PATHLEN],data[128];statepath(path,"options.txt");
 if(save){snprintf(data,sizeof data,"%d %d %d %d %d\n",language,boot_destination,home_enabled,quick_key,apps_in_games);atomic_text(path,data);return;}
 FILE*f=fopen(path,"r");if(f){int a,b,c,d;if(fscanf(f,"%d %d %d %d",&a,&b,&c,&d)==4){language=a>=0&&a<8?a:0;boot_destination=b>=0&&b<5?b:0;home_enabled=c!=0;quick_key=d>=0&&d<8?d:0;int e;apps_in_games=0;if(fscanf(f,"%d",&e)==1)apps_in_games=e==1;}fclose(f);}
}
static Entry*search_entries;static int search_count,search_choice,search_top,search_active,search_keyboard,key_cell;
static char search_query[49];
static unsigned fold_letter(unsigned c){
 if(c>='A'&&c<='Z')return c+32;
 if((c>=192&&c<=197)||(c>=224&&c<=229))return 'a';
 if(c==199||c==231)return 'c';if((c>=200&&c<=203)||(c>=232&&c<=235))return 'e';
 if((c>=204&&c<=207)||(c>=236&&c<=239))return 'i';if(c==209||c==241)return 'n';
 if((c>=210&&c<=214)||(c>=242&&c<=246)||c==216||c==248)return 'o';
 if((c>=217&&c<=220)||(c>=249&&c<=252))return 'u';return c;
}
static unsigned search_char(const char**s){unsigned c=(unsigned char)*(*s)++;if(c>=192&&c<224&&(**s&192)==128)c=((c&31)<<6)|((unsigned char)*(*s)++&63);return fold_letter(c);}
static int search_match(const char*s,const char*q){if(!*q)return 1;for(;*s;s++){const char*a=s,*b=q;while(*a&&*b){if(search_char(&a)!=search_char(&b))break;if(!*b)return 1;}}return 0;}
typedef struct GameSearchPath {struct GameSearchPath*next;char path[];} GameSearchPath;
static GameSearchPath*search_paths;static int search_global;
static void search_index(const char*dir,int level){
 if(level>63)return;DIR*d=opendir(dir);if(!d)return;struct dirent*de;
 while((de=readdir(d))){if(hidden_folder(de->d_name))continue;char path[PATHLEN];struct stat st;if(!join(path,sizeof path,dir,de->d_name))continue;
#ifdef _WIN32
 if(stat(path,&st))continue;
#else
 if(lstat(path,&st)||S_ISLNK(st.st_mode))continue;
#endif
 if(S_ISDIR(st.st_mode)){search_index(path,level+1);continue;}
 if(!S_ISREG(st.st_mode)||!gamefile(de->d_name))continue;
 GameSearchPath*n=malloc(sizeof*n+strlen(path)+1);if(!n){message("Not enough memory");break;}strcpy(n->path,path);n->next=search_paths;search_paths=n;
 }closedir(d);
}
static void search_filter(void){count=choice=list_top=0;if(search_global){for(GameSearchPath*n=search_paths;n;n=n->next){const char*name=strrchr(n->path,'/');name=name?name+1:n->path;if(search_match(name,search_query))addentry(name,n->path,0,0);}qsort(entries,(size_t)count,sizeof(Entry),cmpentry);}else for(int i=0;i<search_count;i++)if(search_match(search_entries[i].name,search_query))entries[count++]=search_entries[i];extra_revision++;}
static void search_end(int restore){if(search_entries){if(restore){memcpy(entries,search_entries,(size_t)search_count*sizeof(Entry));count=search_count;choice=search_choice;list_top=search_top;}free(search_entries);}while(search_paths){GameSearchPath*n=search_paths->next;free(search_paths);search_paths=n;}search_global=0;search_entries=NULL;search_active=search_keyboard=0;extra_revision++;}
static void search_begin(void){if(!search_active){search_entries=malloc((size_t)(count?count:1)*sizeof(Entry));if(!search_entries){message("Not enough memory");return;}memcpy(search_entries,entries,(size_t)count*sizeof(Entry));search_count=count;search_choice=choice;search_top=list_top;search_query[0]=0;search_active=1;key_cell=0;search_global=systems_view()||section==4;if(search_global){for(int i=0;i<2;i++)if(roots[i][0])search_index(roots[i],0);search_filter();}}search_keyboard=1;extra_revision++;}
static const char*keyboard_keys="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-'. ";
static int search_input(int k){
 if(!search_active)return 0;
 if(k==BACK){search_end(1);return 1;}
 if(!search_keyboard){if(k==VIEW){search_keyboard=1;extra_revision++;return 1;}return 0;}
 int old_cell=key_cell,old_keyboard=search_keyboard;
 if(k==UP&&key_cell>=10)key_cell-=10;if(k==DOWN&&key_cell<40)key_cell=key_cell>=30?40:key_cell+10;
 if(k==LEFT&&key_cell>0)key_cell--;if(k==RIGHT&&key_cell<41)key_cell++;
 if(k==ACCEPT){size_t n=strlen(search_query);if(key_cell==41)search_keyboard=0;else if(key_cell==40){if(n){search_query[n-1]=0;search_filter();}}else if(n<sizeof search_query-1){search_query[n]=keyboard_keys[key_cell];search_query[n+1]=0;search_filter();}}
 if(old_cell!=key_cell||old_keyboard!=search_keyboard)extra_revision++;return 1;
}
static void extra_draw(void);
static int extra_action(int k);
static void setting_help_open(int id);

static void adaptive_popup(const char*title,const char*first,const char*second);
