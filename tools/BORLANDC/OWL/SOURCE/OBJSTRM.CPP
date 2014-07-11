// Borland C++ - (C) Copyright 1992 by Borland International

/*------------------------------------------------------------*/
/* filename -       objstrm.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  Member function(s) of following classes:  */
/*                     TStreamable                            */
/*                     TStreamableClass                       */
/*                     TStreamableTypes                       */
/*                     TPWrittenObjects                       */
/*                     TPReadObjects                          */
/*                     pstream                                */
/*                     ipstream                               */
/*                     opstream                               */
/*                     iopstream                              */
/*                     fpbase                                 */
/*                     ifpstream                              */
/*                     ofpstream                              */
/*                     fpstream                               */
/*------------------------------------------------------------*/

#if !defined( __OBJSTRM_H )
#include <objstrm.h>
#endif	// __OBJSTRM_H

#if !defined( __STRING_H )
#include <string.h>
#endif  // __STRING_H

#if !defined( __FSTREAM_H )
#include <fstream.h>
#endif  // __FSTREAM_H

#if !defined( __IO_H )
#include <io.h>
#endif  // __IO_H

#if !defined( __STAT_H )
#include <sys\Stat.h>
#endif  // __STAT_H

#if !defined( __FCNTL_H )
#include <fcntl.h>
#endif  // __FCNTL_H

#if !defined( __ASSERT_H )
#include <assert.h>
#endif  // __ASSERT_H

#if !defined(__ALLOC_H)
#include <alloc.h>
#endif // __ALLOC_H










#if !defined(__APPLICAT_H)
#include <applicat.h>
#endif

#if !defined(__WINDOBJ_H)
#include <windobj.h>
#endif

#if defined(__DLL__)
extern "C"
	unsigned _WinAllocFlag;
#endif

TStreamableTypes *pstream::types;

#if defined(__DLL__)

static unsigned short GetSegmentLimit(Pvoid pMem)
{
    unsigned short Temp = FP_SEG(pMem);     // Temp = selector
    asm lsl ax, Temp;                       // ax = segment limit
    asm mov Temp, ax;                       // Temp = segment limit
    return Temp;
}

static unsigned short GetChecksum(Pvoid pMem, unsigned short SegmentLimit)
{
    unsigned long TempChecksum = 0;     // intialize checksum

    SegmentLimit -= FP_OFF(pMem);       // adjust for starting offset

    // Use either 64 bytes (32 words) or the segment limit, whichever
    // is smaller
    int ChecksumLimit = (SegmentLimit < 64) ? (SegmentLimit>>1) : 32;

    unsigned short _FAR *p = (unsigned short _FAR *)pMem;

    for (int i = 0; i < ChecksumLimit; i++)
             TempChecksum += *p++;

    return (unsigned short)TempChecksum;
}

TObjStrmRegRecord::TObjStrmRegRecord(PTStreamableClass pTStreamableClass)
{
    this->pTStreamableClass = pTStreamableClass;
    SegmentLimit = GetSegmentLimit(pTStreamableClass->build);
    Checksum = GetChecksum(pTStreamableClass->build, SegmentLimit);
}

Pvoid TObjStrmRegRecord::operator new (size_t s)
{
    return (Pvoid)LocalLock(LocalAlloc(LPTR, s));
}

void TObjStrmRegRecord::operator delete(Pvoid ptr)
{
    HANDLE hMem =
#ifdef STRICT	
           LocalHandle( (void NEAR *)FP_OFF( ptr ) );	
#else
           LocalHandle( FP_OFF( ptr ) );
#endif
    if (LocalUnlock(hMem))
        LocalFree(hMem);
}

