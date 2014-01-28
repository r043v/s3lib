/*************
  Gdl ² reborn - pc release
                 ************/

#include <stdio.h>
#include <alloc.h>
//#include <conio_mingw.h>


//#include <windows.h>
//#include "tinyptc.h"

#define kspace 32
#define kenter 13
#define kfin   35
#define kshift 17
#define kleft  37
#define kup    38
#define kright 39
#define kdown  40

int WIDTH=320, HEIGHT=240, fSIZE, *pixel, tick ;
//extern unsigned char key[256] ;

//void drawGfm(int *Gfm, int x, int y);

void addFreeEntry(void * add)
{  static void **freeList = 0 ;
   static int    entryNum = 0 ;
   if(!freeList) freeList = (void**)malloc(512*4) ;
   if(add) freeList[entryNum++] = add ; // add an entry
   else { printf("\n free : %i { ",entryNum) ; while(entryNum--) if(freeList[entryNum]) { printf("%x ",freeList[entryNum]) ;
                                                                            free(freeList[entryNum]) ;
                                                                          } free(freeList) ;
          printf("}\n") ;
        }
   if(entryNum == 512) { printf("\ntoo many free entry.\n") ; system("pause") ; }
}

void close(void)
{ free(pixel) ; addFreeEntry(0) ;
}

int getSize(FILE *f)
{ int last, size ;
  last = ftell(f) ;  fseek(f,0,SEEK_END) ;
  size = ftell(f) ;
  fseek(f,last,SEEK_SET) ; return size ;
}

unsigned char * loadFile(const char *path)
{ FILE *f ; int sz ; unsigned char * fl ;
  printf("\n loading %s",path) ;
  f = fopen(path,"rb") ;
  if(!f){ printf(" .. file open error !\n") ; system("pause") ; }
  else { sz = getSize(f) ;
         printf(" (%iko)\n",sz>>10) ;
       }
  fl = (unsigned char*)malloc(sz) ; addFreeEntry(fl) ;
  fread(fl,sz,1,f) ;    fclose(f) ;
  return fl ;
}

void bin2h(const char *file, const char * name)
{ FILE *f = fopen(file,"rb") ;
  int sz = getSize(f) ;
  char * fl = (char*)malloc(sz) ;
  fread(fl,sz,1,f) ;    fclose(f) ;
  char path[42] ;       sprintf(path,"%s.h",file) ;
  f = fopen(path,"wb") ; fprintf(f,"unsigned char %s[%i] = {\n",name,sz) ;
  for(int c=0;c<sz-1;c++) { fprintf(f,"\t%i,",(unsigned char)fl[c]) ; if(c) if(!(c%8)) fprintf(f,"\n") ; }
  fprintf(f,"\t%i \n};\n",(unsigned char)fl[sz-1]) ;  fclose(f) ;
}

