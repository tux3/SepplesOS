#ifndef LLIST_H
#define LLIST_H

// "Forward declaration" of llist. 
// The actual template definitions are in llist.h
// llist.h MUST be included in every translation unit

template <typename T> class llist
{
	public:
		explicit llist();
		void empty();
//		void dump();
        unsigned size() const;
        bool isEmpty() const;
        T at(unsigned int pos) const;
        void prepend(T udata);
		void append(T udata);
		void insert(unsigned int pos, T udata);
		void removeLast();
		void removeFirst();
		void removeAt(unsigned int index);
        llist& operator<< (T udata);
        T operator[] (unsigned int pos) const; /// Does NOT check the bounds.

	private:
		llist *next, *prev;
		T data;
} __attribute__((packed));

#endif // LLIST_H
