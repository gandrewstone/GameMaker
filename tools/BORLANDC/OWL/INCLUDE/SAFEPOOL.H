// ObjectWindows - (C) Copyright 1992 by Borland International

#if !defined(__SAFEPOOL_H)
#define __SAFEPOOL_H

#if !defined(__OWLDEFS_H)
#include <owldefs.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

const int DEFAULT_SAFETY_POOL_SIZE = 8192;

_CLASSDEF(SafetyPool)

class _EXPORT SafetyPool
{
public:
    static  int		    Allocate(size_t size
				    = DEFAULT_SAFETY_POOL_SIZE);
    static  int		    IsExhausted()
			      // returns true if safetyPool is exhausted
			      { return safetyPool == NULL; };
    static  int		    Size;
    static  Pvoid 	    safetyPool;
};

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif // __SAFEPOOL_H
