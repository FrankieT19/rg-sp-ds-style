/* Stock dmenu Version=1, decoded from device capture. CRC32 polynomial
 * edb88320, initial zero, no final xor, little endian trailer. Type bit0=SD2,
 * bit1=stock RA list. Preserve unknown records rather than rewrite them.
 * Nested path and flag follow the stock serializer and reader (field +324, flag +580).
 */
#define STOCK_FAV_BYTES 262144
static unsigned char stock_original[STOCK_FAV_BYTES];
static size_t stock_size;
static char stock_file[PATHLEN];
static uint32_t stock_crc(const unsigned char*p,size_t n){uint32_t c=0;while(n--){c^=*p++;for(int i=0;i<8;i++)c=(c>>1)^((c&1)?0xedb88320u:0);}return c;}
static int stock_read(unsigned char*b,size_t*n){FILE*f=fopen(stock_file,"rb");if(!f){if(errno!=ENOENT)return 0;memcpy(b,"Version=1\n",10);uint32_t c=stock_crc(b,10);for(int i=0;i<4;i++)b[10+i]=(unsigned char)(c>>(8*i));*n=14;return 1;}*n=fread(b,1,STOCK_FAV_BYTES,f);int more=fgetc(f);fclose(f);if(more!=EOF||*n<14||memcmp(b,"Version=1\n",10))return 0;uint32_t c=stock_crc(b,*n-4),tail=(uint32_t)b[*n-4]|(uint32_t)b[*n-3]<<8|(uint32_t)b[*n-2]<<16|(uint32_t)b[*n-1]<<24;return c==tail;}
static int stock_component(const char*s,int nested){
 if(!*s||*s=='/'||strpbrk(s,":\\\r\n"))return 0;
 const char*p=s;while(*p){const char*e=strchr(p,'/');size_t n=e?(size_t)(e-p):strlen(p);if(!n||(n==1&&p[0]=='.')||(n==2&&p[0]=='.'&&p[1]=='.'))return 0;if(!e)break;if(!nested)return 0;p=e+1;}return 1;
}
static int stock_row(const char*line,char*path,int *ordinal){char name[256],system[64],sub[256];unsigned type,flag;int index,used=0;
 if(sscanf(line,"%255[^:]:%63[^:]:%255[^:]:%u:%d:%u%n",name,system,sub,&type,&index,&flag,&used)!=6||line[used]||type>3||flag>1||!stock_component(name,0)||!stock_component(system,0)||(flag&&!stock_component(sub,1)))return 0;
 char dir[PATHLEN],parent[PATHLEN];if(!join(dir,sizeof dir,roots[type&1],system))return 0;
 if(flag){if(!join(parent,sizeof parent,dir,sub))return 0;}else strcopy(parent,dir,sizeof parent);
 if(!join(path,PATHLEN,parent,name))return 0;*ordinal=index;return 1;
}
static int stock_parts(const char*path,char*system,char*sub,const char**name,int*card){
 *card=card_for(path);if(*card<0)return 0;const char*rel=path+strlen(roots[*card])+1,*first=strchr(rel,'/'),*last=strrchr(rel,'/');
 if(!first||first-rel>=64||!stock_component(rel,1)||strlen(last+1)>255)return 0;
 memcpy(system,rel,first-rel);system[first-rel]=0;*name=last+1;
 if(first==last)strcpy(sub,"nul");else{size_t n=last-first-1;if(n>255)return 0;memcpy(sub,first+1,n);sub[n]=0;}return 1;
}
static int stock_route_type(const char*system,int card){if(launch_route_override)return card+(launch_route_override==1?0:2);char n[PATHLEN],path[PATHLEN],mode[32]={0};snprintf(n,sizeof n,"state/launch-modes/%s.txt",system);join(path,sizeof path,base,n);FILE*f=fopen(path,"r");if(f){fgets(mode,sizeof mode,f);fclose(f);}return card+(!strncmp(mode,"gameroom",8)?0:2);}
static void stock_import(void){
 strcopy(stock_file,"/mnt/data/misc/.favorite",sizeof stock_file);
 const char*test=getenv("DS_STYLE_STOCK_FAV_TEST");if(preview){if(!test)return;strcopy(stock_file,test,sizeof stock_file);}
 if(!stock_read(stock_original,&stock_size)){stock_size=0;return;}
 size_t begin=10;for(size_t i=10;i<stock_size-4;i++)if(stock_original[i]=='\n'){size_t n=i-begin;char row[1024],path[PATHLEN];int index;if(n<sizeof row){memcpy(row,stock_original+begin,n);row[n]=0;if(stock_row(row,path,&index)&&exists(path)&&favindex(path)<0&&favcount<MAX_FAV)strcopy(favourites[favcount++],path,PATHLEN);}begin=i+1;}
}
static void save_favourites(void){
 if(!stock_size){savelines("favourites.txt",favourites,favcount);return;}
 char(*local)[PATHLEN]=calloc(MAX_FAV,PATHLEN);if(!local)return;int count_local=0;
 for(int k=0;k<favcount;k++){int shared=0;size_t begin=10;for(size_t i=10;i<stock_size-4;i++)if(stock_original[i]=='\n'){char row[1024],resolved[PATHLEN];int ordinal;size_t n=i-begin;if(n<sizeof row){memcpy(row,stock_original+begin,n);row[n]=0;if(stock_row(row,resolved,&ordinal)&&!strcmp(resolved,favourites[k]))shared=1;}begin=i+1;}if(!shared)strcopy(local[count_local++],favourites[k],PATHLEN);}
 savelines("favourites.txt",local,count_local);free(local);
}
/* 1 shared write; 0 not a verified writable stock entry; -1 conflicting change.
 * Local favourites remain usable when stock's format or record is unsupported.
 */
