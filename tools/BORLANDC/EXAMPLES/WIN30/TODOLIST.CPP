//---------------------------------------------------------------------
//
//  TODOLIST.CPP - part of TODO example program
//
//      Copyright (c) 1991, 1992 by Borland International
//      All Rights Reserved.
//
//---------------------------------------------------------------------


#define STRICT

#if !defined( __STRSTREAM_H )
#include <strstream.h>
#endif  // __STRSTREAM_H

#if !defined( __FSTREAM_H )
#include <fstream.h>
#endif  // __FSTREAM_H

#if !defined( __CTYPE_H )
#include <ctype.h>
#endif  // __CTYPE_H

#if !defined( __TODOLIST_H )
#include "TodoList.h"
#endif  // __TODOLIST_H

#if !defined( __TODODEFS_H )
#include "TodoDefs.h"
#endif  // __TODODEFS_H

#if !defined( __TODODLGS_H )
#include "TodoDlgs.h"
#endif  // __TODODLGS_H

#include <string.h>
//---------------------------------------------------------------------
//
//  member functions and static data for class TdDate
//
//---------------------------------------------------------------------

static char *MonthNames[] =
    {
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
    };

//---------------------------------------------------------------------
//
//  TdDate::TdDate( const char *str );
//
//  constructor that takes a text string.  It's not very bright.
//
//  Format:
//
//      mmm[m*] d[d], yyyy
//
//      mmm must match the first three characters of a valid month
//          name.  Any contiguous characters after these are ignored.
//      dd  must be in the range 1..31.
//      yyyy isn't checked.
//
//  If an invalid date is entered, TdDate will get the current system
//  date.
//
//---------------------------------------------------------------------

TdDate::TdDate( const char *str ) : Date()
{
    unsigned M = 0;
    while( M < 12 && strnicmp( str, MonthNames[M], 3 ) != 0 )
        M++;
    if( M >= 12 )
        return;

    while( !isspace( *str ) )   // skip month name
        str++;
    while( isspace( *str ) )    // skip trailing whitespace
	str++;

    unsigned D, Y;
    istrstream is( (char *)str );
    is >> D;
    if( D < 1 || D > 31 )
        return;

    is.get();                   // skip the comma

    is >> Y;
    SetMonth( M + 1 );  // 1-based months!
    SetDay( D );
    SetYear( Y );
}

//---------------------------------------------------------------------
//
//  member functions for class TodoEntry
//
//---------------------------------------------------------------------

int TodoEntry::isEqual( const Object& o ) const
{
    return dateDue == ((const TodoEntry&)o).dateDue;
}

int TodoEntry::isLessThan( const Object& o ) const
{
    return dateDue < ((const TodoEntry&)o).dateDue;
}

classType TodoEntry::isA() const
{
    return TodoEntryClass;
}

char *TodoEntry::nameOf() const
{
    return "Todo Entry";
}

hashValueType TodoEntry::hashValue() const
{
    return dateDue.hashValue();
}

//---------------------------------------------------------------------
//
//  void TodoEntry::printOn( ostream& os ) const;
//
//  puts a TodoEntry onto an ostream in text form.
//
//---------------------------------------------------------------------

void TodoEntry::printOn( ostream& os ) const
{
    char temp[ 256 ];
    ostrstream tstr( temp, sizeof temp );
    tstr << priority
         << '\t'
         << dateCreated
         << '\t'
         << dateDue
         << '\t'
	 << ( (text == 0) ? "" : text )
         << ends;
    os << temp;
}

//---------------------------------------------------------------------
//
//  istream& operator >> ( istream& is, TodoEntry& td );
//  ostream& operator << ( ostream& os, TodoEntry& td );
//
//  inserter and extractor for TodoEntry.  These work together to
//  write entries out to a stream and read them back in.
//
//---------------------------------------------------------------------

istream& operator >> ( istream& is, TodoEntry& td )
{
    is >> td.priority;
    int m, d, y;
    if( is >> m >> d >> y )
	td.dateDue = Date( m, d, y );
    if( is >> m >> d >> y )
	td.dateCreated = Date( m, d, y );
    char text[ 128 ];
    if( is.getline( text, sizeof text ) )
        td.text = strdup( text );
    td.dirty = FALSE;
    return is;
}

