#include <lib/llistForward.h>
#include <error.h>

template <typename T> llist<T>::llist()
{
    next = nullptr;
    prev = nullptr;
}

template <typename T> void  llist<T>::empty()
{
    for(;size();)
        removeFirst();
}


//		void template <typename T> llist<T>::dump()
//		{
//			if (next)
//                for(llist *i = this;i->next && (i = i->next);)
//                    std::cout << i->data << std::endl;
//		}

template <typename T> unsigned llist<T>::size() const
{
    if (!next)
        return 0;
    unsigned int j=0;
    for (const llist *i = this;i->next;i=i->next,j++);
    return j;
}

template <typename T> bool llist<T>::isEmpty() const
{
    return !next;
}

template <typename T> T llist<T>::at(unsigned int pos) const
{
    if (!next) // Empty list
        return 0;

    llist *i = next;
    for(unsigned int j = 0; j < pos; i = i->next, j++) // Find the index
        if (!i->next)   // If we reached the end before finding pos
        {
            fatalError("llist::at: Index out of range : %d", pos);
            return 0;
        }

    return i->data;
}

template <typename T> void llist<T>::prepend(T udata)
{
    llist *newlist = new llist;
    newlist->prev = this;
    newlist->next = next;
    if (next) // If prepending to an empty list, then next=0 and we would be writing at adress 0x4 !
        next->prev = newlist;
    next = newlist;
    next->data = udata;
}

template <typename T> void llist<T>::append(T udata)
{
    if (!next) // Empty list
    {
        next = new llist;
        next->prev = this;
        next->data = udata;
    }
    else
    {
        llist *i=next;
        for(;i->next;i=i->next); // Last elem
        i->next = new llist;
        i->next->prev = i;
        i->next->data = udata;
    }
}

// Insert at the given pos, shift the next elems to the right
template <typename T> void llist<T>::insert(unsigned int pos, T udata)
{
    if (pos == 0) // Prepend
    {
        prepend(udata);
        return;
    }

    if (!next) // Empty
        return;

    llist *i = next;
    unsigned int j;
    for(j = 0; j < pos; i = i->next, j++) // Find index
        if (!i->next)   // If we're at the end before reaching pos
        {
            if (j==pos-1) // We just want to append, so we can break here
                break;
            fatalError("llist::append: Index out of rande : %d", pos);
            return;
        }

    if (!i->next && j==pos-1) // Insert after the end (append)
    {
        i->next = new llist;
        i->next->prev = i;
        i->next->data = udata;
    }
    else
    {
        llist *newlist = new llist;
        i->prev->next = newlist;
        newlist->prev = i->prev;
        newlist->next = i;
        i->prev = newlist;
        newlist->data = udata;
    }
}

template <typename T> void llist<T>::removeLast()
{
    if (!next) // Empty list
        return;

    llist *i=next;
    for(;i->next;i=i->next); // Last elem
    i->prev->next = 0;
    delete i;
}

template <typename T> void llist<T>::removeFirst()
{
    if (next->next)
    {
        next = next->next;
        delete next->prev;
        next->prev = this;
    }
    else
    {
        delete next;
        next=0;
    }

}

// Remove the elem at this index
template <typename T> void llist<T>::removeAt(unsigned int index)
{
    if (!next) // Empty
        return;

    if (!index)
    {
        removeFirst();
        return;
    }

    llist *i = next;
    for(unsigned int j = 0; j < index; i = i->next, j++) // Find index
        if (!i->next)
        {
            fatalError("llist::removeAt: Index out of range : %d", index);
            return;
        }

    if (!i->next) // If we're at the end
    {
        i->prev->next = 0;
        delete i;
    }
    else
    {
        i->prev->next = i->next;
        i->next->prev = i->prev;
        delete i;
    }
}

template <typename T> llist<T>& llist<T>::operator<< (T udata)
{
    append(udata);
    return *this; // Allows to chain the <<s
}

/// Does NOT check the bounds.
template <typename T> T llist<T>::operator[] (unsigned int pos) const
{
    llist *i = next;
    for(unsigned int j = 0; j < pos; i = i->next, j++); // Find index
    return i->data;
}
