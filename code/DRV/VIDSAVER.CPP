#include "vidsaver.h"

void VideoSaver::Init(VideoMode *vid,uint MaxSave)
  {
  Vid     = vid;

  SetStdRoutines();
  Size    = vid->Size;
  Address = 0;
  BufSize = MaxSave;
  PixIdx  = 0;
  Buf     = new Pixel [MaxSave];
  }


VideoSaver::~VideoSaver()
  {
  if (Buf) delete Buf;
  Vid = NULL;  // I hate leaving pointers around.
  Buf = NULL;
  }

// These two routines are never called directly.  They are put into the
// Saver structure.
static void PointSave(Coord2d Pos, Pixel col,VideoMode *v)
  {
  VideoSaver *Me = (VideoSaver*) v;
  Me->Buf[Me->PixIdx] = Me->Vid->GetCol(Pos,Me->Vid);
  Me->Vid->Point(Pos,col,Me->Vid);
  Me->PixIdx++;
  }

static void PointRestore(Coord2d Pos, Pixel col,VideoMode *v)
  {
  VideoSaver *Me = (VideoSaver*) v;
  if (Me->Buf[Me->PixIdx]!=col) Me->Vid->Point(Pos,Me->Buf[Me->PixIdx],Me->Vid);
  Me->PixIdx++;
  }

void VideoSaver::Save(void)
  {
  Point  = PointSave;
  PixIdx = 0;
  }

void VideoSaver::Restore(void)
  {
  Point  = PointRestore;
  PixIdx = 0;
  }

