#if !defined( __TODOLIST_H )
#define __TODOLIST_H

//---------------------------------------------------------------------
//
//  TODOLIST.H
//
//      Copyright (c) 1991, 1992 by Borland International
//      All Rights Reserved.
//
//  defines the following classes, used in implementing the Todo list:
//
//      TdDate      - extends the Date class from the class
//                    library by providing a constructor that
//                    converts a text string into a date.
//
//      TodoEntry   - holds the data for an entry in the Todo list.
//
//      TodoList    - container for holding Todo entries
//
//      ListBox     - wrapper around the Windows listbox, providing
//                    an interface that fits with the Todo list.
//
//      TodoWindow  - the main window for this application.  There's
//                    nothing displayed in the window except for the
//                    list box.
//
//---------------------------------------------------------------------

#if !defined( __WINDOWS_H )
#define STRICT
#include <Windows.h>
#endif  // __WINDOWS_H

#if !defined( __IOSTREAM_H )
#include <iostream.h>
#endif  // __IOSTREAM_H

#if !defined( __STRING_H )
#include <string.h>
#endif	// __STRING_H

#if !defined( __SORTABLE_H )
#include "sortable.h"
#endif  // __SORTABLE_H

#if !defined( __SORTARRY_H )
#include "sortarry.h"
#endif  // __SORTARRY_H

#if !defined( __LDATE_H )
#include "ldate.h"
#endif  // __LDATE_H

#if !defined( __TODOWIN_H )
#include "TodoWin.h"
#endif  // __TODOWIN_H

#define TodoEntryClass  __firstUserClass

//---------------------------------------------------------------------
//
//  class TdDate
//
//      extends the Date class from the class library by providing a
//      constructor that converts a text string into a date.
//
//---------------------------------------------------------------------

class TdDate : public Date
{
public:

    TdDate();
    TdDate( const Date& );
    TdDate( const char * );
};

//---------------------------------------------------------------------
//
//  class TodoEntry
//
//      holds the data for an entry in the Todo list.
//
//---------------------------------------------------------------------

class TodoEntry : public Sortable
{
    friend class EditBox;

public:

    TodoEntry();                   // constructor.
    TodoEntry( const TodoEntry& ); // copy constructor.
    ~TodoEntry();		   // destructor.

    BOOL modified();            // indicates whether the entry has
                                // been modified.  Used in determining
                                // whether the list should be saved.

    virtual int             isEqual( const Object& ) const;
    virtual int             isLessThan( const Object& ) const;

    virtual classType       isA() const;
    virtual char            *nameOf() const;
    virtual hashValueType   hashValue() const;

    friend istream& operator >> ( istream&, TodoEntry& );
                                // reads a TodoEntry from a stream.  The
                                // format used must match the format used
                                // by operator <<.

    friend ostream& operator << ( ostream&, TodoEntry& );
                                // writes a TodoEntry to a stream.  The
                                // format used must match the format used
                                // by operator >>.

protected:

    virtual void            printOn( ostream& ) const;
                                // this is the function called by
                                // operator << when applied to an Object.
                                // The disambiguation rules of C++ determine
                                // whether this function or the operator <<
                                // defined for this class will be called.
				// For example:
                                //
                                // TodoEntry td;
                                // cout << td;  // calls the friend function
                                //              // function defined above
                                // cout << (Object&)td;
                                //              // calls the operator << for
                                //              // Object, which calls
                                //              // printOn().

private:

    BOOL dirty;                 // indicates whether this entry has
                                // been modified.

    TdDate dateCreated;
    TdDate dateDue;
    char *text;                 // the note associated with this entry
    int priority;

};

//---------------------------------------------------------------------
//
//  class TodoList
//
//      container for holding Todo entries.  Currently implemented as
//      a SortedArray, so we don't have to explicitly sort the entries
//      when a new one is added.  The sorting is done according to the
//      operator < () defined for a TodoEntry, which sorts according
//      to the due date.
//
//---------------------------------------------------------------------

class TodoList : public SortedArray
{

public:

    TodoList();
    ~TodoList();

    virtual void add( Object& );
				// adds an entry to the Todo list.

    virtual void detach( Object&, DeleteType = DefDelete );
				// removes an entry from the Todo list.

    int indexOf( const TodoEntry& );
                                // returns the index of the specified entry.

    BOOL modified();            // indicates whether the list has
                                // been modified by adding or deleting an
                                // entry.  Used in determining
                                // whether the list should be saved.

    void clear();               // removes all entries from the list.

    friend istream& operator >> ( istream&, TodoList& );
                                // reads a TodoList from a stream.  The
                                // format used must match the format used
                                // by operator <<.

