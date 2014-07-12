#include "viddrive.hpp"

static void PointSave   (Coord2d Pos, Pixel col,VideoMode *v);
static void PointRestore(Coord2d Pos, Pixel col,VideoMode *v);

class VideoSaver:public VideoMode
  {
  Pixel     *Buf;
  uint       PixIdx;
  uint       BufSize;
  public:
  VideoMode *Vid;

  VideoSaver() { Buf=NULL; BufSize=0; }
 ~VideoSaver();
  void Init     (VideoMode *Vid,uint MaxSave);
  void SwitchTo (VideoMode *vid) { Vid = vid; }
  void Save     (void);    // Tell Saver what to do!
  void Restore  (void);
  friend void PointSave(Coord2d Pos, Pixel col,VideoMode *v);
  friend void PointRestore(Coord2d Pos, Pixel col,VideoMode *v);
  };
