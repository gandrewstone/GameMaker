// Borland C++ - (C) Copyright 1991, 1992 by Borland International

#ifndef __BITMAP_H
#define __BITMAP_H

#include <windows.h>

// Interface to simple library of classes to use for Windows GDI.

class _EXPORT Bitmap
{
    private:
        HANDLE hBitmap;
        int GetBitmap( BITMAP FAR * lpbm )
        {
            return GetObject( hBitmap, sizeof( BITMAP ), (LPSTR) lpbm );
        }
    public:
	Bitmap( HINSTANCE hInstance, char FAR * lpszBitmapName )
        {
            hBitmap = LoadBitmap( hInstance, lpszBitmapName );
        }
        ~Bitmap( void )
        {
            DeleteObject( hBitmap );
        }
        void FAR Display( HDC hDC, short xStart, short yStart );
        // Get the size of the bitmap in logical coordinates.
        POINT GetSize( HDC hDC )
        {
            BITMAP bm;
            POINT ptSize;

            GetBitmap( &bm );
            ptSize.x = bm.bmWidth;
            ptSize.y = bm.bmHeight;
            DPtoLP( hDC, &ptSize, 1 );
            return ptSize;
        }
};

#endif  // __BITMAP_H
