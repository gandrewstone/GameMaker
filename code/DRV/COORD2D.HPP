//-------------------------------
// Copyright 1994 G. Andrew Stone
// Recreational Software Designs
// NOT PUBLIC DOMAIN!
//-------------------------------

#ifndef COORDINATE
#define COORDINATE

#include "gen.h"

inline int  Min (int a, int b) { if (a>b) return b; return a; }
inline int  Max (int a, int b) { if (b>a) return b; return a; }
inline int  Abs (int a) { if (a<0) return (-a); else return (a); }
inline void Swap(int &a,int &b) { int c=a; a=b; b=c; }

class Box2d;

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
  friend boolean operator &>  (Coord2d a,Coord2d b)
    { return((boolean)((a.x>b.x)&&(a.y>b.y))); }

  friend boolean operator <  (Coord2d a,Coord2d b)
    { return((boolean)((a.x<b.x)&&(a.y<b.y))); }
  friend boolean operator >= (Coord2d a,Coord2d b)
    { return((boolean)((a.x>=b.x)&&(a.y>=b.y))); }
  friend boolean operator <= (Coord2d a,Coord2d b)
    { return((boolean)((a.x<=b.x)&&(a.y<=b.y))); }
  friend boolean operator != (Coord2d a,Coord2d b)
    { return((boolean)((a.x!=b.x)||(a.y!=b.y))); }

  friend void Swap(Coord2d &a,Coord2d &b) { Coord2d c=a; a=b; b=c; }


  boolean In(Coord2d LowBnd,Coord2d UpBnd)
    { return ((boolean)((*this >=LowBnd)&&(*this <= UpBnd))); }
  boolean In(Box2d &b);
  
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

Coord2d operator += (int v)          { x += v; y += v; return *this; }
Coord2d operator -= (int v)          { x -= v; y -= v; return *this; }
Coord2d operator *= (int v)          { x *= v; y *= v; return *this; }
Coord2d operator /= (int v)          { x /= v; y /= v; return *this; }
Coord2d operator += (Coord2d &p) { x += p.x; y += p.y; return *this;}
Coord2d operator -= (const Coord2d &p) { x -= p.x; y -= p.y; return *this;}
Coord2d operator *= (const Coord2d &p) { x *= p.x; y *= p.y; return *this;}
Coord2d operator /= (const Coord2d &p) { x /= p.x; y /= p.y; return *this;}

Coord2d operator -- () { x--; y--; return *this;}
Coord2d operator ++ () { x++; y++; return *this;}


friend Coord2d operator * (int v,Coord2d &r) { Coord2d p(v*r.x,v*r.y); return p; }
friend Coord2d operator * (Coord2d &r,int v) { Coord2d p(v*r.x,v*r.y); return p; }
friend Coord2d operator / (int v,Coord2d &r) { Coord2d p(v/r.x,v/r.y); return p; }
friend Coord2d operator - (Coord2d &r,int v) { Coord2d p(r.x-v,r.y-v); return p; }
friend Coord2d operator + (Coord2d &r,int v) { Coord2d p(r.x+v,r.y+v); return p; }

friend Coord2d operator / (Coord2d &r,int v) { Coord2d p(r.x/v,r.y/v); return p; }
friend Coord2d operator - (Coord2d &q,Coord2d &r) { Coord2d p(q.x-r.x,q.y-r.y); return p;}
friend Coord2d operator + (Coord2d &q,Coord2d &r) { Coord2d p(q.x+r.x,q.y+r.y); return p;}
friend Coord2d operator / (Coord2d &q,Coord2d &r) { Coord2d p(q.x/r.x,q.y/r.y); return p;}
friend Coord2d operator * (Coord2d &q,Coord2d &r) { Coord2d p(q.x*r.x,q.y*r.y); return p;}
  };

class Box2d
  {
  public:
  Coord2d ul,lr;
  Box2d(int x,int y,int x1,int y1) { Set(x,y,x1,y1); }
  Box2d(Coord2d ull,Coord2d lrr)   { Set(ull,lrr); }
  Box2d() {}
  void Box2d::Set(Coord2d ull,Coord2d lrr)
    { ul=ull; lr=lrr; }
  void Box2d::Set(int x,int y,int x1,int y1)
    { ul.x=x,ul.y=y; lr.x=x1; lr.y=y1; }
  boolean In(Box2d b)  // is one box inside another?
    { return ( (boolean) (ul.In(b)&&lr.In(b)) ); }
  void operator -= (Coord2d c) { ul -= c; lr -= c; }
  };

inline boolean Coord2d::In(Box2d &b)
  {
  return ((boolean)((*this >=b.ul)&&(*this <= b.lr)));
  }

inline Coord2d SignC(Coord2d &a)
    {
    Coord2d r;
    r.x= Sign(a.x);
    r.y= Sign(a.y);
    return(r);
    }

inline Coord2d Abs(Coord2d &a)
    {
    Coord2d r;
    r.x= Abs(a.x);
    r.y= Abs(a.y);
    return(r);
    }

inline Coord2d AbsC(Coord2d &a) { Coord2d p( Abs(a.x),Abs(a.y) ); return p;}
inline Coord2d MakeCoord(int &x,int &y)              { Coord2d r(x,y);     return r; }
inline Coord2d MakeCoord2d (int xin, int yin)        { Coord2d r(xin,yin); return(r); }
inline Coord2d Pt(int x,int y)                     { Coord2d r(x,y);     return (r); }
inline Box2d   MakeBox2d (int x,int y,int x1,int y1) { Box2d r(x,y,x1,y1); return r; }
inline Box2d   MakeBox2d (Coord2d ul,Coord2d lr)     { Box2d r(ul,lr);     return r; }

#endif

