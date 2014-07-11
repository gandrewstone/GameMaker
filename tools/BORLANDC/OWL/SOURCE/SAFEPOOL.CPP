// Borland C++ - (C) Copyright 1992 by Borland International

#include <stddef.h>
#include <stdlib.h>
#include <safepool.h>

typedef void (_FAR *pvf)();

pvf _new_handler;

int	SafetyPool::Size = DEFAULT_SAFETY_POOL_SIZE;
void _FAR *  SafetyPool::safetyPool = new char[DEFAULT_SAFETY_POOL_SIZE];

pvf set_new_handler(pvf p)
{
    pvf t = _new_handler;
    _new_handler = p;
    return t;
}

static void _FAR *originalNew(size_t size)
{
    void _FAR * p;

    size = size ? size : 1;
    while ( (p = malloc(size)) == NULL && _new_handler != NULL)
        _new_handler();
    return p;
}

void _FAR *operator new( size_t size )
{
    void _FAR *allocated = originalNew (size);
    if (allocated == 0)
        if (SafetyPool::IsExhausted())
            return NULL;
        else
        {
            delete SafetyPool::safetyPool;
            SafetyPool::safetyPool = 0;
            allocated = originalNew (size);
            if (allocated == 0)
                return NULL;
        }
        return allocated;
}

void operator delete( void _FAR *ptr )
{
    if (ptr)
    {
        free(ptr);

        // try to restore the safety pool if it is exhausted
        if (SafetyPool::IsExhausted())
            SafetyPool::Allocate();
    }
}

/* This static member function deletes the existing safetypool and
   reallocates one with the specified size.  It returns TRUE if this
   is successful, otherwise FALSE.  Note that if this function returns
   FALSE, the next call to check on the safety pool will indicate the
   low memory condition. */

int SafetyPool::Allocate(size_t size)
{
    delete safetyPool;		 // delete existing safetyPool
	
    safetyPool = new char[size]; // reallocate safetyPool
    Size       = size;

    return !IsExhausted(); // return success/failure
}
