/* Stock dmenu_attr: 136-byte payload + CRC32 (initial zero, no final xor).
 * Writes preserve every byte except brightness (24), volume (36), and CRC.
 * The caller must check the captured stock-code signature before writing. */
static unsigned attr_u32(const unsigned char*p){return p[0]|p[1]<<8|p[2]<<16|(unsigned)p[3]<<24;}
static void attr_put(unsigned char*p,unsigned v){for(int i=0;i<4;i++)p[i]=(unsigned char)(v>>(i*8));}
static unsigned attr_crc(const unsigned char*p,size_t n){unsigned c=0;while(n--){c^=*p++;for(int i=0;i<8;i++)c=(c>>1)^((c&1)?0xedb88320u:0);}return c;}
static int attr_read(const char*path,unsigned char data[140]){FILE*f=fopen(path,"rb");if(!f)return 0;size_t n=fread(data,1,140,f);int extra=fgetc(f);fclose(f);return n==140&&extra==EOF&&attr_crc(data,136)==attr_u32(data+136)&&attr_u32(data+24)<=6&&attr_u32(data+36)<=10;}