int TObjStrmRegRecord::Validate()
{
    WORD Temp = FP_SEG(pTStreamableClass);
    asm lar ax, Temp;           // get access rights
    asm jnz invalid;            // invalid segment

    asm and ax, 0x0800;         // code or data?
    asm jnz invalid;            // code, invalid

    // We can access the pTStreamableClass pointer.  See if segment is
    // big enough to contain a TStreamableClass object.

    long TempSegmentLimit = GetSegmentLimit(pTStreamableClass);
    if (TempSegmentLimit-FP_OFF(pTStreamableClass) <=
        sizeof(TStreamableClass)-1)
        return 0;

    // We can access the TStreamableClass object and the segment in
    // which it resides is big enough to hold a TStreamableClass

    Temp = FP_SEG(pTStreamableClass->build);
    asm lar ax, Temp;           // get access rights
    asm jnz invalid;            // invalid selector

    asm and ax, 0x0800;         // code or data
    asm jz invalid;             // data, invalid

    unsigned short ActualSegmentLimit =
        GetSegmentLimit(pTStreamableClass->build);

    if (ActualSegmentLimit != SegmentLimit)
    {
    invalid:
        return 0;
    }
    else
        return (GetChecksum(pTStreamableClass->build,
                ActualSegmentLimit) == Checksum);
}

#endif // __DLL__

TStreamableClass::TStreamableClass( const char *n, BUILDER b, int d ) :
    name( n ),
    build( b ),
    delta( d )
{
    pstream::initTypes();

#if defined(__DLL__)

    TObjStrmRegRecord * pTObjStrmRegRecord =
        new TObjStrmRegRecord(this);

    pstream::types->registerType(pTObjStrmRegRecord);
#else
    pstream::types->registerType(this);
#endif
}

#if defined(__DLL__)
void TStreamableTypes::registerType(TObjStrmRegRecord *pTObjStrmRegRecord)
{
    unsigned SavedWinAllocFlag = _WinAllocFlag;
    _WinAllocFlag |= GMEM_DDESHARE;

    insert((void *)pTObjStrmRegRecord);

    _WinAllocFlag = SavedWinAllocFlag;
}
#endif

Boolean TStreamableTypes::search( void *key, ccIndex& index )
{
    ccIndex l = 0;
    ccIndex h = count - 1;
    while( l <= h )
    {
        ccIndex i = (l +  h) >> 1;
        // i is our midpoint
#if defined(__DLL__)

        TObjStrmRegRecord * pTObjStrmRegRecord =
            (TObjStrmRegRecord *)items[i];

        if (!(pTObjStrmRegRecord->Validate()))
        {
            // remove this entry
            count--;
            memmove(&items[i], &items[i+1], (count-i)*sizeof(void *));
            h--;
            continue;           // do the while again
        }
#endif
        ccIndex c = compare( keyOf( items[i] ), key );
        if (c < 0)
            l = i + 1;
        else if (c > 0)
            h = i - 1;
        else
        {
            index = i;
            return True;
        }
    }
    index = l;
    return False;
}

const TStreamableClass *TStreamableTypes::lookup( const char *name )
{
    ccIndex loc;

    if (!search((void *)name, loc))
        return NULL;

#if defined(__DLL__)
    return ((TObjStrmRegRecord *)at(loc))->pTStreamableClass;
#else
    return (PTStreamableClass)at(loc);
#endif
}

Pvoid TStreamableTypes::keyOf(Pvoid d)
{
#if defined(__DLL__)
    TObjStrmRegRecord * pTObjStrmRegRecord = (TObjStrmRegRecord *)d;
    return (void *)((pTObjStrmRegRecord->pTStreamableClass)->name);
#else
    return (void *)((PTStreamableClass)d)->name;
#endif
}

int TStreamableTypes::compare( void *d1, void *d2 )
{
    return strcmp( (char *)d1, (char *)d2 );
}

void TPWrittenObjects::registerObject( const void *adr )
{
    TPWObj *o = new TPWObj( adr, curId++ );
    insert( o );
}

P_id_type TPWrittenObjects::find( const void *d )
{
    ccIndex loc;
    if( search( Pvoid( d ), loc ) )
        return ((TPWObj *)at( loc ))->ident;
    else
        return 0;
}

