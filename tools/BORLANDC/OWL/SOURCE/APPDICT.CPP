// Borland C++ - (C) Copyright 1992 by Borland International

/* ---------------------------------------------------------
   APPDICT.CPP
   Implementation for the TAppDictionary class, a dictionary of
   associations between task handles and TApplication pointers.
   Used to support GetApplicationObject().
   --------------------------------------------------------- */

#include "appdict.h"

#if defined(__DLL__)
TAppDictionary *AppDictionary = new TAppDictionary();
#else
PTApplication pTApplication;
#endif

PTApplication _EXPFUNC GetApplicationObject()
{
#if defined (__DLL__)
    return AppDictionary->Lookup();
#else
    return pTApplication;
#endif
}

#if defined (__DLL__)
TAppDictionary::TAppDictionary(int InitialCount)
{
    if (InitialCount != 0)
        Table = AllocTable(InitialCount);

    if (Table)
        NumEntries = InitialCount;
    else
        NumEntries = 0;
}

TAppDictionary::~TAppDictionary()
{
    if (NumEntries != 0 && Table)
    {
        FreeTable(Table);
        Table = NULL;
        NumEntries = 0;
    }
}

TAppDictionaryEntry * TAppDictionary::AllocTable(int Count)
{
    TAppDictionaryEntry * pTable;

    pTable = (TAppDictionaryEntry *)
        LocalLock(LocalAlloc(LMEM_ZEROINIT,
            Count * sizeof(TAppDictionaryEntry)));
    return pTable;
}

void TAppDictionary::FreeTable(TAppDictionaryEntry *pTable)
{
    HANDLE hMem =
#ifdef STRICT	
	    LocalHandle((void NEAR*)FP_OFF(pTable));
#else
	    LocalHandle(FP_OFF(pTable));
#endif
    if (LocalUnlock(hMem))
        LocalFree(hMem);
}

TAppDictionaryEntry *TAppDictionary::GrowTable(int Increment)
{
    int OldNumEntries = NumEntries;
    TAppDictionaryEntry * OldInstanceDataTable = Table;

    NumEntries = NumEntries + Increment;

    Table = AllocTable(NumEntries);

    // copy old table to new location
    _fmemcpy(Table, OldInstanceDataTable,
        OldNumEntries * sizeof(TAppDictionaryEntry));

    FreeTable(OldInstanceDataTable);

    // return pointer to first entry in new block

    return &Table[OldNumEntries];
}

void TAppDictionary::Add(PTApplication pApplication)
{
    HANDLE hTask = GetCurrentTask();
    TAppDictionaryEntry *pFreeEntry = NULL;  // no free entry yet

    // First see if table already.  If so, replace entry

    for (TAppDictionaryEntry * pEntry = Table; pEntry < &Table[NumEntries];
         pEntry++)
    {
        if (pEntry->hTask == hTask) // already in table?
        {
            pEntry->pApplication = pApplication;
            return;
        }
        else                    // see if entry is free
        {
            if (!pFreeEntry && pEntry->hTask == 0)  // remember this entry
                pFreeEntry = pEntry;
        }
    }

    // Not in table.  See if we encountered a free entry in table.  If
    // so, use it.  Otherwise grow table.

    if ((pEntry = (pFreeEntry ? pFreeEntry : GrowTable())) != 0)
    {
        pEntry->hTask = hTask;
        pEntry->pApplication = pApplication;
        return;
    }
}

PTApplication TAppDictionary::Lookup()
{
    HANDLE hTask = GetCurrentTask();

    for (TAppDictionaryEntry * pEntry = Table; pEntry < &Table[NumEntries];
         pEntry++)
    {
        if (pEntry->hTask == hTask)
            return pEntry->pApplication;
    }
    // Didn't find an entry in the table
    return NULL;
}

void TAppDictionary::Delete()
{
    HANDLE hTask = GetCurrentTask();

    for (TAppDictionaryEntry * pEntry = Table; pEntry < &Table[NumEntries];
         pEntry++)
    {
        if (pEntry->hTask == hTask)
        {
            pEntry->hTask = 0;       // found entry.  Zero it.
            pEntry->pApplication = 0;
            return;
        }
    }
}

#endif // __DLL__