ostream& operator << ( ostream& os, TodoEntry& td )
{
    os << td.priority << " "
       << td.dateDue.Month() << " " << td.dateDue.Day() << " "
       << td.dateDue.Year() << " "
       << td.dateCreated.Month() << " " << td.dateCreated.Day() << " "
       << td.dateCreated.Year() << " " << td.text << endl;
    if( os )
        td.dirty = FALSE;
    return os;
}

//---------------------------------------------------------------------
//
//  member functions for class TodoList.
//
//---------------------------------------------------------------------

void TodoList::add( Object& o )
{
    dirty = TRUE;               // mark that the list has been modified
    SortedArray::add( o );      // add the entry
}

void TodoList::detach( Object& o, DeleteType d )
{
    dirty = TRUE;               // mark that the list has been modified
    SortedArray::detach( o, d );// remove the entry
}

int TodoList::indexOf( const TodoEntry& tde )
{
    for( int i = 0; i < getItemsInContainer(); i++ )
        if( (*this)[i] == tde )
            return i;
    return -1;
}

BOOL TodoList::modified()
{
    if( dirty == TRUE )         // if we've added or deleted entries
        return TRUE;            // we've been modified
                                // otherwise, if any entry has been
                                // modified, the list has been modified.

    ContainerIterator& ci = initIterator();
    while( ci != 0 )
        {
	TodoEntry& ent = (TodoEntry&)ci++;
        if( ent != NOOBJECT && ent.modified() == TRUE )
            {
            delete &ci;
            return TRUE;
            }
        }
    delete &ci;
    return FALSE;
}

void TodoList::clear()
{
    int count = getItemsInContainer();
    for( int i = 0; i < count; i++ )
        if( (*this)[i] != NOOBJECT )
	    destroy( i );
}

//---------------------------------------------------------------------
//
//  istream& operator >> ( istream& is, TodoList& tl );
//  ostream& operator << ( ostream& os, TodoList& tl );
//
//  inserter and extractor for TodoList.  These work together to
//  write lists out to a stream and read them back in.
//
//---------------------------------------------------------------------

istream& operator >> ( istream& is, TodoList& tl )
{
    do  {
        TodoEntry td;
        is >> td;
        if( !is.eof() )
            {
            tl.add( *(new TodoEntry( td )) );
            tl.dirty = FALSE;
            }
	} while( !is.eof() );
    return is;
}

ostream& operator << ( ostream& os, TodoList& tl )
{
    ContainerIterator& ci = tl.initIterator();
    while( ci != 0 )
        {
        TodoEntry& ent = (TodoEntry&)(ci++);
        if( ent != NOOBJECT )
            os << ent;
        }
    if( os )
        tl.dirty = FALSE;
    delete &ci;
    return os;
}

//---------------------------------------------------------------------
//
//  member functions for class ListBox.
//
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//
//  const ListBox& ListBox::operator = ( const TodoList& tdl );
//
//  copies the contents of a TodoList into a ListBox.
//
//---------------------------------------------------------------------

const ListBox& ListBox::operator = ( const TodoList& tdl )
{
    assert( hListBox != 0 );

    clear();

    ContainerIterator& ci = tdl.initIterator();
    while( ci != 0 )
	{
	Object& cur = ci++;
	if( cur != NOOBJECT )
	    {
	    char buf[100];      // write the entry into a string
				// and insert that string into
				// the list box

	    ostrstream( buf, 100 ) << cur << ends;
	    SendMessage( hListBox, LB_ADDSTRING, NULL, (LONG)(LPSTR)buf );
	    }
	}
    select( 0 );

    return *this;
}

void ListBox::insert( int i, const TodoEntry& tde )
{
    char temp[100];
    ostrstream( temp, sizeof( temp ) ) << (Object&)tde << ends;

    SendMessage( hListBox, LB_INSERTSTRING, i, (LONG)(LPSTR)temp );
    select( i );
}