    friend ostream& operator << ( ostream&, TodoList& );
                                // writes a TodoList to a stream.  The
                                // format used must match the format used
                                // by operator >>.

private:

    BOOL dirty;                 // indicates whether this list has been
                                // modified.

};

//---------------------------------------------------------------------
//
//  class ListBox
//
//      wrapper around the Windows listbox, providing an interface
//      that fits with the Todo list.  This is used to display the
//      Todo list in a window.
//
//---------------------------------------------------------------------

class ListBox
{

public:

    ListBox();

    const ListBox& operator = ( const TodoList& );
                                // copies the entries in the TodoList
                                // into the list box.

    void focus();               // sets focus to the list box.
    void move( const RECT& );   // moves and resizes the list box.
    int current();              // returns the index of the current
                                // selection.
    void remove( int );         // removes the specified entry from
				// the list box.
    void insert( int, const TodoEntry& );
				// adds an entry to the list box.
    void replace( int, const TodoEntry& );
                                // replaces an entry in the list box
                                // with another entry.
    void select( int );         // selects the specified entry.
    void clear();               // removes all entries.

    void create( HWND, HWND, const RECT& );
                                // builds the list box.  This can't be
                                // done in the constructor because we
                                // don't have enough information at the
                                // time of construction.

private:

    HWND hListBox;              // handle of the list box.

};

//---------------------------------------------------------------------
//
//  class TodoWindow
//
//      the main window for this application.  There's nothing displayed
//      in the window except for the list box.
//---------------------------------------------------------------------

class TodoWindow : public Window
{
public:

    TodoWindow();
    ~TodoWindow();

protected:

    virtual LONG dispatch( WPARAM, WPARAM, LPARAM );

    virtual BOOL registerClass();
    virtual BOOL createWindow();

private:

    ListBox listBox;            // the list box used by this window.
    TodoList tdl;               // the Todo list being displayed in this
                                // window.  There's a lot of parallelism
                                // between the operations of these two
                                // objects, and it might be worthwhile
                                // to add a class derived from both
                                // ListBox and TodoList for use here.

    char *fileName;             // path to the file currently being
                                // used.  0 if there is no file.

    void newList();
    void openFile();
    void saveFile();
    void saveFileAs();
    void editBox();
    void newEntry();
    void delEntry();
    void aboutBox();

    void fileBox();
    BOOL saveBox();

    BOOL getFileName( const char *, BOOL );

    void moveListBox();

    void readFile();
    void writeFile();
    void checkSave();

    BOOL processCommand( WPARAM, LPARAM );

};

//---------------------------------------------------------------------
//
//  inline functions.
//
//---------------------------------------------------------------------

inline int min( int a, int b ) { return (a<b) ? a : b; }

inline TdDate::TdDate() : Date()
{
}

inline TdDate::TdDate( const Date& d ) : Date( d )
{
}

inline TodoEntry::TodoEntry() : dirty( FALSE ), priority( 1 ), text( 0 )
{
}

inline TodoEntry::TodoEntry( const TodoEntry&td ) :
	dirty( td.dirty ),
	priority( td.priority ),
	text( strdup(td.text) ),
	dateCreated( td.dateCreated ),
	dateDue( td.dateDue )
{
}

inline TodoEntry::~TodoEntry()
{
	delete text;
}

inline BOOL TodoEntry::modified()
{
    return dirty;
}

inline TodoList::TodoList() : SortedArray( 20 )
{
}

inline TodoList::~TodoList()
{
}

inline ListBox::ListBox() : hListBox( 0 )
{
}

inline void ListBox::focus()
{
    if( IsWindow( hListBox ) )
        SetFocus( hListBox );
}

inline void ListBox::move( const RECT& wrect )
{
    MoveWindow( hListBox,
                wrect.left,
                wrect.top,
                wrect.right - wrect.left,
                wrect.bottom - wrect.top,
                TRUE
              );
}

inline int ListBox::current()
{
    return (int)SendMessage( hListBox, LB_GETCURSEL, 0, 0 );
}

inline void ListBox::remove( int i )
{
    SendMessage( hListBox, LB_DELETESTRING, i, 0 );
    select( i );
}

inline void ListBox::replace( int i, const TodoEntry& tde )
{
    remove( i );
    insert( i, tde );
}

inline void ListBox::select( int i )
{
    i = min( i, (int)SendMessage( hListBox, LB_GETCOUNT, 0, 0 ) - 1 );
    SendMessage( hListBox, LB_SETCURSEL, i, 0 );
}

inline void ListBox::clear()
{
    SendMessage( hListBox, LB_RESETCONTENT, 0, 0 );
}

inline TodoWindow::TodoWindow() : fileName( 0 )
{
}

inline TodoWindow::~TodoWindow()
{
}

#endif  // __TODOLIST_H