int TPWrittenObjects::compare( void *o1, void *o2 )
{
    if( o1 == o2 )
        return 0;
    else if( (void huge *)o1 < (void huge *)o2 )
        return -1;
    else
        return 1;
}

void TPReadObjects::registerObject( const void *adr )
{
    ccIndex loc = insert( Pvoid( adr ));
    assert( loc == curId++ );   // to be sure that TNSCollection
                                // continues to work the way
                                // it does now...
}

void _Cdecl pstream::error( StreamableError errorcondition )
{
  setstate( errorcondition );

  if(errorcondition == peNotRegistered)
    MessageBox(	
      GetApplicationObject()->MainWindow->HWindow,
      "Type Not Registered", "PStream Error Condition",
	      MB_OK | MB_ICONEXCLAMATION);
  else
  {
    assert(errorcondition == peInvalidType);
    MessageBox(	
      GetApplicationObject()->MainWindow->HWindow,
      "Invalid Type Encountered", "PStream Error Condition",
	      MB_OK | MB_ICONEXCLAMATION);
  }

  clear(ios::badbit);
}

void _Cdecl pstream::error( StreamableError errorcondition, RCTStreamable )
{
  error( errorcondition );
}

void pstream::initTypes()
{
    if( types == NULL )
    {
#if defined(__DLL__)
    unsigned SavedWinAllocFlag = _WinAllocFlag;
    _WinAllocFlag |= GMEM_DDESHARE;
#endif

        types = new TStreamableTypes;

#if defined(__DLL__)
    _WinAllocFlag = SavedWinAllocFlag;
#endif
    }
}

ipstream& ipstream::seekg( streampos pos )
{
    objs.removeAll();
    objs.insert( new TPWObj( 0, 0 ));
    bp->seekoff( pos, ios::beg );
    return *this;
}

ipstream& ipstream::seekg( streamoff off, ios::seek_dir dir )
{
    objs.removeAll();
    objs.insert( new TPWObj( 0, 0 ));
    bp->seekoff( off, dir );
    return *this;
}

ushort ipstream::readWord()
{
    ushort temp;
    bp->sgetn( (char *)&temp, sizeof( ushort ) );
    return temp;
}

void ipstream::freadBytes( void far *data, size_t sz )
{
    if (sz > 0)
    {
        char *buf = new char[sz+1];

        bp->sgetn( (char *)buf, sz );
        _fstrncpy((char far *)data, buf, sz);

        delete buf;
    }
}

char *ipstream::readString()
{
    uchar len = readByte();

    char *buf = new char[len+1];

    if( buf != 0 )
    {
        readBytes( buf, len );
        buf[len] = EOS;
    }
    return buf;
}

char *ipstream::readString( char *buf, unsigned maxLen )
{
    assert( buf != 0 );

    uchar len = readByte();

    if( len > maxLen-1 )
        len = maxLen;           // but see buf[len] = EOS;

    readBytes( buf, len );
    buf[len] = EOS;
    return buf;
}

char far *ipstream::freadString( char far *buf, unsigned maxLen )
{
    assert (buf != 0 );

    uchar len = readByte();

    if( len > maxLen-1 )
        len = maxLen;           // but see buf[len] = EOS;

    freadBytes( buf, len);
    buf[len] = EOS;
    return buf;
}

char far *ipstream::freadString()
{
    uchar len = readByte();

    char far *buf = (char far *)farmalloc(len+1);

    freadBytes(buf, len);
    buf[len] = EOS;
    return buf;
}

ipstream& operator >> ( ipstream& ps, signed char &ch )
{
    ch = ps.readByte();
    return ps;
}

ipstream& operator >> ( ipstream& ps, unsigned char &ch )
{
    ch = ps.readByte();
    return ps;
}

ipstream& operator >> ( ipstream& ps, signed short &sh )
{
    sh = ps.readWord();
    return ps;
}

ipstream& operator >> ( ipstream& ps, unsigned short &sh )
{
    sh = ps.readWord();
    return ps;
}

