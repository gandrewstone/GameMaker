/*---------------------------------------------------------------------------

  macfile.c

  This source file is used by the mac port to support commands not available
  directly on the Mac, i.e. mkdir().
  It also helps determine if we're running on a Mac with HFS and a disk
  formatted for HFS (HFS - Hierarchical File System; compared to its predecessor,
  MFS - Macintosh File System).
  
  ---------------------------------------------------------------------------*/

#include "unzip.h"

#ifdef MACOS
#ifndef FSFCBLen
#define FSFCBLen    (*(short *)0x3F6)
#endif

static short wAppVRefNum;
static long lAppDirID;
int hfsflag;            /* set if disk has hierarchical file system */

static int IsHFSDisk(short wRefNum)
{
    /* get info about the specified volume */
    if (hfsflag == true) {
        HParamBlockRec    hpbr;
        Str255 temp;
        short wErr;
        
        hpbr.volumeParam.ioCompletion = 0;
        hpbr.volumeParam.ioNamePtr = temp;
        hpbr.volumeParam.ioVRefNum = wRefNum;
        hpbr.volumeParam.ioVolIndex = 0;
        wErr = PBHGetVInfo(&hpbr, 0);

        if (wErr == noErr && hpbr.volumeParam.ioVFSID == 0
            && hpbr.volumeParam.ioVSigWord == 0x4244) {
                return true;
        }
    }

    return false;
} /* IsHFSDisk */

void macfstest(int vrefnum)
{
    Str255 st;

    /* is this machine running HFS file system? */
    if (FSFCBLen <= 0) {
        hfsflag = false;
    }
    else
    {
        hfsflag = true;
    }

    /* get the file's volume reference number and directory ID */
    if (hfsflag == true) {
        WDPBRec    wdpb;
        OSErr err = noErr;

        if (vrefnum != 0) {
            wdpb.ioCompletion = false;
            wdpb.ioNamePtr = st;
            wdpb.ioWDIndex = 0;
            wdpb.ioVRefNum = vrefnum;
            err = PBHGetVol(&wdpb, false);
        
            if (err == noErr) {
                wAppVRefNum = wdpb.ioWDVRefNum;
                lAppDirID = wdpb.ioWDDirID;
            }
        }

        /* is the disk we're using formatted for HFS? */
        hfsflag = IsHFSDisk(wAppVRefNum);
    }
    
    return;
} /* mactest */

int macmkdir(char *path, short nVRefNum, long lDirID)
{
    OSErr    err = -1;

    if (path != 0 && strlen(path)<256 && hfsflag == true) {
        HParamBlockRec    hpbr;
        Str255    st;

        CtoPstr(path);
        if ((nVRefNum == 0) && (lDirID == 0))
        {
            hpbr.fileParam.ioNamePtr = st;
            hpbr.fileParam.ioCompletion = NULL;
            err = PBHGetVol((WDPBPtr)&hpbr, false);
            nVRefNum = hpbr.wdParam.ioWDVRefNum;
            lDirID = hpbr.wdParam.ioWDDirID;
        }
        else
        {
            err = noErr;
        }
        if (err == noErr) {
            hpbr.fileParam.ioCompletion = NULL;
            hpbr.fileParam.ioVRefNum = nVRefNum;
            hpbr.fileParam.ioDirID = lDirID;
            hpbr.fileParam.ioNamePtr = (StringPtr)path;
            err = PBDirCreate(&hpbr, false);
        }    
        PtoCstr(path);
    }

    return (int)err;
} /* mkdir */

void ResolveMacVol(short nVRefNum, short *pnVRefNum, long *plDirID, StringPtr pst)
{
    if (hfsflag)
    {
        WDPBRec  wdpbr;
        Str255   st;
        OSErr    err;

        wdpbr.ioCompletion = (ProcPtr)NULL;
        wdpbr.ioNamePtr = st;
        wdpbr.ioVRefNum = nVRefNum;
        wdpbr.ioWDIndex = 0;
        wdpbr.ioWDProcID = 0;
        wdpbr.ioWDVRefNum = 0;
        err = PBGetWDInfo( &wdpbr, false );
        if ( err == noErr )
        {
            if (pnVRefNum)
                *pnVRefNum = wdpbr.ioWDVRefNum;
            if (plDirID)
                *plDirID = wdpbr.ioWDDirID;
            if (pst)
                BlockMove( st, pst, st[0]+1 );
        }
    }
    else
    {
        if (pnVRefNum)
            *pnVRefNum = nVRefNum;
        if (plDirID)
            *plDirID = 0;
        if (pst)
            *pst = 0;
    }
}

