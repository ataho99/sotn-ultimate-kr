// unecm (Neill Corlett ECM format) — LOGO 단계용. 검증 = 결과 sha1 대조.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
static uint8_t ecc_f[256], ecc_bl[256]; static uint32_t edc_t[256];
static void init(void){for(int i=0;i<256;i++){int j=(i<<1)^((i&0x80)?0x11D:0);ecc_f[i]=j;ecc_bl[i^j]=i;uint32_t e=i;for(int k=0;k<8;k++)e=(e>>1)^((e&1)?0xD8018001u:0);edc_t[i]=e;}}
static uint32_t edc(const uint8_t*s,int n){uint32_t e=0;while(n--)e=(e>>8)^edc_t[(e^*s++)&0xff];return e;}
static void eblock(const uint8_t*src,int maj,int min,int mm,int mi,uint8_t*dst){int size=maj*min;for(int m=0;m<maj;m++){int idx=(m>>1)*mm+(m&1);uint8_t a=0,b=0;for(int n=0;n<min;n++){uint8_t t=src[idx];idx+=mi;if(idx>=size)idx-=size;a^=t;b^=t;a=ecc_f[a];}a=ecc_bl[ecc_f[a]^b];dst[m]=a;dst[m+maj]=a^b;}}
static void ecc(uint8_t*s,int zero){uint8_t sv[4];if(zero){memcpy(sv,s+12,4);memset(s+12,0,4);}eblock(s+12,86,24,2,86,s+0x81C);eblock(s+12,52,43,86,88,s+0x8C8);if(zero)memcpy(s+12,sv,4);}
static void p32(uint8_t*d,uint32_t v){d[0]=v;d[1]=v>>8;d[2]=v>>16;d[3]=v>>24;}
int main(int c,char**v){if(c<3){fprintf(stderr,"usage: unecm in.ecm out.bin\n");return 2;}init();FILE*in=fopen(v[1],"rb"),*out=fopen(v[2],"wb");if(!in||!out){perror("open");return 1;}
 uint8_t h[4];if(fread(h,1,4,in)!=4||memcmp(h,"ECM\0",4)){fprintf(stderr,"not ECM\n");return 1;}
 static uint8_t buf[1<<16];uint8_t s[2352];long long nsec=0;
 for(;;){int ch=fgetc(in);if(ch==EOF){fprintf(stderr,"EOF early\n");return 1;}int type=ch&3;uint32_t num=(ch>>2)&0x1f;int bits=5;while(ch&0x80){ch=fgetc(in);num|=(uint32_t)(ch&0x7f)<<bits;bits+=7;}
  if(num==0xFFFFFFFFu)break;num++;
  if(type==0){while(num){uint32_t n=num>sizeof buf?sizeof buf:num;if(fread(buf,1,n,in)!=n){fprintf(stderr,"short raw\n");return 1;}fwrite(buf,1,n,out);num-=n;}continue;}
  while(num--){memset(s,0,2352);s[0]=0;memset(s+1,0xFF,10);s[11]=0;
   if(type==1){if(fread(s+12,1,3,in)!=3)return 1;s[15]=1;if(fread(s+16,1,0x800,in)!=0x800)return 1;p32(s+0x810,edc(s,0x810));ecc(s,0);}
   else if(type==2){s[15]=2;if(fread(s+20,1,0x804,in)!=0x804)return 1;memcpy(s+16,s+20,4);p32(s+0x818,edc(s+16,0x808));ecc(s,1);fwrite(s+16,1,2336,out);nsec++;continue;}
   else if(type==3){s[15]=2;if(fread(s+20,1,0x918,in)!=0x918)return 1;memcpy(s+16,s+20,4);p32(s+0x92C,edc(s+16,0x91C));fwrite(s+16,1,2336,out);nsec++;continue;}
   else{fprintf(stderr,"bad type\n");return 1;}
   fwrite(s,1,2352,out);nsec++;}}
 fclose(out);fprintf(stderr,"done sectors(ecm)=%lld\n",nsec);return 0;}