void ListBox::create( HWND owner, HWND hInst, const RECT &wrect )
{
    hListBox = CreateWindow( "ListBox", NULL,
			     LBS_NOTIFY | WS_BORDER | WS_VSCROLL | LBS_USETABSTOPS | WS_CHILD | WS_VISIBLE,
			     wrect.left,
			     wrect.top,
			     wrect.right - wrect.left,
			     wrect.bottom - wrect.top,
			    (HWND)owner,
			    (HMENU)IDC_LISTBOX,
			    (HINSTANCE)hInst,
			     NULL
			   );

    int tabs[] = { 10, 100, 200 };
    SendMessage( hListBox,
                 LB_SETTABSTOPS,
                 sizeof(tabs)/sizeof(*tabs),
                 (LONG)(LPSTR)tabs
                );
    focus();
}

//---------------------------------------------------------------------
//
//  member functions for class TodoWindow.
//
//  these are mostly self-explanatory.
//
//---------------------------------------------------------------------

BOOL TodoWindow::registerClass()
{
    WNDCLASS wc;

    wc.style = 0;
    wc.lpfnWndProc = Window::wndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance =(HINSTANCE) hInst;
    wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground =(HBRUSH) GetStockObject( WHITE_BRUSH );
    wc.lpszMenuName = "TodoMenu";
    wc.lpszClassName = "TodoClass";

    return RegisterClass( &wc );
}

BOOL TodoWindow::createWindow()
{
    hWindow = CreateWindow(
		"TodoClass",
		"Todo List",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		(HINSTANCE)hInst,
		NULL
		);
    if( hWnd() == 0 )
        return FALSE;

    insert();                   // insert this window into the window list

    RECT wrect;
    GetClientRect( (HWND)hWnd(), (LPRECT) &wrect);
    listBox.create( (HWND)hWnd(), (HWND)hInst, wrect );
                                // build a list box in the client rectangle
    listBox = tdl;              // copy the Todo list into the list box

    ShowWindow( (HWND)hWnd(), show );
    UpdateWindow( (HWND)hWnd() );
    return TRUE;
}

void TodoWindow::aboutBox()
{
    AboutBox ab( (HWND)hWnd() );
    ab.run();
}

void TodoWindow::editBox()
{
    int cur = listBox.current();
    if( cur == -1 )
        {
        newEntry();             // if there's nothing in the list,
        return;                 // need to create an entry
        }

    EditBox ed( (HWND)hWnd(), (TodoEntry&)tdl[ cur ] );
    ed.run();

    listBox.replace( cur, (TodoEntry&)tdl[ cur ] );
}

void TodoWindow::newEntry()
{
    TodoEntry *tde = new TodoEntry();
    EditBox ed( (HWND)hWnd(), *tde );

    if( ed.run() != 0 )         // ed.run() returns 0 if terminated by
        delete tde;             // OK, 1 if terminated by Cancel.
    else
        {
        tdl.add( *tde );
        listBox.insert( tdl.indexOf( *tde ), *tde );
        }
}

void TodoWindow::delEntry()
{
    int cur = listBox.current();
    if( cur == -1 )             // if there's nothing in the list, there's
        return;                 // nothing to delete.
    tdl.destroy( cur );
    listBox.remove( cur );
    listBox.select( cur );
}

void TodoWindow::moveListBox()
{
    RECT wrect;
    GetClientRect( (HWND)hWnd(), (LPRECT) &wrect);

    listBox.move( wrect );
}

//---------------------------------------------------------------------
//
//  void TodoWindow::checkSave();
//
//  checks whether the Todo list has been modified.  If it has, asks
//  the user whether to save the list or not, and if it is to be saved,
//  writes it to a file.
//
//---------------------------------------------------------------------

void TodoWindow::checkSave()
{
    if( tdl.modified() == TRUE && saveBox() == TRUE )
        {
        if( fileName == 0 )
            if( getFileName( "Write to:", FALSE ) == FALSE )
                return;
        writeFile();
        }
}

void TodoWindow::newList()
{
    checkSave();                // dump the current list
    tdl.clear();
    listBox.clear();
    delete fileName;
    fileName = 0;               // mark that there's no file
}

