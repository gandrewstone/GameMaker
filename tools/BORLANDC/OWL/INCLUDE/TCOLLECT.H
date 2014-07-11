// ObjectWindows - (C) Copyright 1992 by Borland International

#if !defined( __TCOLLECT_H )
#define __TCOLLECT_H

#if !defined( __OBJECT_H )
#include <object.h>
#endif  // __OBJECT_H

#if !defined( __OWLDEFS_H )
#include <owldefs.h>
#endif  // __OWLDEFS_H

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

enum Boolean { False, True };

typedef unsigned short ushort;
typedef unsigned char uchar;

const char EOS = '\0';

typedef int ccIndex;
typedef Boolean (_FAR *ccTestFunc)( Pvoid, Pvoid );
typedef void (_FAR *ccAppFunc)( Pvoid, Pvoid );

const ccNotFound = -1;
const maxCollectionSize = (int)((65536uL - 16)/sizeof( Pvoid ));

const TCOLLECTION_CLASS_HASH_VALUE = 0;

_CLASSDEF(TNSCollection)
_CLASSDEF(TNSSortedCollection)

class _CLASSTYPE TNSCollection
{
public:

    TNSCollection( ccIndex aLimit, ccIndex aDelta );
    ~TNSCollection();

    Pvoid at( ccIndex index );
    virtual ccIndex indexOf( Pvoid item );

    void atFree( ccIndex index );
    void atRemove( ccIndex index );
    void remove( Pvoid item );
    void removeAll();
    void free( Pvoid item );
    void freeAll();

    void atInsert( ccIndex index, Pvoid item );
    void atPut( ccIndex index, Pvoid item );
    virtual ccIndex insert( Pvoid item );

    static void error( ccIndex code, ccIndex info );

    Pvoid firstThat( ccTestFunc Test, Pvoid arg );
    Pvoid lastThat( ccTestFunc Test, Pvoid arg );
    void forEach( ccAppFunc action, Pvoid arg );

    void pack();
    virtual void setLimit( ccIndex aLimit );

    ccIndex getCount()
        { return count; }

protected:

    TNSCollection();

    Pvoid _FAR *items;
    ccIndex count;
    ccIndex limit;
    ccIndex delta;
    Boolean shouldDelete;

private:

    virtual void freeItem( Pvoid item );
};

class _CLASSTYPE TNSSortedCollection: public virtual TNSCollection
{
public:

    TNSSortedCollection( ccIndex aLimit, ccIndex aDelta) :
	    TNSCollection( aLimit, aDelta )
	    { delta = aDelta; setLimit( aLimit ); }

    virtual Boolean search( Pvoid key, ccIndex _FAR & index );

    virtual ccIndex indexOf( Pvoid item );
    virtual ccIndex insert( Pvoid item );

    Boolean duplicates;

protected:

    TNSSortedCollection() {};
    virtual Pvoid keyOf( Pvoid item ) { return item; }

private:

    virtual int compare( Pvoid key1, Pvoid key2 ) = 0;
};

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif // __TCOLLECT_H
