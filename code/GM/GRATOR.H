#include "gen.h"
#include "coord2d.hpp"
static uint StrInBuf(char *str,char *Buf,int Blen);

typedef uchar Pixel;

#define NOSCENE     0xFFFF
#define STARTTYPE   1
#define ENDTYPE     2
#define ADVTYPE    10
#define SHELLTYPE  11
#define MENUTYPE   12

#define STARTSCENE  0
#define ENDSCENE    1

#define OLDMAXLINK    20 // Define maximum number of links to other scenes
#define OLDMAXSCENES  20 // Define maximum number of scenes for this game
#define OLDLASTFNAME  10 // Define number of files that are part of a scene
#define STARTENDFILES 5  // # of files in start or end of a scene

#define PREGRAPHICFILEIDX  9
#define POSTGRAPHICFILEIDX 10


#define SCENEDESCLEN     20
#define SCENEFILENAMELEN 13
#define SCENEFILES       20
#define MAXSCENELINKS    20
#define NOLINK         0xFF


#define SNDFILE           7
#define EXTRASCENEBUFFER 30

typedef struct           // Declare a typed data structure
  {
  int fx;                // Map block X where character jumps from to new map
  int fy;                // Map block Y where character jumps from to new map
  int tnum;              // Scene number to go to
  int tx;                // Map block X address to jump to
  int ty;                // Map block Y address to jump to
  } OLDlink;                // Name the data structure

class OLDintegratorstruct
  {
  public:
  unsigned int gametype; // All adventure games are 10.
  int x;                 // X pixel location of scene within integrator
  int y;                 // Y pixel location of scene within integrator
  char desc[15];         // Description of the scene
  char fnames[9][OLDLASTFNAME]; // List of file name used for this scene
  OLDlink links[OLDMAXLINK];   // List of linked scenes associated with this scene
  char red,green,blue;   // change every col in pal by this amt.
  unsigned char ssup;    // Scroll direction info
  unsigned char ssdown;  // 0=no scroll, 0xFF=scroll on char. #=constant speed
  unsigned char ssleft;
  unsigned char ssright;
  unsigned char PinChar; // TRUE if char should always be exactly in the middle
  char extra[11];        // Not Used
  }; 

class ScrollInfo
  {
  public:
  uchar up;      // Scroll direction info
  uchar down;    // 0=no scroll, 0xFF=scroll on char. #=constant speed
  uchar left;
  uchar right;
  uchar Style;   // 0=normal, 1=Pin character in center, 2=snap
  uchar Xtra[11];
  void Clear(void)
    {
    for (int i=0;i<11;i++) Xtra[i]=0;
    up      = down = left = right = 0xFF;
    Style   = FALSE;
    }
  };

typedef struct
  {
  int NumScenes;
  int FilesPerScene;             // Always = to SCENEFILES.
  int LinksPerScene;             // Always = to MAXSCENELINKS.
  } GratorHeader;


class Link
  {
  public:
  uint    ToScene;              // Scene number to go to.
  Coord2d From;
  Coord2d To;
  void Clear(void)
    {
    To.x = To.y = From.y = From.x = 0;
    ToScene = NOSCENE;
    }
  Link& operator = (OLDlink &v)
    {
    From.x  = v.fx;
    From.y  = v.fy;
    To.x    = v.tx;
    To.y    = v.ty;
    if (v.tnum==OLDMAXSCENES) ToScene=NOSCENE;
    else ToScene = v.tnum;
    return (*this);
    }
  };