ipstream& operator >> ( ipstream& ps, signed int &i )
{
    i = ps.readWord();
    return ps;
}

ipstream& operator >> ( ipstream& ps, unsigned int &i )
{
    i = ps.readWord();
    return ps;
}

ipstream& operator >> ( ipstream& ps, signed long &l )
{
    ps.readBytes( &l, sizeof(l) );
    return ps;
}

ipstream& operator >> ( ipstream& ps, unsigned long &l )
{
    ps.readBytes( &l, sizeof(l) );
    return ps;
}

ipstream& operator >> ( ipstream& ps, float &l )
{
    ps.readBytes( &l, sizeof(l) );
    return ps;
}

ipstream& operator >> ( ipstream& ps, double &l )
{
    ps.readBytes( &l, sizeof(l) );
    return ps;
}

ipstream& operator >> ( ipstream& ps, long double &l )
{
    ps.readBytes( &l, sizeof(l) );
    return ps;
}

ipstream& operator >> ( ipstream& ps, TStreamable& t )
{
    const TStreamableClass *pc;

    if((pc = ps.readPrefix()) == NULL)
      ps.error(pstream::peInvalidType );
    ps.readData( pc, &t );
    ps.readSuffix();
    return ps;
}

ipstream& operator >> ( ipstream& ps, void *&t )
{
    char ch = ps.readByte();
    switch( ch )
        {
        case pstream::ptNull:
            t = 0;
            break;
        case pstream::ptIndexed:
            {
            P_id_type index = ps.readWord();
	    t = Pvoid( ps.find( index ) );
            assert( t != 0 );
            break;
            }
        case pstream::ptObject:
            {
            const TStreamableClass *pc;
	
	    if((pc = ps.readPrefix()) == NULL)
              ps.error( pstream::peInvalidType );

            t = ps.readData( pc, 0 );
            ps.readSuffix();
            break;
            }
        default:
            ps.error( pstream::peInvalidType );
            break;
        }
    return ps;
}

const TStreamableClass *ipstream::readPrefix()
{
    char ch = readByte();

    assert( ch == '[' );    // don't combine this with the previous line!
                            // We must always do the read, even if we're
			    // not checking assertions

    char name[125];
    readString( name, sizeof name );
    return types->lookup( name );
}

void *ipstream::readData( const TStreamableClass *c, TStreamable *mem )
{
    if( mem == 0 )
        mem = c->build();

    registerObject( (char *)mem - c->delta );   // register the actual address
                                        // of the object, not the address
                                        // of the TStreamable sub-object
    return mem->read( *this );
}

void ipstream::readSuffix()
{
    char ch = readByte();

    assert( ch == ']' );    // don't combine this with the previous line!
                            // We must always do the write, even if we're
                            // not checking assertions
}

opstream& opstream::seekp( streampos pos )
{
    objs.removeAll();
    objs.insert( new TPWObj( 0, 0 ));
    bp->seekoff( pos, ios::beg );
    return *this;
}

opstream& opstream::seekp( streamoff pos, ios::seek_dir dir )
{
    objs.removeAll();
    objs.insert( new TPWObj( 0, 0 ));
    bp->seekoff( pos, dir );
    return *this;
}

opstream& opstream::flush()
{
    bp->sync();
    return *this;
}

void opstream::fwriteBytes( const void far *data, size_t sz )
{
    if (sz > 0)
    {
        char *buf = new char[sz+1];

        _fstrcpy(buf, (char far *)data);
        bp->sputn( (char *)buf, sz );

        delete buf;
    }
}

void opstream::writeString( const char *str )
{
    int len = strlen( str );
    writeByte( (uchar)len );
    writeBytes( str, len );
}

void opstream::fwriteString( const char far * str )
{
    int len = _fstrlen( str );
    writeByte( (uchar)len );
    fwriteBytes(str, len);
}

opstream& operator << ( opstream& ps, signed char ch )
{
    ps.writeByte( ch );
    return ps;
}

