#ifndef SIZEDARRAY_H
#define SIZEDARRAY_H

#include <std/types.h>
#include <mm/malloc.h>
#include <mm/memcpy.h>
#include <debug.h>

template <typename T> struct SizedArray
{
    SizedArray<T>()
    {
        size=0;
        elems=nullptr;
    }

    SizedArray<T>(const SizedArray<T>& other)
    {
        size = other.size;
        if (size)
        {
            elems=new T[size];
            memcpy((char*)elems, (char*)other.elems, size*sizeof(T));
        }
        else
            elems = nullptr;
    }

    SizedArray<T>& operator=(const SizedArray<T>& other)
    {
        size = other.size;
        delete elems;
        if (size)
        {
            elems=new T[size];
            memcpy((char*)elems, (char*)other.elems, size*sizeof(T));
        }
        else
            elems = nullptr;
        return *this;
    }

    T& operator[](int index)
    {
        return elems[index];
    }

    ~SizedArray<T>()
    {
        delete[] elems;
    }

    void empty()
    {
        delete[] elems;
        size = 0;
        elems = nullptr;
    }

    void append(T elem)
    {
        size++;
        elems=(T*)Malloc::realloc((u8*)elems, size*sizeof(T));
        elems[size-1]=elem;
    }

    void removeAt(u64 pos)
    {
        size--;
        T* newElems = new T[size];
        memcpy(newElems, elems, pos*sizeof(T));
        memcpy(newElems+pos,elems+pos+1, (size-pos)*sizeof(T));
        delete elems;
        elems = newElems;
    }

    u64 size; ///< Number of elements
    T* elems; ///< Actual elements
};

#endif // SIZEDARRAY_H
