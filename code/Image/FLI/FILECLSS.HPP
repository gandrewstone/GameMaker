// --------------------------------
//  Copyright 1994 G. Andrew Stone
//  Not public domain!
// --------------------------------

#ifndef FILECLASS
#define FILECLASS
#include "gen.h"
#define DEBUG(x)
#include <string.h>
#include <process.h>
#include <stdio.h>

class FileRead
  {
  char          Name[NAMELEN];
  FILE          *fp;
  uchar         *buf;
  long int      idx;
  long int      blen;
  long int      End;
  public:
  boolean       EOFflag;
  long int      Length;
  long int      CurPos;

  boolean       Open(char *fname)
    {
    if (fp != NULL) fclose(fp);
    strcpy(Name,fname);

    EOFflag=True;  // If Open fails, this should be true, so set it now.

    if ((fp=fopen(fname,"rb"))==NULL) return(False);
    fseek(fp,0,SEEK_END);    // Go to the end of the file
    Length = ftell(fp);      // Get the file length
    fseek(fp,0,SEEK_SET);    // Go back to the beginning

//    blen=Min(blen,Length);
    // Get Buffer memory.
    if (buf==NULL)
      {
      while ((buf==NULL)&&(blen>10)) { buf = new unsigned char [blen]; blen-=2; }
      blen+=2;
      }
    if (buf==NULL) { fclose(fp); fp=NULL; return(False); }

    End     =  fread(buf,sizeof(unsigned char),blen,fp);
    CurPos  =  0;
    idx     =  0;
    EOFflag =  False;
    return(True);
    }

  void Close(void)
    {
    if (fp != NULL) { fclose(fp); fp=NULL; }
    Name[0]=0;
    if (buf!=NULL) delete buf;
    blen=65534;
    End=0;
    idx=0;
    EOFflag=True;
    }

  inline int Peek(void) { return(*(buf+idx)); }
  uchar *Peek(uint Bytes);
  boolean LoadMore(void);

  boolean Get(void *buffer,unsigned long int Numbytes)
    {
//    DEBUG("Get\n");

    unsigned long int Amt;

    if (idx+Numbytes>=End) Amt = End-idx;
    else Amt=Numbytes;

    for (int i=0;i<Amt;i++) ((uchar *)buffer)[i]=buf[idx+i];
//    memcpy(buffer,buf+idx,Numbytes);
    if (Amt !=Numbytes)
      {
      if (!LoadMore()) exit(1);
      for (int i=0;i<Numbytes-Amt;i++) ((uchar *)buffer)[i+Amt]=buf[idx+i];
//    memcpy(buffer+Amt,buf+idx,Numbytes);
      idx+=Numbytes-Amt;
      }
    else idx+=Numbytes;
    CurPos+=Numbytes;
    return(LoadMore());
    }
  boolean Skip(uint Bytes)
  {
  DEBUG("Skip\n");
  idx+=Bytes;
  CurPos+=Bytes;
  return(LoadMore());
  }

  int Getc(void)
    {
    if ((EOFflag)||(buf==NULL)||(fp==NULL)) return(EOF);
    unsigned char val=*(buf+idx);
    idx++;
    CurPos++;
    if (idx>=End) LoadMore();
    return(val);
    }
  inline void Backup(unsigned int amt=1) { idx-=amt; }

  FileRead()  { fp=NULL; Name[0]=0; buf=NULL; blen=65534; idx=0; End=0; EOFflag=True;}
  ~FileRead() { Close(); }
  FileRead(char *fname)
    {
    fp=NULL; Name[0]=0; buf=NULL; blen=65534; idx=0; End=0; EOFflag=True;
    if (!Open(fname)) { printf("File Not Found:%s.\n",fname); exit(1); }
    }
  };


class FileWrite
  {
  char          Name[NAMELEN];
  FILE          *fp;
  boolean       EOFflag;
  public:
  long int      Length;
  long int      CurPos;

  boolean       Open(char *fname)
    {
    if (fp != NULL) fclose(fp);
    if ((fp=fopen(fname,"wb"))==NULL) return(False);
    strcpy(Name,fname);

    Length=0;
    EOFflag =  False;
    CurPos  =  0;
    return(True);
    }

  void Close(void)
    {
    if (fp != NULL) { fclose(fp); fp=NULL; }
    EOFflag=False;
    }

  long int Write(void *Data,long int count)
    {
    return(fwrite(Data,count,1,fp));
    }

  FileWrite()  { fp=NULL; Name[0]=0; EOFflag=False;}
  ~FileWrite() { Close(); }
  FileWrite(char *fname)
    {
    fp=NULL; Name[0]=0; EOFflag=False;
    if (!Open(fname)) { printf("File Not Found:%s.\n",fname); exit(1); }
    }
  };


#endif


