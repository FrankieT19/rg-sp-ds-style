/* File-backed PCM for testing the actual worker/mixer without audio hardware. */
struct pcm { FILE *out; int open, prepared; };
static struct pcm fake_pcm;
struct pcm *pcm_open(unsigned int card,unsigned int device,unsigned int flags,const struct pcm_config *config){
 (void)flags;if(card||device||config->channels!=2||config->rate!=48000)exit(20);
 fake_pcm.out=fopen(getenv("AUDIO_CAPTURE"),"wb");fake_pcm.open=1;return &fake_pcm;
}
int pcm_is_ready(const struct pcm *p){return p->out!=NULL;}
const char *pcm_get_error(const struct pcm *p){(void)p;return "mock PCM";}
int pcm_prepare(struct pcm *p){p->prepared++;return 0;}
int pcm_writei(struct pcm *p,const void *data,unsigned int count){return fwrite(data,4,count,p->out)==count?(int)count:-1;}
int pcm_close(struct pcm *p){if(!p->open)exit(21);p->open=0;return fclose(p->out);}
static ssize_t mock_read(int fd,void *data,size_t bytes){
 (void)fd;static int sent;if(sent++)return 0;
 const char *events=getenv("AUDIO_EVENTS");if(!events)events="4";size_t n=strlen(events);if(n>bytes)n=bytes;
 for(size_t i=0;i<n;i++)((unsigned char*)data)[i]=(unsigned char)(events[i]-'0');return (ssize_t)n;
}
#define read mock_read