short macopen(char *sz, short nFlags, short nVRefNum, long lDirID)
{
    OSErr   err;
    Str255  st;
    char    chPerms = (!nFlags) ? fsRdPerm : fsRdWrPerm;
    short   nFRefNum;

    CtoPstr( sz );
    BlockMove( sz, st, sz[0]+1 );
    PtoCstr( sz );
    if (hfsflag)
    {
        if (nFlags > 1)
            err = HOpenRF( nVRefNum, lDirID, st, chPerms, &nFRefNum);
        else
            err = HOpen( nVRefNum, lDirID, st, chPerms, &nFRefNum);
    }
    else
    {
        /*
         * Have to use PBxxx style calls since the high level
         * versions don't support specifying permissions
         */
        ParamBlockRec    pbr;

        pbr.ioParam.ioNamePtr = st;
        pbr.ioParam.ioVRefNum = gnVRefNum;
        pbr.ioParam.ioVersNum = 0;
        pbr.ioParam.ioPermssn = chPerms;
        pbr.ioParam.ioMisc = 0;
        if (nFlags >1)
            err = PBOpenRF( &pbr, false );
        else
            err = PBOpen( &pbr, false );
        nFRefNum = pbr.ioParam.ioRefNum;
    }
    if ( err )
        return -1;
    else
        return nFRefNum;
}

short maccreat(char *sz, short nVRefNum, long lDirID, OSType ostCreator, OSType ostType)
{
    OSErr   err;
    Str255  st;
    FInfo   fi;

    CtoPstr( sz );
    BlockMove( sz, st, sz[0]+1 );
    PtoCstr( sz );
    if (hfsflag)
    {
        err = HGetFInfo( nVRefNum, lDirID, st, &fi );
        if (err == fnfErr)
            err = HCreate( nVRefNum, lDirID, st, ostCreator, ostType );
        else if (err == noErr)
        {
            fi.fdCreator = ostCreator;
            fi.fdType = ostType;
            err = HSetFInfo( nVRefNum, lDirID, st, &fi );
        }
    }
    else
    {
        err = GetFInfo( st, nVRefNum, &fi );
        if (err == fnfErr)
            err = Create( st, nVRefNum, ostCreator, ostType );
        else if (err == noErr)
        {
            fi.fdCreator = ostCreator;
            fi.fdType = ostType;
            err = SetFInfo( st, nVRefNum, &fi );
        }
    }
    if (err == noErr)
        return noErr;
    else
        return -1;
}

short macread(short nFRefNum, char *pb, unsigned cb)
{
    long    lcb = cb;

    (void)FSRead( nFRefNum, &lcb, pb );

    return (short)lcb;
}

short macwrite(short nFRefNum, char *pb, unsigned cb)
{
    long    lcb = cb;

    (void)FSWrite( nFRefNum, &lcb, pb );

    return (short)lcb;
}

short macclose(short nFRefNum)
{
    return FSClose( nFRefNum );
}

long maclseek(short nFRefNum, long lib, short nMode)
{
    ParamBlockRec   pbr;

    if (nMode == SEEK_SET)
        nMode = fsFromStart;
    else if (nMode == SEEK_CUR)
        nMode = fsFromMark;
    else if (nMode == SEEK_END)
        nMode = fsFromLEOF;
    pbr.ioParam.ioRefNum = nFRefNum;
    pbr.ioParam.ioPosMode = nMode;
    pbr.ioParam.ioPosOffset = lib;
    (void)PBSetFPos(&pbr, 0);
    return pbr.ioParam.ioPosOffset;
}

#endif /* MACOS */
