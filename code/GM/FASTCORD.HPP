#ifndef COORDINATE
#define COORDINATE

#include "gen.h"

#define Sign(x) ((x) > 0 ? 1:  ((x) == 0 ? 0:  (-1)))
#define Min(a,b) ( (a) < (b) ? (a):(b) )
#define Max(a,b) ( (a) > (b) ? (a):(b) )
#define Abs(a)  ( (a) > 0 ? (a):(-(a)) )

/* inline int Min (int a, int b) { if (a>b) return b; return a; }
inline int Max (int a, int b) { if (b>a) return b; return a; }
inline int Abs (int a) { if (a<0) return (-a); else return (a); }
*/

typedef struct
  {
  int x,y;
  } Coord2d;


/*
class Coord2d
  {
  public:
  int x,y;
  void Set(int xin,int yin) { x=xin; y=yin;}
  Coord2d(int xin,int yin) { Set(xin,yin); }
  Coord2d(void)            {}

  void Wrap(Coord2d Bnds)
    {
    if (y<0)            y += Bnds.y;
    else if (y>=Bnds.y) y -= Bnds.y;
    if (x<0)            x += Bnds.x;
    else if (x>=Bnds.x) x -= Bnds.x;
    }

  friend boolean operator >  (Coord2d a,Coord2d b)
    { return((boolean)((a.x>b.x)&&(a.y>b.y))); }
  friend boolean operator <  (Coord2d a,Coord2d b)
    { return((boolean)((a.x<b.x)&&(a.y<b.y))); }
  friend boolean operator >= (Coord2d a,Coord2d b)
    { return((boolean)((a.x>=b.x)&&(a.y>=b.y))); }
  friend boolean operator <= (Coord2d a,Coord2d b)
    { return((boolean)((a.x<=b.x)&&(a.y<=b.y))); }
  friend boolean operator != (Coord2d a,Coord2d b)
    { return((boolean)((a.x!=b.x)||(a.y!=b.y))); }

  friend Coord2d MakeCoord2d (int xin, int yin)
    { Coord2d r(xin,yin); return(r); }

  boolean In(Coord2d LowBnd,Coord2d UpBnd)
    {
    return ( (*this >=LowBnd)&&(*this <= UpBnd) );
    }

  friend Coord2d Min (Coord2d &a, Coord2d &b)
    {
    Coord2d r;
    r.x= Min(a.x,b.x);
    r.y= Min(a.y,b.y);
    return(r);
    }
  friend Coord2d Max (Coord2d &a, Coord2d &b)
    {
    Coord2d r;
    r.x= Max(a.x,b.x);
    r.y= Max(a.y,b.y);
    return(r);
    }

Coord2d& operator += (int &v)          { x += v; y += v; return *this; }
Coord2d& operator -= (int &v)          { x -= v; y -= v; return *this; }
Coord2d& operator *= (int &v)          { x *= v; y *= v; return *this; }
Coord2d& operator /= (int &v)          { x /= v; y /= v; return *this; }
Coord2d& operator += (Coord2d &p) { x += p.x; y += p.y; return *this;}
Coord2d& operator -= (const Coord2d &p) { x -= p.x; y -= p.y; return *this;}
Coord2d& operator *= (const Coord2d &p) { x *= p.x; y *= p.y; return *this;}
Coord2d& operator /= (const Coord2d &p) { x /= p.x; y /= p.y; return *this;}

friend Coord2d operator * (int &v,Coord2d &r) { Coord2d p(v*r.x,v*r.y); return p; }
friend Coord2d operator / (int &v,Coord2d &r) { Coord2d p(v/r.x,v/r.y); return p; }
friend Coord2d operator - (Coord2d &r,int &v) { Coord2d p(r.x-v,r.y-v); return p; }
friend Coord2d operator + (Coord2d &r,int &v) { Coord2d p(r.x+v,r.y+v); return p; }

friend Coord2d operator / (Coord2d &r,int &v) { Coord2d p(r.x/v,r.y/v); return p; }
friend Coord2d operator - (Coord2d &q,Coord2d &r) { Coord2d p(q.x-r.x,q.y-r.y); return p;}
friend Coord2d operator + (Coord2d &q,Coord2d &r) { Coord2d p(q.x+r.x,q.y+r.y); return p;}
friend Coord2d operator / (Coord2d &q,Coord2d &r) { Coord2d p(q.x/r.x,q.y/r.y); return p;}
friend Coord2d operator * (Coord2d &q,Coord2d &r) { Coord2d p(q.x*r.x,q.y*r.y); return p;}
  };

inline Coord2d AbsC(Coord2d &a) { Coord2d p( Abs(a.x),Abs(a.y) ); return p;}
*/

#define Coord2d_Wrap(r,MACROx,MACROy) { if (r.y<0) r.y += MACROy; else if (r.y>=MACROy) r.y -= MACROy; if (r.x<0) r.x += MACROx; else if (r.x>=MACROx) r.x -= MACROx; }
#define Coord2d_Set(r,MACROx,MACROy)   { r.x=MACROx; r.y=MACROy; }


#define Coord2d_Add(r,a,b)   { r.x=a.x+b.x; r.y=a.y+b.y; }
#define Coord2d_Sub(r,a,b)   { r.x=a.x-b.x; r.y=a.y-b.y; }
#define Coord2d_Div(r,a,b)   { r.x=a.x/b.x; r.y=a.y/b.y; }
#define Coord2d_Mul(r,a,b)   { r.x=a.x*b.x; r.y=a.y*b.y; }

#define Coord2d_SelfAdd(r,a) { r.x+=a.x; r.y+=a.y; }
#define Coord2d_SelfSub(r,a) { r.x-=a.x; r.y-=a.y; }
#define Coord2d_SelfDiv(r,a) { r.x/=a.x; r.y/=a.y; }
#define Coord2d_SelfMul(r,a) { r.x*=a.x; r.y*=a.y; }

#define Coord2d_GreaterEq(a,b) ((boolean)((a.x>=b.x)&&(a.y>=b.y)))
#define Coord2d_LessEq(a,b)    ((boolean)((a.x<=b.x)&&(a.y<=b.y)))
#define Coord2d_In(This,LowBnd,UpBnd) ((boolean) ((Coord2d_GreaterEq(This,LowBnd))&&(Coord2d_LessEq(This,UpBnd))))
#define Coord2d_NotEq(a,b)    ((boolean)((a.x!=b.x)&&(a.y!=b.y)))


#endif

