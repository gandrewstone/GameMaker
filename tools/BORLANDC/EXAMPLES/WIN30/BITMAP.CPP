// Borland C++ - (C) Copyright 1991, 1992 by Borland International
//
// This file is used in the DLLDEMO program. First build BITMAP.DLL
// and then DLLDEMO. There are project files provided to build each
// of these.

#define  STRICT
#include <windows.h>

#define _EXPORT _export
#include "bitmap.h"

// Implementation of CompatibleDC and Bitmap classes.

class CompatibleDC
{
    private:
        HDC hDCMem;
    public:
        // Create a memory device context, specify a selected object,
        // and set the DC's mapping mode.
        CompatibleDC( HDC hDC )
        {
            hDCMem = CreateCompatibleDC( hDC );
            SetMapMode( hDCMem, GetMapMode( hDC ) );
        }
        ~CompatibleDC( void ) { DeleteDC( hDCMem ); };
        HDC Get_hDCMem( void ) { return hDCMem; }
};

void FAR _export Bitmap::Display( HDC hDC, short xStart, short yStart )
{
    POINT ptSize, ptOrigin;

    CompatibleDC MemoryDC( hDC );
    HDC hDCMem = MemoryDC.Get_hDCMem();
    SelectObject( hDCMem, hBitmap );

    ptSize = GetSize( hDC );
    ptOrigin.x = 0;
    ptOrigin.y = 0;
    DPtoLP( hDCMem, &ptOrigin, 1 );

    BitBlt( hDC, xStart, yStart, ptSize.x, ptSize.y,
            hDCMem, ptOrigin.x, ptOrigin.y, SRCCOPY );
}
