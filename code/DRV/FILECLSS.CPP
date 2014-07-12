// Copyright 1994 G. Andrew Stone
// Recreational Software Designs
// NOT PUBLIC DOMAIN!
//----------------------------------------------------------------------

#include "fileclss.hpp"

boolean FileRead::Open(char *fname)
    {
    if (fp != NULL) fclose(fp);

    strcpy(Name,fname);

    if ((fp=fopen(fname,"rb"))==NULL) { EOFflag=True; return(False); }
    fseek(fp,0,SEEK_END);    // Go to the end of the file
    Length = ftell(fp);      // Get the file length
    fseek(fp,0,SEEK_SET);    // Go back to the beginning
    if (blen<Length+1L)
      {
      if (buf!=NULL) { delete buf; buf=NULL; }
      blen = Length+1L;
      if (blen>0x0000FFF0) blen=0x0000FFF0;
      }

    if (buf==NULL)    // Get Buffer memory.
      {
      while ((buf==NULL)&&(blen>10)) { buf = new uchar [blen]; blen-=2; }
      blen+=2;
      }
    if (buf==NULL) { fclose(fp); fp=NULL; return(False); }

    End     =  fread(buf,sizeof(uchar),blen,fp);
    EOFflag =  False;
    CurPos  =  0;
    idx     =  0;
    if (End==Length) { fclose(fp); fp=NULL; }  // Its all in!
    return(True);
    }

uchar* FileRead::Peek(uint &Bytes)
    {
    DEBUG("Peek\n");
    if (idx+((long int)Bytes)>=End)
      {
//      if (idx+Bytes>=blen)
//        {
        uint LeftOver = End-idx;
//        memcpy(buf,buf+idx,LeftOver);  // Bombs w/ gif.
        for (uint t=0;t<LeftOver;t++) buf[t]=buf[t+idx];
        End=LeftOver+fread(buf+LeftOver,sizeof(uchar),((uint)blen)-LeftOver,fp);
        if (End==0) EOFflag=True;
        idx=0;
//        }
/*      else
        {
        End=End+fread(buf,sizeof(unsigned char),blen/2,fp);
        if (idx>End) EOFflag=True;
        }
*/
      if (idx+((long int)Bytes)>End) { Bytes=End-idx; if (Bytes==0) EOFflag=True; }
      }
    return(buf+idx);
    }

boolean FileRead::LoadMore(void)
  {
  if ((idx>=End)&&(!EOFflag))
    {
    if (CurPos>=Length) { EOFflag=True; return((boolean)!EOFflag); }
    if (idx==End) // == blen
      {
      idx=0;
      if (fp!=NULL) End=fread(buf,sizeof(uchar),blen,fp);
      else End=0;
      if (End==0) EOFflag=True;
      }
    if (idx>blen) { EOFflag=True; }
/*    else
      {
      End=End+fread(&(buf[End]),sizeof(unsigned char),blen/2,fp);
      if (idx>End) EOFflag=True;
      }
*/
    }
  return((boolean) !EOFflag);
  }


boolean FileWrite::Open(char *fname)
    {
    if (fp != NULL) fclose(fp);
    if ((fp=fopen(fname,"wb"))==NULL) return(False);
    strcpy(Name,fname);

    Length  =  0;
    CurPos  =  0;
    return(True);
    }

uint FileWrite::Write(uchar *Data,uint count)
    {
    if (fp != NULL)
      {
      count=fwrite(Data,1,count,fp);
      CurPos+=count;
      Length=CurPos;
      return(count);
      }
    return(False);
    }

void FileWrite::Close(void)
    {
    if (fp != NULL) { fclose(fp); fp=NULL; }
    Length=0; CurPos=0;
    }


