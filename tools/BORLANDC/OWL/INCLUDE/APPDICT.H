// ObjectWindows - (C) Copyright 1992 by Borland International

#if !defined(__APPDICT_H)
#define __APPDICT_H

#include <windows.h>
#include "applicat.h"

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

const int DEFAULT_DICTIONARY_SIZE = 10;
const int DEFAULT_DICTIONARY_INCREMENT = 10;

class _CLASSTYPE TAppDictionaryEntry
{
public:
    HANDLE hTask;
    PTApplication pApplication;
};

class _CLASSTYPE TAppDictionary
{
    int NumEntries;
    TAppDictionaryEntry * Table;
public:
    TAppDictionary(int InitialCount = DEFAULT_DICTIONARY_SIZE);
    ~TAppDictionary();

    static TAppDictionaryEntry * AllocTable(int Count);
    static void FreeTable(TAppDictionaryEntry *pTable);

    TAppDictionaryEntry *GrowTable(int Increment =
                                   DEFAULT_DICTIONARY_INCREMENT);
    void Add(PTApplication pApplication);
    void Delete();
    PTApplication Lookup();
};

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif // __APPDICT_H
