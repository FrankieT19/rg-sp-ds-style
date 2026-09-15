static int input_profile[16],input_node[16],input_trigger[16][2];static uint64_t input_scan_at;
static void controller_discover(void){
 input_scan_at=millis();
 DIR*nodes=opendir("/dev/input");if(!nodes)return;struct dirent*entry;
 while((entry=readdir(nodes))){int node;char suffix;if(sscanf(entry->d_name,"event%d%c",&node,&suffix)!=1||node<0||node>4095)continue;int found=0,slot=-1;for(int i=0;i<16;i++){if(input[i]>=0&&input_node[i]==node)found=1;if(input[i]<0&&slot<0)slot=i;}if(found||slot<0)continue;
 char path[80];snprintf(path,sizeof path,"/dev/input/event%d",node);int fd=open(path,O_RDONLY|O_NONBLOCK|O_CLOEXEC);if(fd<0)continue;
 unsigned long bits[(KEY_MAX+8*sizeof(long))/(8*sizeof(long))]={0};if(ioctl(fd,EVIOCGBIT(EV_KEY,sizeof bits),bits)<0){close(fd);continue;}
 int useful=0;int check[]={304,288,103,544,KEY_VOLUMEUP,312};for(unsigned j=0;j<sizeof check/sizeof*check;j++)if(bits[check[j]/(8*sizeof(long))]&(1ul<<(check[j]%(8*sizeof(long)))))useful=1;if(!useful){close(fd);continue;}
 struct input_id id={0};ioctl(fd,EVIOCGID,&id);int external=id.bustype==BUS_USB||id.bustype==BUS_BLUETOOTH;int profile=0;
 char name[80]={0};ioctl(fd,EVIOCGNAME(sizeof name),name);if(external){unsigned hash=2166136261u;unsigned char*bytes=(unsigned char*)&id;for(unsigned j=0;j<sizeof id;j++)hash=(hash^bytes[j])*16777619u;for(unsigned j=0;name[j];j++)hash=(hash^(unsigned char)name[j])*16777619u;if(!hash)hash=1;
 for(profile=1;profile<controller_count;profile++)if(controllers[profile].id==hash)break;
 if(profile==controller_count){if(controller_count==CONTROL_PROFILES){close(fd);continue;}controller_count++;controllers[profile].id=hash;controllers[profile].external=1;controller_defaults(profile);}
 strcopy(controllers[profile].name,name,sizeof controllers[profile].name);
 memset(controllers[profile].available,0,sizeof controllers[profile].available);for(int c=256;c<544;c++)controllers[profile].available[c]=(bits[c/(8*sizeof(long))]>>(c%(8*sizeof(long))))&1;
 struct input_absinfo axis;if(ioctl(fd,EVIOCGABS(ABS_Z),&axis)>=0&&axis.maximum>axis.minimum)controllers[profile].available[312]=1;if(ioctl(fd,EVIOCGABS(ABS_RZ),&axis)>=0&&axis.maximum>axis.minimum)controllers[profile].available[313]=1;
 /* A stale mapping from another driver must not strand Accept. */
 int accept=controllers[profile].code[0];if(!controllers[profile].available[accept])controller_defaults(profile);
 }
 input[slot]=fd;input_node[slot]=node;input_profile[slot]=profile;input_trigger[slot][0]=input_trigger[slot][1]=0;
 }
 closedir(nodes);
}
static int startup_held(int code){int valid=0;for(int i=0;i<16;i++)if(input[i]>=0&&input_profile[i]==0){unsigned char capabilities[(KEY_MAX+8)/8]={0},keys[(KEY_MAX+8)/8]={0};if(ioctl(input[i],EVIOCGBIT(EV_KEY,sizeof capabilities),capabilities)<0||!(capabilities[311/8]&(1u<<(311%8))))continue;if(ioctl(input[i],EVIOCGKEY(sizeof keys),keys)<0)continue;valid=1;if(keys[code/8]&(1u<<(code%8)))return 1;}return valid?0:-1;}
static int controller_axis(int fd,struct input_event*e){if(e->code==ABS_HAT0X||e->code==ABS_HAT0Y)return e->value;struct input_absinfo a;if(ioctl(fd,EVIOCGABS(e->code),&a)<0||a.maximum<=a.minimum)return 0;long long center=((long long)a.minimum+a.maximum)/2,dead=((long long)a.maximum-a.minimum)/4;return e->value<center-dead?-1:e->value>center+dead?1:0;}
