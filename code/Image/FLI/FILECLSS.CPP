// --------------------------------
//  Copyright 1994 G. Andrew Stone
//  Not public domain!
// --------------------------------

#include "fileclss.hpp"
uchar* FileRead::Peek(uint Bytes)
    {
    DEBUG("Peek\n");
    if (idx+Bytes>=End)
      {
//      if (idx+Bytes>=blen)
//        {
        uint LeftOver = End-idx;
//        memcpy(buf,buf+idx,LeftOver);  // Bombs w/ gif.
        for (uint t=0;t<LeftOver;t++) buf[t]=buf[t+idx];
        End=LeftOver+fread(buf+LeftOver,sizeof(uchar),blen-LeftOver,fp);
        if (End==0) EOFflag=True;
        idx=0;
//        }
/*      else
        {
        End=End+fread(buf,sizeof(unsigned char),blen/2,fp);
        if (idx>End) EOFflag=True;
        }
*/
      }
    if (idx+Bytes>End) { DEBUG("Peek Failure!\n"); return(NULL); }
    return(buf+idx);
    }

boolean FileRead::LoadMore(void)
  {
  while ((idx>=End)&&(!EOFflag))
    {
    if (idx==End) // == blen
      {
      idx=0;
      End=fread(buf,sizeof(unsigned char),blen,fp);
      if (End==0) EOFflag=True;
      }
    if (idx>blen) printf("FILE Buffer ERROR idx>blen!\n");
/*    else
      {
      End=End+fread(&(buf[End]),sizeof(unsigned char),blen/2,fp);
      if (idx>End) EOFflag=True;
      }
*/
    }
  return(!EOFflag);
  }


