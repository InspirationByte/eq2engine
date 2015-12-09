//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Linked list, and constant iterator without a removal
//////////////////////////////////////////////////////////////////////////////////

#ifndef DKLINKEDLIST_H
#define DKLINKEDLIST_H

template <class TYPE>
struct DkLLNode
{
	DkLLNode <TYPE> *prev;
	DkLLNode <TYPE> *next;
	TYPE object;
};

template <class T>
class DkLinkedList
{
public:
	template <class> friend class DkLinkedListIterator;

	DkLinkedList()
	{
		count = 0;
		first = NULL;
		last  = NULL;
		curr  = NULL;
		del   = NULL;
	}

	~DkLinkedList()
	{
		clear();
	}

	int getCount() const { return count; }

	void addFirst(const T& object)
	{
		DkLLNode <T> *node = new DkLLNode <T>;
		node->object = object;
		insertNodeFirst(node);
		count++;
	}

	void addLast(const T& object)
	{
		DkLLNode <T> *node = new DkLLNode <T>;
		node->object = object;
		insertNodeLast(node);
		count++;
	}

	void insertBeforeCurrent(const T& object)
	{
		DkLLNode <T> *node = new DkLLNode <T>;
		node->object = object;
		insertNodeBefore(curr, node);
		count++;
	}

	void insertAfterCurrent(const T& object)
	{
		DkLLNode <T> *node = new DkLLNode <T>;
		node->object = object;
		insertNodeAfter(curr, node);
		count++;
	}

	void insertSorted(const T& object, int (* comparator )(const T &a, const T &b) )
	{
		DkLLNode <T>* newnode = new DkLLNode <T>;
		newnode->object = object;

		// Special case for the head end
		//if(last && (comparator)(last->object, object) >= 0 )
		//{
		//	insertNodeLast(newnode);
		//}
		//else
		{
			DkLLNode <T>* curr = first;
			while (curr != NULL && (comparator)(curr->object, object) <= 0)
			{
				curr = curr->next;
			}

			if(curr)
				insertNodeBefore(curr, newnode);
			else
				insertNodeLast(newnode);
		}

		count++;
	}

	bool removeCurrent()
	{
		if (curr != NULL)
		{
			releaseNode(curr);

			if (del)
				delete del;
			del = curr;
			count--;
		}
		return (curr != NULL);
	}

	bool goToFirst() { return (curr = first) != NULL; }
	bool goToLast () { return (curr = last ) != NULL; }
	bool goToPrev () { return (curr = curr->prev) != NULL; }
	bool goToNext () { return (curr = curr->next) != NULL; }

	bool goToObject(const T& object)
	{
		curr = first;
		while (curr != NULL)
		{
			if (object == curr->object) return true;
			curr = curr->next;
		}
		return false;
	}

	const T & getCurrent() const 
	{
		return curr->object;
	}

	DkLLNode<T>* getCurrentNode() const
	{
		return curr;
	}

	void setCurrent(const T& object)
	{
		curr->object = object;
	}

	const T & getPrev() const
	{
		return curr->prev->object;
	}

	const T & getNext() const
	{
		return curr->next->object;
	}

	const T & getPrevWrap() const
	{
		return ((curr->prev != NULL)? curr->prev : last)->object;
	}

	const T & getNextWrap() const
	{
		return ((curr->next != NULL)? curr->next : first)->object;
	}

	void clear()
	{
		delete del;
		del = NULL;
		while (first)
		{
			curr = first;
			first = first->next;
			delete curr;
		}
		last = curr = NULL;
		count = 0;
	}

	void moveCurrentToTop()
	{
		if (curr != NULL)
		{
			releaseNode(curr);
			insertNodeFirst(curr);
		}
	}

protected:
	void insertNodeFirst(DkLLNode <T> *node)
	{
		if (first != NULL)
		{
			first->prev = node;
		}
		else
		{
			last = node;
		}
		node->next = first;
		node->prev = NULL;

		first = node;
	}

	void insertNodeLast(DkLLNode <T> *node)
	{
		if (last != NULL)
		{
			last->next = node;
		}
		else
		{
			first = node;
		}
		node->prev = last;
		node->next = NULL;

		last = node;
	}

	void insertNodeBefore(DkLLNode <T> *at, DkLLNode <T> *node)
	{
		DkLLNode <T> *prev = at->prev;
		at->prev = node;
		if (prev){
			prev->next = node;
		} else {
			first = node;
		}
		node->next = at;
		node->prev = prev;
	}

	void insertNodeAfter(DkLLNode <T> *at, DkLLNode <T> *node)
	{
		DkLLNode <T> *next = at->next;
		at->next = node;
		if (next){
			next->prev = node;
		} else {
			last = node;
		}
		node->prev = at;
		node->next = next;
	}

	void releaseNode(const DkLLNode <T> *node)
	{
		if (node->prev == NULL)
		{
			first = node->next;
		}
		else
		{
			node->prev->next = node->next;
		}
		if (node->next == NULL)
		{
			last = node->prev;
		}
		else
		{
			node->next->prev = node->prev;
		}
	}

	DkLLNode <T> *first, *last, *curr, *del;
	int count;
};

template <class T>
class DkLinkedListIterator
{
public:
	template <class> friend class DkLinkedList;

	DkLinkedListIterator();

	DkLinkedListIterator(DkLinkedList<T> &list)
	{
		first = list.first;
		last = list.last;
		curr = list.curr;
	}

	bool goToFirst() { return (curr = first) != NULL; }
	bool goToLast () { return (curr = last ) != NULL; }
	bool goToPrev () { return (curr = curr->prev) != NULL; }
	bool goToNext () { return (curr = curr->next) != NULL; }

	bool goToObject(const T& object)
	{
		curr = first;
		while (curr != NULL)
		{
			if (object == curr->object) return true;
			curr = curr->next;
		}
		return false;
	}

	const T & getCurrent() const { return curr->object; }

	const T & getPrev() const { return curr->prev->object; }
	const T & getNext() const { return curr->next->object; }
	const T & getPrevWrap() const { return ((curr->prev != NULL)? curr->prev : last)->object; }
	const T & getNextWrap() const { return ((curr->next != NULL)? curr->next : first)->object; }

private:

	DkLLNode <T> *first, *last, *curr;
	int count;
};

#endif // _DkLinkedList_H_