static int stock_toggle(const char*path,int remove_entry){
 if(!stock_size)return 0;int card=card_for(path);if(card<0)return 0;
 char system[64],sub[256];const char*name;if(!stock_parts(path,system,sub,&name,&card))return 0;
 unsigned char*current=malloc(STOCK_FAV_BYTES),*out=malloc(STOCK_FAV_BYTES);if(!current||!out){free(current);free(out);return -1;}
 size_t size;if(!stock_read(current,&size)||size!=stock_size||memcmp(current,stock_original,size)){free(current);free(out);return -1;}
 memcpy(out,"Version=1\n",10);size_t length=10,begin=10;int removed=0,rows=0;
 for(size_t i=10;i<size-4;i++)if(current[i]=='\n'){size_t n=i-begin;char row[1024]={0},resolved[PATHLEN];int index=0,match=0;if(n<sizeof row){memcpy(row,current+begin,n);row[n]=0;if(stock_row(row,resolved,&index)){match=!strcmp(path,resolved);}}if(!index){if(length+n+5>=STOCK_FAV_BYTES){free(current);free(out);return -1;}memcpy(out+length,current+begin,n+1);length+=n+1;rows++;begin=i+1;continue;}rows++;if(match&&remove_entry)removed++;else{char*last=strrchr(row,':');*last=0;char*order=strrchr(row,':');*order=0;int written=snprintf((char*)out+length,STOCK_FAV_BYTES-length,"%s:%d:%s\n",row,rows-removed,last+1);if(written<0||length+(size_t)written+4>=STOCK_FAV_BYTES){free(current);free(out);return -1;}length+=(size_t)written;}begin=i+1;}
 if(begin!=size-4||(!remove_entry&&rows>=300)||(remove_entry&&!removed)){free(current);free(out);return 0;}
 if(!remove_entry){int n=snprintf((char*)out+length,STOCK_FAV_BYTES-length,"%s:%s:%s:%d:%d:%d\n",name,system,sub,stock_route_type(system,card),rows+1,strcmp(sub,"nul")!=0);if(n<0||length+(size_t)n+4>=STOCK_FAV_BYTES){free(current);free(out);return -1;}length+=(size_t)n;}
 uint32_t crc=stock_crc(out,length);for(int i=0;i<4;i++)out[length++]=(unsigned char)(crc>>(i*8));
 char backup[PATHLEN],tmp[PATHLEN];statepath(backup,"stock-favourites-before.bin");snprintf(tmp,sizeof tmp,"%s.dsstyle.tmp",stock_file);
 FILE*f=fopen(backup,"wb");int ok=0;if(f){ok=fwrite(current,1,size,f)==size;if(fclose(f))ok=0;}
 if(ok){f=fopen(tmp,"wb");ok=f!=NULL;if(f){ok=fwrite(out,1,length,f)==length;
#ifndef _WIN32
 if(fflush(f)||fsync(fileno(f)))ok=0;
#endif
 if(fclose(f))ok=0;}}
 if(ok){size_t check;ok=stock_read(current,&check)&&check==stock_size&&!memcmp(current,stock_original,check);}
 if(ok){
#ifdef _WIN32
 ok=MoveFileExA(tmp,stock_file,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
 ok=rename(tmp,stock_file)==0;
#endif
 }
 if(ok){memcpy(stock_original,out,length);stock_size=length;}else remove(tmp);
 free(current);free(out);return ok?1:-1;
}