opstream& operator << ( opstream& ps, unsigned char ch )
{
    ps.writeByte( ch );
    return ps;
}

opstream& operator << ( opstream& ps, signed short sh )
{
    ps.writeWord( sh );
    return ps;
}

opstream& operator << ( opstream& ps, unsigned short sh )
{
    ps.writeWord( sh );
    return ps;
}

opstream& operator << ( opstream& ps, signed int i )
{
    ps.writeWord( i );
    return ps;
}

opstream& operator << ( opstream& ps, unsigned int i )
{
    ps.writeWord( i );
    return ps;
}

opstream& operator << ( opstream& ps, signed long l )
{
    ps.writeBytes( &l, sizeof(l) );
    return ps;
}

opstream& operator << ( opstream& ps, unsigned long l )
{
    ps.writeBytes( &l, sizeof(l) );
    return ps;
}

opstream& operator << ( opstream& ps, float l )
{
    ps.writeBytes( &l, sizeof(l) );
    return ps;
}

opstream& operator << ( opstream& ps, double l )
{
    ps.writeBytes( &l, sizeof(l) );
    return ps;
}

opstream& operator << ( opstream& ps, long double l )
{
    ps.writeBytes( &l, sizeof(l) );
    return ps;
}

opstream& operator << ( opstream& ps, TStreamable& t )
{
    ps.writePrefix( t );
    ps.writeData( t );
    ps.writeSuffix( t );
    return ps;
}

opstream& operator << ( opstream& ps, TStreamable *t )
{
    P_id_type index;
    if( t == 0 )
        ps.writeByte( pstream::ptNull );
    else if( (index = ps.find( t )) != 0 )
        {
        ps.writeByte( pstream::ptIndexed );
        ps.writeWord( index );
        }
    else
        {
        ps.writeByte( pstream::ptObject );
        ps << *t;
        }
    return ps;
}

void opstream::writePrefix( const TStreamable& t )
{
    writeByte( '[' );
    writeString( t.streamableName() );
}

void opstream::writeData( TStreamable& t )
{
    if ( types->lookup( t.streamableName() ) == NULL )
        error( peNotRegistered, t );
    else
    {
        registerObject( &t );
        t.write( *this );
    }
}

fpbase::fpbase( const char *name, int omode, int prot )
{
    pstream::init( &buf );
    open( name, omode, prot );
}

void fpbase::open( const char *b, int m, int prot )
{
    if( buf.is_open() )
        clear(ios::failbit);        // fail - already open
    else if( buf.open(b, m, prot) )
    {
      clear(ios::goodbit);
    }
    else
        clear(ios::badbit);        // open failed
}

void fpbase::attach( int f )
{
    if( buf.is_open() )
        setstate(ios::failbit);
    else if( buf.attach(f) )
        clear(ios::goodbit);
    else
        clear(ios::badbit);
}

void fpbase::close()
{
    if( buf.close() )
        clear(ios::goodbit);
    else
        setstate(ios::failbit);
}

void fpbase::setbuf(char* b, int len)
{
    if( buf.setbuf(b, len) )
        clear(ios::goodbit);
    else
        setstate(ios::failbit);
}

ifpstream::ifpstream( const char* name, int omode, int prot ) :
        fpbase( name, omode | ios::in | ios::binary, prot )
{
}

void ifpstream::open( const char _FAR *name, int omode, int prot )
{
    fpbase::open( name, omode | ios::in | ios::binary, prot );
}

ofpstream::ofpstream( const char* name, int omode, int prot ) :
        fpbase( name, omode | ios::out | ios::binary, prot )
{
}

void ofpstream::open( const char _FAR *name, int omode, int prot )
{
    fpbase::open( name, omode | ios::out | ios::binary, prot );
}

fpstream::fpstream( const char* name, int omode, int prot ) :
        fpbase( name, omode | ios::out | ios::binary, prot )
{
}

void fpstream::open( const char _FAR *name, int omode, int prot )
{
    fpbase::open( name, omode | ios::in | ios::out | ios::binary, prot );
}



