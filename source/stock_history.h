/* Stock history uses Version=1, filename:system:subpath:type:subdir and the
   same zero-seed CRC32 trailer as favourites. Newest record comes first. */
static int history_read(const char*path,unsigned char*b,size_t*n){FILE*f=fopen(path,"rb");if(!f){if(errno!=ENOENT)return 0;memcpy(b,"Version=1\n",10);uint32_t c=stock_crc(b,10);for(int i=0;i<4;i++)b[10+i]=(unsigned char)(c>>(8*i));*n=14;return 1;}*n=fread(b,1,STOCK_FAV_BYTES,f);int ok=!ferror(f)&&fgetc(f)==EOF;fclose(f);if(!ok||*n<14||memcmp(b,"Version=1\n",10))return 0;uint32_t c=stock_crc(b,*n-4);for(int i=0;i<4;i++)if(b[*n-4+i]!=(unsigned char)(c>>(i*8)))return 0;return 1;}
static void history_path(char*p){strcopy(p,"/mnt/data/misc/.history",PATHLEN);if(preview){const char*t=getenv("DS_STYLE_STOCK_HISTORY_TEST");strcopy(p,t?t:"",PATHLEN);}}
static int history_row(const char*line,char*path){char name[256],system[64],sub[256],row[1024];unsigned type,flag;int used=0;if(sscanf(line,"%255[^:]:%63[^:]:%255[^:]:%u:%u%n",name,system,sub,&type,&flag,&used)!=5||line[used])return 0;snprintf(row,sizeof row,"%s:%s:%s:%u:1:%u",name,system,sub,type,flag);int order;return stock_row(row,path,&order);}
static void history_import(void){char file[PATHLEN];history_path(file);if(!*file)return;unsigned char*b=malloc(STOCK_FAV_BYTES);size_t size;if(!b)return;if(history_read(file,b,&size)){recentcount=0;size_t begin=10;for(size_t i=10;i<size-4;i++)if(b[i]=='\n'){char row[1024],path[PATHLEN];size_t n=i-begin;if(n<sizeof row){memcpy(row,b+begin,n);row[n]=0;if(history_row(row,path)&&exists(path)&&recentcount<50){int found=0;for(int j=0;j<recentcount;j++)if(!strcmp(recents[j],path))found=1;if(!found)strcopy(recents[recentcount++],path,PATHLEN);}}begin=i+1;}}free(b);}
static int history_add(const char*path){char file[PATHLEN],system[64],sub[256];const char*name;int card;history_path(file);if(!*file||!stock_parts(path,system,sub,&name,&card))return 0;
 unsigned char*b=malloc(STOCK_FAV_BYTES),*out=malloc(STOCK_FAV_BYTES),*check=malloc(STOCK_FAV_BYTES);if(!b||!out||!check){free(b);free(out);free(check);return 0;}
 size_t size;int ok=history_read(file,b,&size);size_t len=10,begin=10;int rows=1;if(!ok)goto done;memcpy(out,"Version=1\n",10);
 len+=snprintf((char*)out+len,STOCK_FAV_BYTES-len,"%s:%s:%s:%d:%d\n",name,system,sub,stock_route_type(system,card),strcmp(sub,"nul")!=0);
 for(size_t i=10;i<size-4;i++)if(b[i]=='\n'){size_t n=i-begin;char row[1024],resolved[PATHLEN];int match=0;if(n<sizeof row){memcpy(row,b+begin,n);row[n]=0;match=history_row(row,resolved)&&!strcmp(path,resolved);}if(!match&&rows<10){if(len+n+5>=STOCK_FAV_BYTES){ok=0;goto done;}memcpy(out+len,b+begin,n+1);len+=n+1;rows++;}begin=i+1;}
 if(begin!=size-4){ok=0;goto done;}uint32_t crc=stock_crc(out,len);for(int i=0;i<4;i++)out[len++]=(unsigned char)(crc>>(8*i));
 char backup[PATHLEN],tmp[PATHLEN];statepath(backup,"stock-history-before.bin");snprintf(tmp,sizeof tmp,"%s.dsstyle.tmp",file);FILE*f=fopen(backup,"wb");ok=f!=NULL;if(f){ok=fwrite(b,1,size,f)==size;if(fclose(f))ok=0;}if(!ok)goto done;
 f=fopen(tmp,"wb");ok=f!=NULL;if(f){ok=fwrite(out,1,len,f)==len;
#ifndef _WIN32
 if(fflush(f)||fsync(fileno(f)))ok=0;
#endif
 if(fclose(f))ok=0;}size_t checked;if(ok)ok=history_read(file,check,&checked)&&checked==size&&!memcmp(check,b,size);
 if(ok){
#ifdef _WIN32
 ok=MoveFileExA(tmp,file,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
 ok=rename(tmp,file)==0;
#endif
 }if(!ok)remove(tmp);
 done:free(b);free(out);free(check);return ok;
}
static void shared_refresh(void){char selected[PATHLEN]={0};if(home_favourites&&home_recent<favcount)strcopy(selected,favourites[home_recent],sizeof selected);favcount=0;loadlines("favourites.txt",favourites,&favcount,MAX_FAV);stock_import();history_import();home_recent=0;if(*selected)for(int i=0;i<favcount;i++)if(!strcmp(selected,favourites[i])){home_recent=i;break;}}

static int shared_route(const char*path,int history){unsigned char*allocated=NULL,*data=stock_original;size_t size=stock_size;if(history){char file[PATHLEN];history_path(file);if(!*file)return 0;allocated=malloc(STOCK_FAV_BYTES);if(!allocated)return 0;if(!history_read(file,allocated,&size)){free(allocated);return 0;}data=allocated;}int route=0;size_t begin=10;for(size_t i=10;i+4<size;i++)if(data[i]=='\n'){size_t n=i-begin;char row[1024],resolved[PATHLEN];int ordinal;if(n<sizeof row){memcpy(row,data+begin,n);row[n]=0;int valid=history?history_row(row,resolved):stock_row(row,resolved,&ordinal);if(valid&&!strcmp(resolved,path)){char*c=row;for(int j=0;j<3;j++)c=strchr(c,':')+1;route=(atoi(c)&2)?2:1;break;}}begin=i+1;}free(allocated);return route;}