class SceneFiles
  {
  char *Mem;
  uint MaxMem;
  uint FreePtr;
  public:

  void Clear(void)
    {
    if (Mem!=NULL) { Mem[0]=0; FreePtr=1; }
    }

  SceneFiles()
    {
    MaxMem  = 10000;
    Mem     = new char [MaxMem];
    if (Mem==NULL) exit(1);
    Clear();
    }
  ~SceneFiles()
    {
    if (Mem) delete Mem;
    Mem     = NULL;
    FreePtr = 0;
    MaxMem  = 0;
    }

  uint Add(char *fname)
    {
    uint RetVal;
    if (fname[0]==0) return(0);
    if ( (RetVal=StrInBuf(fname,Mem,FreePtr))==0xFFFF)
      {
      strcpy(Mem+FreePtr,fname);
      RetVal=FreePtr;
      FreePtr+=strlen(fname)+1;
      }
    return(RetVal);
    }
  boolean Load(FILE *fp);

  boolean Save(FILE *fp)
    {
    fwrite(&FreePtr,sizeof(uint),1,fp);   // Write the Buffer Length.
    fwrite(Mem,sizeof(char),FreePtr,fp);  // Write the buffer.
    return(TRUE);
    }

  char* operator() (uint fptr) { return(&Mem[fptr]); }
  };

class Scene
  {
  public:
  uint       gametype;
  Coord2d    Pos;
  ScrollInfo Scroll;
  uchar      NumLinks;
  char       desc [SCENEDESCLEN];  // Description of the scene
  uint       Files[SCENEFILES];    // List of file name used for this scene
  Link       links[MAXSCENELINKS]; // List of linked scenes associated with this scene

  Scene() { Clear(); }

  void Clear(void)
    {
    int i;
    gametype=NOSCENE;
    NumLinks=MAXSCENELINKS;
    Pos.x=Pos.y=0;
    strcpy(desc,"UnTitled");
    for (i=0;i<SCENEDESCLEN ;i++) desc[i]  = 0;
    for (i=0;i<SCENEFILES   ;i++) Files[i] = 0;
    for (i=0;i<MAXSCENELINKS;i++) links[i].Clear();
    Scroll.Clear();
    }

  Scene& operator = (OLDintegratorstruct &v)
    {
    if ((v.gametype==STARTTYPE)||(v.gametype==ENDTYPE)||(v.gametype==ADVTYPE))
      gametype     = v.gametype;
    else gametype = NOSCENE;

    Pos.x        = v.x;
    Pos.y        = v.y;
    Scroll.up    = v.ssup;
    Scroll.left  = v.ssleft;
    Scroll.right = v.ssright;
    Scroll.down  = v.ssdown;
    Scroll.Style = FALSE;
    NumLinks     = MAXSCENELINKS;

    for (int i=0;i<OLDMAXLINK;i++) links[i] = v.links[i];
    strncpy(desc,v.desc,SCENEDESCLEN);
    return(*this);
    }
  };
 

class GameClass
  {
  char Temp[81];
  public:
  Scene      *scns;
  SceneFiles  Files;
  int         MaxScenes;
  int         NumScenes;
  GameClass()
    {
    MaxScenes=EXTRASCENEBUFFER;
    NumScenes=0;
    if ((scns = new Scene [MaxScenes])==NULL) exit(1);
    }
  ~GameClass()
    {
    MaxScenes=0;
    NumScenes=0;
    if (scns) delete scns;
    scns=NULL;
    }

  void    Init           (void);
  boolean FileExists     (int Scnum,int Filenum)
    {
    if (scns[Scnum].Files[Filenum]==0) return(False);
    return(True);
    }
  char   *File           (int Scnum,int Filenum)
    {
    strcpy(Temp,WorkDir);
    strcat(Temp,Files(scns[Scnum].Files[Filenum]));
    return(Temp);
    }
  boolean Load           (char *Name);
  boolean Load           (FILE *fp);
  boolean Save           (FILE *fp);
  uint    CreateScene    (int x,int y);
  void    EraseScene     (uint scene);
  uint    SceneAt        (int x,int y);
  void    DrawScene      (uint scene, Pixel col);
  void    DrawSceneLinks (uint scene,Pixel col);
  void    CleanDrawLink  (int from,int to,Pixel col);
  void    DrawLink       (int from,int to,Pixel col);
  void    Draw           (void);
  void    Optimize       (void);           // Eliminate all unused scenes and redundancies.
  void    FileConverter  (int oldsc,int oldfi,int newsc,int newfi,char *Ext,OLDintegratorstruct *old);
  };

// End of Grator.h Header file
