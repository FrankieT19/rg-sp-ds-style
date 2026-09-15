/* Keep the already-working stock tinyplay process open across navigation.
 * A WAV FIFO supplies continuous PCM; silence between events avoids repeated
 * codec startup, and a full silent tail prevents short samples being dropped. */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
enum { PCM_OUT=0,PCM_FORMAT_S16_LE=0 };
struct pcm_config {unsigned channels,rate,period_size,period_count,format;};
struct pcm {int fd;pid_t child;char dir[96],fifo[120];};
static const char*pcm_get_error(struct pcm*p){(void)p;return strerror(errno);}
static int pcm_is_ready(struct pcm*p){return p&&p->fd>=0;}
static int pcm_prepare(struct pcm*p){(void)p;return -1;}
static int stream_write(int fd,const void*data,size_t bytes){const char*p=data;while(bytes){ssize_t n=write(fd,p,bytes);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;p+=n;bytes-=n;}return 0;}
static struct pcm*pcm_open(unsigned card,unsigned device,int flags,const struct pcm_config*cfg){
 (void)card;(void)device;(void)flags;(void)cfg;struct pcm*p=calloc(1,sizeof *p);if(!p)return NULL;p->fd=-1;p->child=-1;
 strcpy(p->dir,"/tmp/dsstyle-pcm-XXXXXX");if(!mkdtemp(p->dir))return p;snprintf(p->fifo,sizeof p->fifo,"%s/stream.wav",p->dir);if(mkfifo(p->fifo,0600))return p;
 signal(SIGPIPE,SIG_IGN);p->child=fork();if(!p->child){execl("/mnt/vendor/bin/tinyplay","tinyplay",p->fifo,"-p","256","-n","4",(char*)NULL);_exit(127);}if(p->child<0)return p;
 for(int i=0;i<200;i++){p->fd=open(p->fifo,O_WRONLY|O_NONBLOCK);if(p->fd>=0)break;if(waitpid(p->child,NULL,WNOHANG)==p->child){p->child=-1;break;}struct timespec t={0,10000000};nanosleep(&t,NULL);}
 if(p->fd<0)return p;
#ifdef F_SETPIPE_SZ
 fcntl(p->fd,F_SETPIPE_SZ,4096);
#endif
 fcntl(p->fd,F_SETFL,fcntl(p->fd,F_GETFL)&~O_NONBLOCK);
 unsigned char h[44]={'R','I','F','F',0xff,0xff,0xff,0x7f,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,2,0,0x80,0xbb,0,0,0,0xee,2,0,4,0,16,0,'d','a','t','a',0xdb,0xff,0xff,0x7f};
 if(stream_write(p->fd,h,sizeof h)){close(p->fd);p->fd=-1;}return p;
}
static int pcm_writei(struct pcm*p,const void*data,unsigned frames){return stream_write(p->fd,data,(size_t)frames*4)?-1:(int)frames;}
static void pcm_close(struct pcm*p){if(!p)return;if(p->fd>=0)close(p->fd);if(p->child>0){int done=0;for(int i=0;i<200;i++){if(waitpid(p->child,NULL,WNOHANG)==p->child){done=1;break;}struct timespec t={0,10000000};nanosleep(&t,NULL);}if(!done){kill(p->child,SIGTERM);waitpid(p->child,NULL,0);}}if(*p->fifo)unlink(p->fifo);if(*p->dir)rmdir(p->dir);free(p);}
