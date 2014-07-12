//-------------------------------
// Copyright 1994 G. Andrew Stone
// Recreational Software Designs
// NOT PUBLIC DOMAIN!
//-------------------------------

#include "xms.hpp"


class XMSVideoMode:public VideoMode
  {
  public:
  XMStransfer Set;
  XMStransfer Get;
  };

boolean XMSVideoInit(XMSVideoMode *v);
void    XMSVideoUnInit(XMSVideoMode *x);


