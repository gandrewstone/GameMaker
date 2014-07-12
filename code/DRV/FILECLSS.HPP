//-------------------------------
// Copyright 1994 G. Andrew Stone
// Recreational Software Designs
// NOT PUBLIC DOMAIN!
//-------------------------------

#ifndef FILECLASS
#define FILECLASS
#include "gen.h"
#define DEBUG(x)
#include <string.h>
#include <process.h>
#include <stdio.h>

class FileRead
  {
  FILE          *fp;
  uchar         *buf;
  longi         idx;
  longi         blen;
  longi         End;
  uchar         OldChar;
  uchar         *OldPos;
  public:
  char          Name[NAMELEN];
  boolean       EOFflag;
  longi         Length;
  longi         CurPos;

  boolean       SetPos(longi Pos)
    {
    if ((Pos>=(CurPos-idx))&&(Pos<=(CurPos-idx)+End))
      {
      idx= Pos-CurPos+idx; CurPos=Pos; if (CurPos<Length) EOFflag=False;
      return(True);
      }
    return(False);
    }


  boolean       Open(char *fname);

  void Close(void)
    {
    if (fp  != NULL) { fclose(fp); fp  = NULL; }
    if (buf != NULL) { delete buf; buf = NULL; }
    blen=0xFFF0;
    Name[0]=0;
    End =0;
    idx =0;
    EOFflag=False;
    }

  inline int Peek(void) { return(*(buf+idx)); }
  uchar*     Peek(uint &Bytes);
  boolean    LoadMore(void);
  boolean    Get(void *buffer,uint Numbytes)
    {
    DEBUG("Get\n");
    if (!EOFflag)
      {
      if (idx+Numbytes <= End)
        {
        if (buffer !=NULL) //memcpy(buffer,buf+idx,Numbytes);
          for(uint i=0;i<Numbytes;i++) ((uchar*) buffer)[i] = *(buf+idx+i);
        idx    += Numbytes;
        CurPos += Numbytes;
        }
      else
        {
        uint   CanCopy=End-idx;
        Get(buffer,CanCopy);  // Split this up so that the LoadMore() gets executed in the correct spot.
        Get(((uchar*)buffer)+CanCopy,Numbytes-CanCopy);
        }
      return(LoadMore());
      }
    return(False);
    }

  boolean Skip(long int Bytes)
    {
    DEBUG("Skip\n");
    idx    += Bytes;
    CurPos += Bytes;
    return(LoadMore());
    }
  int Get(void)
    {
    if ((EOFflag)||(buf==NULL)) return(EOF);
    uchar val=*(buf+idx);
    idx   ++;
    CurPos++;
    LoadMore();
    return(val);
    }
  int Getc(void)
    {
    if ((EOFflag)||(buf==NULL)) return(EOF);
    uchar val=*(buf+idx);
    idx   ++;
    CurPos++;
    LoadMore();
    return(val);
    }

  inline void Backup(uint amt=1) { idx-=amt; CurPos-=amt; }

  FileRead()  { fp=NULL; Name[0]=0; buf=NULL; blen=65534; idx=0; End=0; EOFflag=True;}
  ~FileRead() { Close(); }
  FileRead(char *fname)
    {
    fp=NULL; Name[0]=0; buf=NULL; blen=65534; idx=0; End=0; EOFflag=True;
    Open(fname);
    }
  };




class FileWrite
  {
  FILE          *fp;
  public:
  char          Name[NAMELEN];
  long int      Length;
  long int      CurPos;

  boolean       Open(char *fname);
  uint          Write(uchar *Data,uint count);
  void          Close(void);


  FileWrite()  { fp=NULL; Name[0]=0; Length=0; CurPos=0; }
  ~FileWrite() { Close(); }
  FileWrite(char *fname)
    {
    fp=NULL; Name[0]=0; Length=0; CurPos=0;
    Open(fname);
    }
  };

#endif


