// ObjectWindows - (C) Copyright 1992 by Borland International

/* ------------------------------------------------------------------------*/
/*   defines the classes TStreamable, TStreamableClass, pstream,           */
/*   ipstream, opstream, iopstream, ifpstream, ofpstream, and iofpstream.  */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if !defined( __OBJSTRM_H )
#define __OBJSTRM_H

#if !defined( __OBJECT_H )
#include <object.h>
#endif  // __OBJECT_H

#if !defined( __IOSTREAM_H )
#include <iostream.h>
#endif  // __IOSTREAM_H

#if !defined( __FSTREAM_H )
#include <fstream.h>
#endif  // __FSTREAM_H

#if !defined( __TCOLLECT_H )
#include <tcollect.h>
#endif  // __TCOLLECT_H

#if !defined( __STDLIB_H )
#include <stdlib.h>
#endif  // __STDLIB_H

#if !defined( __DOS_H )
#include <dos.h>
#endif // __DOS_H

#if  defined(__DLL__)
#  define _EXPFUNC _export
#else
#  define _EXPFUNC
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

#if !defined( __fLink_def )
#define __fLink_def
struct fLink
{
    fLink near *f;
    class _CLASSTYPE TStreamableClass near *t;
};
#endif

#define __link( s )             \
  extern TStreamableClass s;    \
  static fLink force ## s =     \
    { (fLink near *)&force ## s, (TStreamableClass near *)&s };

typedef unsigned P_id_type;

enum StreamableInit { streamableInit };

/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class TStreamable                                                     */
/*                                                                         */
/*   This is the base class for all storable objects.  It provides         */
/*   three member functions, streamableName(), read(), and write(), which  */
/*   must be overridden in every derived class.                            */
/*                                                                         */
/* ------------------------------------------------------------------------*/

_CLASSDEF(TStreamable)
_CLASSDEF(TStreamableClass)
_CLASSDEF(TStreamableTypes)
_CLASSDEF(TPWrittenObjects)
_CLASSDEF(TPWObj)
_CLASSDEF(TPReadObjects)
_CLASSDEF(pstream)
_CLASSDEF(ipstream)
_CLASSDEF(opstream)
_CLASSDEF(iopstream)
_CLASSDEF(fpbase)
_CLASSDEF(ifpstream)
_CLASSDEF(ofpstream)
_CLASSDEF(fpstream)

class _CLASSTYPE TStreamable
{

    friend class _CLASSTYPE opstream;
    friend class _CLASSTYPE ipstream;

private:

    virtual const Pchar streamableName() const = 0;

protected:

    virtual Pvoid read(Ripstream) = 0;
    virtual void write(Ropstream) = 0;
};

/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class TStreamableClass                                                */
/*                                                                         */
/*   Used internally by TStreamableTypes and pstream.                      */
/*                                                                         */
/* ------------------------------------------------------------------------*/

typedef PTStreamable (_FAR *BUILDER)();
#define __DELTA( d ) (FP_OFF((TStreamable *)(d *)1)-1)

class _CLASSTYPE TStreamableClass
{
    friend class _CLASSTYPE TStreamableTypes;
    friend class _CLASSTYPE opstream;
    friend class _CLASSTYPE ipstream;
    friend class _CLASSTYPE TObjStrmRegRecord;

public:

    TStreamableClass( PCchar n, BUILDER b, int d );

protected:
    PCchar name;
    BUILDER build;
    int delta;
};

class _CLASSTYPE TObjStrmRegRecord
{
    friend class TStreamableClass;
public:
    TObjStrmRegRecord(PTStreamableClass pTStreamableClass);
    int Validate();

    PTStreamableClass pTStreamableClass;
private:
    unsigned short SegmentLimit;
    unsigned short Checksum;
	Pvoid operator new (size_t s);
	void operator delete(Pvoid ptr);
};

/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class TStreamableTypes                                                */
/*                                                                         */
/*   Maintains a database of all registered types in the application.      */
/*   Used by opstream and ipstream to find the functions to read and       */
/*   write objects.                                                        */
/*                                                                         */
/* ------------------------------------------------------------------------*/

class _CLASSTYPE TStreamableTypes : private TNSSortedCollection
{

public:

    TStreamableTypes() : TNSSortedCollection( 5, 5 )
        {duplicates = True;}
    ~TStreamableTypes() { shouldDelete = False; }

#if !defined(__DLL__)
    void registerType( PCTStreamableClass d)
        { insert( (void *)d ); }
#else
    void registerType(TObjStrmRegRecord *pTObjStrmRegRecord);
#endif

    PCTStreamableClass lookup( PCchar );

    Pvoid operator new( size_t sz ) { return ::operator new( sz ); }
    Pvoid operator new( size_t, Pvoid arena ) { return arena; }

protected:
    virtual Boolean search( Pvoid key, ccIndex _FAR & index );

private:
    virtual Pvoid keyOf(Pvoid d);
    int compare( Pvoid, Pvoid );
};


/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class TPWObj                                                          */
/*                                                                         */
/*   Used internally by TPWrittenObjects.                                  */
/*                                                                         */
/* ------------------------------------------------------------------------*/

class _CLASSTYPE TPWObj
{
	
    friend class _CLASSTYPE TPWrittenObjects;
    friend class _CLASSTYPE TPReadObjects;
    friend class _CLASSTYPE ipstream;
    friend class _CLASSTYPE opstream;

private:

    TPWObj(PCvoid adr, P_id_type id ) : address( Pvoid(adr) ), ident( id ) {}

    Pvoid address;
    P_id_type ident;

};

/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class TPWrittenObjects                                                */
/*                                                                         */
/*   Maintains a database of all objects that have been written to the     */
/*   current object stream.                                                */
/*                                                                         */
/*   Used by opstream when it writes a pointer onto a stream to determine  */
/*   whether the object pointed to has already been written to the stream. */
/*                                                                         */
/* ------------------------------------------------------------------------*/

class _CLASSTYPE TPWrittenObjects : public TNSSortedCollection
{

    friend class _CLASSTYPE opstream;

public:

    void removeAll() { curId = 1; TNSSortedCollection::removeAll(); }

private:

    TPWrittenObjects() : TNSSortedCollection( 5, 5 ), curId( 1 ) {}
    ~TPWrittenObjects() { shouldDelete = False; }

    void registerObject( PCvoid adr );
    P_id_type find( PCvoid adr );

    Pvoid keyOf(Pvoid d) { return ((TPWObj *)d)->address; }
    int compare( Pvoid, Pvoid );

    P_id_type curId;

};

/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class TPReadObjects                                                   */
/*                                                                         */
/*   Maintains a database of all objects that have been read from the      */
/*   current persistent stream.                                            */
/*                                                                         */
/*   Used by ipstream when it reads a pointer from a stream to determine   */
/*   the address of the object being referred to.                          */
/*                                                                         */
/* ------------------------------------------------------------------------*/

class _CLASSTYPE TPReadObjects : public TNSCollection
{

    friend class _CLASSTYPE ipstream;

public:

    void removeAll() { curId = 1; TNSCollection::removeAll(); }

private:

    TPReadObjects() : TNSCollection( 5, 5 ), curId( 1 )
        { insert( 0 ); }    // prime it, so we won't use index 0
    ~TPReadObjects() { shouldDelete = False; }

    void registerObject( PCvoid adr );
    PCvoid find( P_id_type id ) { return at( id ); }
    P_id_type curId;

};

/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class pstream                                                         */
/*                                                                         */
/*   Base class for handling streamable objects.                           */
/*                                                                         */
/* ------------------------------------------------------------------------*/

class _CLASSTYPE pstream
{

    friend class _CLASSTYPE TStreamableTypes;

public:

    enum StreamableError { peNotRegistered = 0x1000, peInvalidType = 0x2000 };
    enum PointerTypes { ptNull, ptIndexed, ptObject };

    _Cdecl pstream( Pstreambuf sb ) { init( sb ); }
    virtual _Cdecl ~pstream() {}

    int _Cdecl rdstate() const { return state; }
    int _Cdecl eof() const { return state & ios::eofbit; }
    int _Cdecl fail() const
        { return state & (ios::failbit | ios::badbit | ios::hardfail); }
    int _Cdecl bad() const { return state & (ios::badbit | ios::hardfail); }
    int _Cdecl good() const { return state == 0; }

    void _Cdecl clear( int i = 0 )
        { state = (i & 0xFF) | (state & ios::hardfail); }

    _Cdecl operator Pvoid() const { return fail() ? 0 : (void *)this; }
    int _Cdecl operator ! () const { return fail(); }

	Pstreambuf _Cdecl rdbuf() const { return bp; }

	static void initTypes();
	static PTStreamableTypes types;

protected:

	_Cdecl pstream() {}

	void _Cdecl error( StreamableError );
	void _Cdecl error( StreamableError, RCTStreamable );

	Pstreambuf bp;
	int state;
	
	void _Cdecl init( Pstreambuf sbp) { state = 0; bp = sbp; }
	void _Cdecl setstate( int b) { state |= (b&0x00FF); }
};

/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class ipstream                                                        */
/*                                                                         */
/*   Base class for reading streamable objects                             */
/*                                                                         */
/* ------------------------------------------------------------------------*/

class _CLASSTYPE ipstream : virtual public pstream
{

public:

    _Cdecl  ipstream( Pstreambuf sb) { pstream::init( sb ); }
    _Cdecl ~ipstream() {}

    streampos _Cdecl tellg()
        { return bp->seekoff( 0, ios::cur, ios::in ); }

    Ripstream _Cdecl seekg( streampos );
    Ripstream _Cdecl seekg( streamoff, ios::seek_dir );

    uchar _Cdecl readByte() { return bp->sbumpc(); }
    void _Cdecl readBytes( Pvoid data, size_t sz)
        { bp->sgetn( (char *)data, sz ); }
    void _Cdecl freadBytes( void far *data, size_t sz );

    ushort _Cdecl readWord();
    Pchar _Cdecl readString();
    Pchar _Cdecl readString( Pchar, unsigned );
    char far *freadString();
    char far *freadString( char far *buf, unsigned maxLen );

    friend Ripstream _Cdecl _EXPFUNC operator >> ( Ripstream,
        signed char _FAR & );
    friend Ripstream _Cdecl _EXPFUNC operator >> ( Ripstream,
        unsigned char _FAR & );
    friend Ripstream _Cdecl _EXPFUNC operator >> ( Ripstream,
        signed short _FAR & );
    friend Ripstream _Cdecl _EXPFUNC operator >> ( Ripstream,
        unsigned short _FAR & );
    friend Ripstream _Cdecl _EXPFUNC operator >> ( Ripstream,
        signed int _FAR & );
    friend Ripstream _Cdecl _EXPFUNC operator >> ( Ripstream,
        unsigned int _FAR & );
    friend Ripstream _Cdecl _EXPFUNC operator >> ( Ripstream,
        signed long _FAR & );
    friend Ripstream _Cdecl _EXPFUNC operator >> ( Ripstream,
        unsigned long _FAR & );
    friend Ripstream _Cdecl _EXPFUNC operator >> ( Ripstream,
        float _FAR & );
    friend Ripstream _Cdecl _EXPFUNC operator >> ( Ripstream,
        double _FAR & );
    friend Ripstream _Cdecl _EXPFUNC operator >> ( Ripstream,
        long double _FAR & );

    friend Ripstream _Cdecl _EXPFUNC operator >> ( Ripstream, RTStreamable );
    friend Ripstream _Cdecl _EXPFUNC operator >> ( Ripstream, RPvoid);

protected:

    _Cdecl ipstream() {}

    PCTStreamableClass _Cdecl readPrefix();
    Pvoid _Cdecl readData( PCTStreamableClass, PTStreamable );
    void _Cdecl readSuffix();

    PCvoid _Cdecl find( P_id_type id) { return objs.find( id ); }
    void _Cdecl registerObject( PCvoid adr ) { objs.registerObject( adr ); }

private:

    TPReadObjects objs;

};

/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class opstream                                                        */
/*                                                                         */
/*   Base class for writing streamable objects                             */
/*                                                                         */
/* ------------------------------------------------------------------------*/

class _CLASSTYPE opstream : virtual public pstream
{

public:

    _Cdecl opstream( Pstreambuf sb) { pstream::init( sb ); }
    _Cdecl ~opstream() {}

	streampos _Cdecl tellp() { return bp->seekoff( 0, ios::cur, ios::out ); }
	Ropstream _Cdecl seekp( streampos pos);
	Ropstream _Cdecl seekp( streamoff off, ios::seek_dir dir);
	Ropstream _Cdecl flush();

	void _Cdecl writeByte( uchar ch) { bp->sputc( ch ); }
	void _Cdecl writeBytes( PCvoid data, size_t sz)
		{ bp->sputn( (char *)data, sz ); }
	void _Cdecl fwriteBytes( const void far *data, size_t sz );

    void _Cdecl writeWord( ushort sh)
        { bp->sputn( (char *)&sh, sizeof( ushort ) ); }

    void _Cdecl writeString( PCchar );
    void _Cdecl fwriteString( const char far * str );

    friend Ropstream _Cdecl _EXPFUNC operator << ( Ropstream, signed char );
    friend Ropstream _Cdecl _EXPFUNC operator << ( Ropstream, unsigned char );
    friend Ropstream _Cdecl _EXPFUNC operator << ( Ropstream, signed short );
    friend Ropstream _Cdecl _EXPFUNC operator << ( Ropstream, unsigned short );
    friend Ropstream _Cdecl _EXPFUNC operator << ( Ropstream, signed int );
    friend Ropstream _Cdecl _EXPFUNC operator << ( Ropstream, unsigned int );
    friend Ropstream _Cdecl _EXPFUNC operator << ( Ropstream, signed long );
    friend Ropstream _Cdecl _EXPFUNC operator << ( Ropstream, unsigned long );
    friend Ropstream _Cdecl _EXPFUNC operator << ( Ropstream, float );
    friend Ropstream _Cdecl _EXPFUNC operator << ( Ropstream, double );
    friend Ropstream _Cdecl _EXPFUNC operator << ( Ropstream, long double );

    friend Ropstream _Cdecl _EXPFUNC operator << ( Ropstream, RTStreamable );
    friend Ropstream _Cdecl _EXPFUNC operator << ( Ropstream, PTStreamable );

protected:

    _Cdecl opstream() {}

    void _Cdecl writePrefix( RCTStreamable );
    void _Cdecl writeData( RTStreamable );
    void _Cdecl writeSuffix( RCTStreamable )
        { writeByte( ']' ); }

    P_id_type _Cdecl find( PCvoid adr ) { return objs.find( adr ); }
    void _Cdecl registerObject( PCvoid adr ) { objs.registerObject( adr ); }

private:
	TPWrittenObjects objs;

};

/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class iopstream                                                       */
/*                                                                         */
/*   Base class for reading and writing streamable objects                 */
/*                                                                         */
/* ------------------------------------------------------------------------*/

class _CLASSTYPE iopstream : public ipstream, public opstream
{

public:

	_Cdecl iopstream( Pstreambuf sb) { pstream::init( sb ); }
	_Cdecl ~iopstream() {}

protected:

	_Cdecl iopstream() {}
};

/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class fpbase                                                          */
/*                                                                         */
/*   Base class for handling streamable objects on file streams            */
/*                                                                         */
/* ------------------------------------------------------------------------*/

class _CLASSTYPE fpbase : virtual public pstream
{
	
public:

	_Cdecl fpbase() { pstream::init( &buf ); }
	_Cdecl fpbase( PCchar, int, int = filebuf::openprot );
	_Cdecl fpbase( int f ) : buf (f) { pstream::init( &buf ); }
	_Cdecl fpbase( int f, Pchar b, int len) : buf( f, b, len )
		{ pstream::init( &buf ); }
	_Cdecl ~fpbase() {}

	void _Cdecl open( PCchar, int, int = filebuf::openprot );
	void _Cdecl attach( int );
	void _Cdecl close();
	void _Cdecl setbuf( Pchar, int );
	Pfilebuf _Cdecl rdbuf() { return &buf; }
	
private:

	filebuf buf;

};

/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class ifpstream                                                       */
/*                                                                         */
/*   Base class for reading streamable objects from file streams           */
/*                                                                         */
/* ------------------------------------------------------------------------*/

class _CLASSTYPE ifpstream : public fpbase, public ipstream
{

public:

	_Cdecl ifpstream() {}
	_Cdecl ifpstream( PCchar name,
                      int mode = ios::in,
                      int prot = filebuf::openprot
                    );
	_Cdecl ifpstream(int f) : fpbase( f ) {}
	_Cdecl ifpstream( int f, Pchar b, int len) : fpbase(f, b, len) {}
	_Cdecl ~ifpstream() {}

    Pfilebuf _Cdecl rdbuf() { return fpbase::rdbuf(); }
    void _Cdecl open( PCchar,
                      int = ios::in,
                      int = filebuf::openprot
					);
};

/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class ofpstream                                                       */
/*                                                                         */
/*   Base class for writing streamable objects to file streams             */
/*                                                                         */
/* ------------------------------------------------------------------------*/

class _CLASSTYPE ofpstream : public fpbase, public opstream
{

public:

	_Cdecl ofpstream() {}
	_Cdecl ofpstream( PCchar,
					  int = ios::out,
                      int = filebuf::openprot
                    );
	_Cdecl ofpstream( int f) : fpbase( f ) {}
	_Cdecl ofpstream( int f, Pchar b, int len ) : fpbase(f, b, len) {}
	_Cdecl ~ofpstream() {}

    Pfilebuf _Cdecl rdbuf() { return fpbase::rdbuf(); }
    void _Cdecl open( PCchar,
                      int = ios::out,
                      int = filebuf::openprot
                    );

};

/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   class fpstream                                                        */
/*                                                                         */
/*   Base class for reading and writing streamable objects to              */
/*   bidirectional file streams                                            */
/*                                                                         */
/* ------------------------------------------------------------------------*/

class _CLASSTYPE fpstream : public fpbase, public iopstream
{

public:

	_Cdecl fpstream() {}
	_Cdecl fpstream( PCchar, int, int = filebuf::openprot );
	_Cdecl fpstream( int f) : fpbase( f ) {}
	_Cdecl fpstream( int f, Pchar b, int len ) : fpbase(f, b, len) {}
	_Cdecl ~fpstream() {}

	Pfilebuf _Cdecl rdbuf() { return fpbase::rdbuf(); }
	void _Cdecl open( PCchar, int, int = filebuf::openprot );

};

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif  // __OBJSTRM_H

