
#include "viddrive.hpp"


static void VidSavePoint   (Coord2d Pos, Pixel col,VideoMode *Vid);
static void VidRestPoint   (Coord2d Pos, Pixel col,VideoMode *Vid);

class VideoSave:public VideoMode
  {
  public:
  Pixel *Buf;
  uint   idxSave;
  uint   idxRest;
  void (*RealPoint)      (Coord2d Pos, Pixel col,VideoMode *v);

  VideoSave()            { Buf==NULL; Forget();  }
  void Forget(void)      { idxSave=0; idxRest=0; }

  void SaveMode(void)    { idxSave=0; Point = VidSavePoint; }
  void RestoreMode(void) { idxRest=0; Point = VidRestPoint; }
  void IgnoreMode(void)  {            Point = RealPoint;    }

  VideoSave& operator = (VideoMode &Vid)
    {
    ((VideoMode*)this)->SetStdRoutines();
    SetMode             = Vid.SetMode;
    RealPoint           = Vid.Point;
    GetCol              = Vid.GetCol;
    Point               = VidSavePoint;   // Start off in Save mode.

    Address             = Vid.Address;
    Size                = Vid.Size;
    LineSep             = Vid.LineSep;
    ColDepth            = Vid.ColDepth;
    ViewPageOff         = Vid.ViewPageOff;
    BIOSMode            = Vid.BIOSMode;

    idxSave = idxRest   = 0;
    return(*this);
    }
  };


static void VidRestPoint   (Coord2d Pos, Pixel col,VideoMode *Vid)
  {
  VideoSave *v=((VideoSave*)Vid);

  v->RealPoint(Pos,v->Buf[v->idxRest],Vid);
  v->idxRest++;
  }

static void VidSavePoint   (Coord2d Pos, Pixel col,VideoMode *Vid)
  {
  VideoSave *v=((VideoSave*)Vid);

  v->Buf[v->idxSave] = v->GetCol(Pos,Vid);
  v->idxSave++;
  v->RealPoint(Pos,col,Vid);
  }