void TodoWindow::openFile()
{
    checkSave();                // dump the current list
    tdl.clear();
    listBox.clear();
    if( getFileName( "Read from:", TRUE ) == TRUE )
        readFile();             // read new data from the specified file
}

void TodoWindow::saveFile()
{
    if( fileName == 0 )
        if( getFileName( "Write to:", FALSE ) == FALSE )
            return;
    writeFile();
}

void TodoWindow::saveFileAs()
{
    if( getFileName( "Write to:", FALSE ) == TRUE )
        writeFile();
}

BOOL TodoWindow::getFileName( const char *caption, BOOL me )
{
    char curdir[ MAXDIR ];
    getcwd( curdir, sizeof curdir );
    FileBox fb( (HWND)hWnd(), caption, curdir, "*.tdo", me );

    if( fb.run() != 0 )
        return FALSE;

    delete fileName;
    fileName = strdup( fb.getPath() );
    return TRUE;
}

void TodoWindow::readFile()
{
    ifstream in( fileName );    // open the input file
    assert( in );
    in >> tdl;                  // read the Todo list
    listBox = tdl;              // build the list box
}

void TodoWindow::writeFile()
{
    ofstream out( fileName );
    assert( out );
    out << tdl;
}

BOOL TodoWindow::saveBox()
{
    if( MessageBox( (HWND)hWnd(),
		    "Save Changes",
		    "Current List Modified",
		    MB_YESNO | MB_ICONQUESTION
		) == IDYES )
        return TRUE;
    else
        return FALSE;
}

//---------------------------------------------------------------------
//
//  BOOL TodoWindow::processCommand( WPARAM wParam, LPARAM lParam );
//
//  dispatches commands to the appropriate member functions.
//
//---------------------------------------------------------------------

BOOL TodoWindow::processCommand( WPARAM wParam, LPARAM lParam )
{
    switch( wParam )
        {

        case IDM_QUIT:
	    SendMessage((HWND)hWnd(), WM_CLOSE, 0, 0L);
            return TRUE;

        case IDM_NEW_LIST:
            newList();
            return TRUE;

        case IDM_OPEN:
            openFile();
            return TRUE;

        case IDM_SAVE:
            saveFile();
            return TRUE;

        case IDM_SAVEAS:
            saveFileAs();
            return TRUE;

        case IDM_EDIT:
            editBox();
            return TRUE;

        case IDM_NEW_ENTRY:
            newEntry();
            return TRUE;

        case IDM_DEL_ENTRY:
            delEntry();
            return TRUE;

        case IDM_ABOUT:
            aboutBox();
            return TRUE;

        case IDC_LISTBOX:
            if( HIWORD( lParam ) == LBN_DBLCLK )
                {
                editBox();
                return TRUE;
                }
            else
                return FALSE;
        default:
            return FALSE;
        }
}

//---------------------------------------------------------------------
//
//  LONG TodoWindow::dispatch( UINT msg, WPARAM wParam, LPARAM lParam );
//
//  dispatches messages to the appropriate member functions.
//
//---------------------------------------------------------------------

LONG TodoWindow::dispatch( UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
        {
        case WM_COMMAND:

            if( processCommand( wParam, lParam ) == TRUE )
                {
                listBox.focus();
                return 0;
                }
            break;

        case WM_MOVE:
        case WM_SIZE:

            moveListBox();
            return 0;

        case WM_QUERYENDSESSION:
            return TRUE;

        case WM_CLOSE:

            checkSave();
	    DestroyWindow( (HWND)hWnd() );
            return 0;

        case WM_DESTROY:
        case WM_QUIT:

            PostQuitMessage( 0 );
            break;
        }
        return Window::dispatch( msg, wParam, lParam );
}

//---------------------------------------------------------------------
//
//  int PASCAL WinMain( HINSTANCE, HINSTANCE, LPSTR, int );
//
//  the main entry point for the program.
//
//---------------------------------------------------------------------

int PASCAL WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
{
    TodoWindow td;
    td.create();
    return td.run();
}
