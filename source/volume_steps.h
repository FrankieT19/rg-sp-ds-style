/* Stock keeps 0..10; private UI half-steps never enter stock files. */
static int volume_coarse(int step){return (step+1)/2;}
static int volume_saved(const char*base,int stock){
 char path[2048];snprintf(path,sizeof path,"%s/state/volume-step.txt",base);FILE*f=fopen(path,"r");int half=-1,last=-1;
 if(f){if(fscanf(f,"%d %d",&half,&last)!=2)half=-1;fclose(f);}
 return half>=0&&half<=20&&last==stock&&volume_coarse(half)==stock?half:stock*2;
}
