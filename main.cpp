
#include "Gdl.h"
#include "PRI.S3M.h"
#include "windows.h"

#define  u8 unsigned char
#define  s8  signed  char
#define u16 unsigned short
#define s16  signed  short
#define u32 unsigned int
#define s32  signed  int

#include "SoundServer.h"

const char *adlibType[] = { "","","amel","abd","asnare","atom","acym","ahihat" } ;
const char      *note[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" } ;
     // int      period[] = { 1712,1616,1524,1440,1356,1280,1208,1140,1076,1016,960,907 } ;
      u8 emptyraw[32*5] = { 255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                            255,0,255,255,0,255,0,255,255,0,
                          } ;


// power of pointer !

struct sample
{  u8  *offset, *data ;
   u8  *flag,   *volume, *packed, *name, *fname ;
   u8  *looped, *stereo, *in16bits ;
   u16 *hrz16 ;
   u32 *size, *check, *loopEnd, *loopStart, adData ; // adData for looped, stereo and in16bits
};

struct sample * loadSample(u8*sample, u8*s3m)
{ u8 *s = sample ; u32 *is = (u32*)(s+16) ;
  struct sample *spl = (struct sample*)malloc(sizeof(struct sample)) ;
         memset(spl,0,sizeof(struct sample)) ; addFreeEntry(spl) ;
  if(*s > 1) { free(spl) ; return 0 ; }
  spl->offset    = s   ;
  spl->fname     = s+1 ;
  spl->name      = s+16*3  ;
  spl->volume    = s+16+12 ;
  spl->packed    = s+16+14 ;
  spl->flag      = s+16+15 ;
  spl->check     = (u32*)(s+16*3+28) ;
  if(*s != 1 || *(spl->check) != 0x53524353) { *(spl->flag) = 142 ; return spl ; } // empty/bad or unsuported sample
  spl->size      = is++ ;
  spl->loopStart = is++ ;
  spl->loopEnd   = is   ;
   { u8 flag = *(spl->flag) ;
     u8   *d = (u8*)&(spl->adData)  ;
     *d   = flag&1 ; // looped ?
     d[1] = flag&2 ; // stereo ?
     d[2] = flag&4 ; // 16 bit ?
     spl->looped   = d   ;
     spl->stereo   = ++d ;
     spl->in16bits = ++d ;
   }
   spl->hrz16 = (u16*)(s+32) ;

  { u32 z = *(s+14) + ((*(s+13))<<8) + ((*(s+15))<<16) ; //*(short*)(s+14) + ((*(s+13)) << 16) ;// printf("\n z : %x | %ik",z,z>>10) ;
    z/=16 ;
    if(z > 1024*1024) printf("sample error !!!") ;
    spl->data = s3m + z ;
  } return spl ;
}

struct chn
{

};

struct s3m
{ u8  *file ;
  u32 adData ; // adData for mstVol & stereo
  u16 *orderNb, *sampleNb, *ptnNb, *flag, *version ;
  u8  *glbVol, *mstVol, *iSpeed, *iTempo, *stereo, *fchSetting, *chnNb, *order, **ptn ;
  struct chn ** chn ;
  struct sample ** sample ;
  u8  *songName, *tracker, *comment ;
  u8  *chnSwitch ;
  s8  *chnSetting ;
};

struct s3m * loadS3m(u8 *song)
{  struct s3m s3m, *out ; u8 *f = song ; int i ;
   s3m.file = s3m.songName = f ;
   f+=28 ; if(*f++ != 0x1a) return 0 ; // not an s3m
           if(*f != 16 && *f != 17) return 0 ;
   f+=3 ;  s3m.orderNb  = (u16*)f ;
   f+=2 ;  s3m.sampleNb = (u16*)f ;
   f+=2 ;  s3m.ptnNb    = (u16*)f ;
   f+=2 ;  s3m.flag     = (u16*)f ;
   f+=2 ;  s3m.tracker  = (u8*)malloc(42) ; addFreeEntry(s3m.tracker) ;
           { int tr4k3r = *(u16*)f;
             if(tr4k3r>>12 == 0x1) sprintf((char*)s3m.tracker,"ScreamTracker") ;
             else           sprintf((char*)s3m.tracker,"unknow (%x)",tr4k3r>>12);
             tr4k3r&=0xfff ;
             sprintf((char*)s3m.tracker,"%s v%x.%x",s3m.tracker,tr4k3r>>8,tr4k3r&0xff);
           }
   f+=2 ;  s3m.version  = (u16*)f ;
   f+=2 ;  if(*(int*)f != 0x4d524353){ free(s3m.tracker) ; return 0 ; } // not an s3m
   f+=4 ;  s3m.glbVol   = f++ ;
           s3m.iSpeed   = f++ ;
           s3m.iTempo   = f++ ;
           { u8* p = (u8*)(&(s3m.adData)) ;
             p[0] = (u8)((*f)&0x7f) ;
             p[1] = (u8)((*f)>>7) ;
           }
   f+=11;  if(*(u16*)f) s3m.comment = s3m.file + (*(u16*)f)*16 ;  // <- fix here !
           else         s3m.comment = 0 ;
   f+=2 ;  s3m.fchSetting = f ;    // read channels
           s3m.chnSetting = (s8*)malloc(32*4) ; addFreeEntry(s3m.chnSetting) ;
          //printf("\n 255>>7 %i, 128>>7 %i",255>>7,128>>7);
          s3m.chnSwitch = (u8*)malloc(32) ; addFreeEntry(s3m.chnSwitch) ;
          {  u8*p = (u8*)(&(s3m.adData)) ; p+=2 ; s3m.chnNb = p ; *p=0 ;
             for(i=0;i<32;i++)
             {  int ch = *f++ ;
                 if(ch>>7) { (s3m.chnSetting)[i] = 127 ; (s3m.chnSetting)[i+32] = -1 ; (s3m.chnSwitch)[i] = 255 ; } // unactive or unabled channel
                  else
                   { s8 *s = s3m.chnSetting + i ;
                     if(ch<8) { *s = -1 ; *(s+32) = ch+1 ; } // left
                     else if(ch<16) { *s = 1 ; *(s+32) = ch-7 ; } // right
                          else if(ch<32) { *s = 0  ; *(s+32) = ch-15 ; } // adlib chn
                               else     { *s = 42 ; *(s+32) = -1 ; continue ; }    // error <- maybe can try a fix here !
                     (s3m.chnSwitch)[i] = (*p)++ ;
                     printf("\nchn %i -> %i",i,(s3m.chnSwitch)[i]) ;
                   }
             };
             printf("\n%i channels",*s3m.chnNb) ;
          }
           { u8 n=0 ; i = *(s3m.orderNb) ;
             s3m.order = (u8*)malloc(i+42) ; addFreeEntry(s3m.order) ;
             while(*f != 255 && i--)
             {   while(*f == 254)  f++ ;
                 s3m.order[n++] = *f++ ;
             };  if(*f == 255) { f++ ; --i ; } else --i ; if(i>0) f+=i ;
                 s3m.order[n] = 0xff ;
           }
           
           {  u16 *p = (u16*)f ; struct sample *s ; u8 *off ; u8 valid=0 ;
              f += (*s3m.sampleNb)*2 ; //off = (u8**)malloc(s3m.sampleNb*4) ;
              s3m.sample = (struct sample **)malloc(4*(*s3m.sampleNb)) ;
              addFreeEntry(s3m.sample) ;
             for(i=0;i<*s3m.sampleNb;i++)
              { off = (*p++)*16 + s3m.file ;
                switch(*off)
                 { case 0 : s3m.sample[i] = loadSample(off, song) ; break ;
                   case 1 : s3m.sample[i] = loadSample(off, song) ; break ;
                   case 2 : printf("adlib melody [%s] .. unsuported yet\n",adlibType[2]) ;
                            s3m.sample[i] = 0 ;
                   break ;
                   default : s3m.sample[i] = 0 ;
                             if(*off < 8 && *off > 2)
                                  printf("adlib drum [%s] .. unsuported yet\n",adlibType[*off]) ;
                             else printf("%i | -> sample error !\n",*off) ;

                   break ;
                 };
              };

              s3m.ptn = (u8**)malloc(4*(*s3m.ptnNb)) ;  addFreeEntry(s3m.ptn) ;
              memset(s3m.ptn,0,4*(*s3m.ptnNb)) ; i=0 ;
              
    while(s3m.order[i] < 99)
	{   u8 nb = s3m.order[i] ;
        if(s3m.ptn[nb] == 0)
	              { printf("\n read pttn %i\t",nb) ;
	                s3m.ptn[nb] = (u8*)malloc(64*(*s3m.chnNb)*5) ;
                    addFreeEntry(s3m.ptn[nb]) ;
	                off = (p[ s3m.order[i] ])*16 + song ;
		                 { u8 *ptr = off+2, *o = s3m.ptn[nb], *chn=o, v ; int size = *(u16*)off, nb=0 ;
		                   memcpy(o,emptyraw,32*5); printf(" offset %x\tsize %i\t",off-song,size) ;
		                   //system("pause") ;
		                   while(/*size - (ptr-off) && */nb < 64)
		                   { v = *ptr++ ;
		                     if(!v) {// printf("\nend of raw %i .. size - (ptr-off) = %i",nb++,size - (ptr-off)) ;
		                               if(nb++ != 63){ o+=(*s3m.chnNb)*5 ; memcpy(o,emptyraw,(*s3m.chnNb)*5); }
                                       else          { int z = size - (ptr-off) ;
                                                       printf(" ..  ") ;
                                                       // in "Mailbox-on-NES" z= -2, what is the tracker ?
                                                       if(z && z!=-2) printf("error ! size : %i",size - (ptr-off)) ;
                                                       else           printf("ok") ;
                                                     }
		                            }
		                     else { chn = o + ((s3m.chnSwitch)[v&31])*5 ;
		                            //printf("\n channel %i ",chnb) ;
		                            if(v&32) { //printf(" note : %s-%i %x",note[(*ptr)&0x0f],(*ptr)>>4,ptr[1]) ;
		                                       *chn = *ptr++ ;  // copy note
		                                       *(chn+1) = *ptr++ ; // and sample nb
		                                     }
		                            if(v&64) { //printf(" volume %x",*ptr) ;
		                                       *(chn+2) = *ptr++ ; // copy volume value
		                                     }
		                            if(v&128){ //printf(" command %x info %x",*ptr,ptr[1]) ;
		                                       *(chn+3) = *ptr++ ;  // copy command nb
		                                       *(chn+4) = *ptr++ ;  // and command arg
		                                     }
		                         }
		                   };
		                }  ++i ; //off+=2 ;
	               } else ++i ;
              };	//printf("\nout of read patterns.") ;
           }

   out = (struct s3m *)malloc(sizeof(struct s3m)) ;
   s3m.mstVol = (u8*)(&(out->adData)) ;
   s3m.stereo = (s3m.mstVol)+1 ;
   memcpy(out,&s3m,sizeof(struct s3m)) ;

   return out ;
}

u8 * sample001  ;
u32  sampleSize ;
u16  sampleHz   ;

	void	mySoundProc(void *pSoundBuffer,long bufferLen)
	{		// Convert params, assuming we create a 16bits, mono waveform.
            printf(".") ;
            if(sampleSize > bufferLen){ sampleSize = bufferLen ;
                                        printf("\n !!! out of buffer !!!") ;
                                      }
			//signed short *pSample = (signed short*)pSoundBuffer;
                u8* pSample = (u8*)pSoundBuffer ;
			int	nbSample = bufferLen ;// / sizeof(signed short);
	        int i=0 ;
            static int spli=0 ;
             while(i<nbSample)
             { pSample[i] = sample001[spli++] ;
               i++ ; if(spli >= sampleSize) spli=0 ;
             } ;

             return ;
	}

extern int REPLAY_RATE ;

void bin2h(const char *file, const char * name);

int main(int argc, char *argv[])
{   int *goblin ; struct s3m s3m, *ld ; int z=0 ;
    unsigned char *f ;  unsigned short *p ;
    int orderNum,insNum,patNum,tracker,version,i,*offset, x ;
    if(argv[1]){ goblin = (int*)loadFile(argv[1]) ; }
     else { goblin = (int*)_Goblin ;
            printf("\n * s3lib say you hello !\n\nput any s3m file as argument ..\n\n") ;
          }
    ld = loadS3m((u8*)goblin) ;
    if(ld==0) { printf("not an s3m\n\n") ; system("pause") ; addFreeEntry(0) ; return 0 ; }

    //bin2h("c:\\PRI.S3M", "goblin") ;

    memcpy(&s3m,ld,sizeof(struct s3m)) ;

            printf("\n songName\t\"%s\"",s3m.songName) ;
            printf("\n tracker\t %s",s3m.tracker) ;
            //if(s3m.comment) printf("\n comment\t %s",s3m.comment) ;
            printf("\n orderNb\t %i",*s3m.orderNb) ;
            printf("\n sampleNb\t %i",*s3m.sampleNb) ;
            printf("\n pttrnNb\t %i",*s3m.ptnNb) ;
            printf("\n flag\t\t %i",*s3m.flag) ;
            printf("\n version\t %i",*s3m.version) ;
            printf("\n glbVol\t\t %i",*s3m.glbVol) ;
            printf("\n mstVol\t\t %i",*s3m.mstVol) ;
            printf("\n iSpeed\t\t %i",*s3m.iSpeed) ;
            printf("\n iTempo\t\t %i",*s3m.iTempo) ;
            printf("\n stereo\t\t %i",*s3m.stereo) ;

            printf("\n order { %i",(s3m.order)[0]) ;

            while(++z < *s3m.orderNb && (s3m.order)[z] != 0xff)
               printf(",%i",(s3m.order)[z]) ; printf(" }") ;

            { struct sample *s ; z=0 ; printf("\n\n    samples (%i)",*s3m.sampleNb) ;
              while(z < *s3m.sampleNb) { s = s3m.sample[z++] ;
                                         if(s) { if(*(s->flag) != 142) //    printf(" empty") ; else
                                                 {      //if(z==33){
                                                        sample001  = s->data ;
                                                        sampleSize = *(s->size) ;
                                                        REPLAY_RATE = sampleHz = *(s->hrz16) ;
                                                        //}
                                                        printf("\n %i\t| ",z) ;
                                                        if(*(s->packed))   printf(" packed ") ;
                                                        else               printf("unpacked") ;
                                                        printf(" | vol %i | sze %i\t",*(s->volume),*(s->size)) ;
                                                        if(*(s->in16bits)) printf("16bits") ;
                                                        else               printf(" 8bits") ;
                                                        if(*(s->stereo))   printf(" stereo") ;
                                                        else               printf("  mono ") ;

                                                        if(*(s->looped))
                                                             printf(" | looped [%i .. %i]",*(s->loopStart),*(s->loopEnd)) ;
                                                        else printf("\t\t\t") ;
                                                        printf("\t| %s",s->fname) ;
                                                      }
                                               }// else printf("\n %i\t| sample error.",z) ;
              }; z=0 ;  printf("\n\n    msg / sample name") ;
                        while(z < *s3m.sampleNb) { s = s3m.sample[z++] ;
                                                   if(s) printf("\n %i\t|\t%s",z,s->name) ;
                        };
            }

            //printf("\n\nsample data : %x\nsize %i\n",sample001,sampleSize) ;

          printf("\n\n     chn settings.") ;
           { s8 chn, *s = s3m.chnSetting ;
            for(i=0;i<32;i++)
            { if(*s == 127){ s++ ; continue ; }
              chn = *(s+32) ;
              if(chn < 0) printf("\n %i\t./ error \\.",i) ;
              else { printf("\n %i\t| ",i) ;
                     switch(*s++)
                      {  case -1 : printf("left  %i",chn); break ;
                         case  1 : printf("right %i",chn); break ;
                         case  0 : printf("adlib %i",chn); break ;
                         default : printf("chn setting error.") ; break ;
                      };
                   }
            };
           }

           CSoundServer *pServer = (CSoundServer *) malloc(sizeof(CSoundServer));
           addFreeEntry(pServer) ;

            //if(goblin != (int*)_Goblin) free(goblin) ;
            printf("\n\n") ;


            if(pServer->open(mySoundProc,sampleSize)) system("pause") ;
            //pServer->close() ;
            addFreeEntry(0) ;
}


