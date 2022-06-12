//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Double Linked list
//////////////////////////////////////////////////////////////////////////////////

#ifndef LIST_H
#define LIST_H

template <class TYPE>
struct ListNode
{
	ListNode <TYPE> *prev;
	ListNode <TYPE> *next;
	TYPE object;
};

template <class T>
class List
{
public:
	template <class> friend class ListIterator;

	List()
	{
		count = 0;
		first = nullptr;
		last  = nullptr;
		curr  = nullptr;
		del   = nullptr;
	}

	virtual ~List()
	{
		clear();
	}

	int getCount() const { return count; }

	bool addFirst(const T& object)
	{
		ListNode <T> *node = new ListNode <T>;
		node->object = object;
		insertNodeFirst(node);
		count++;
		return true;
	}

	bool addLast(const T& object)
	{
		ListNode <T> *node = new ListNode <T>;
		node->object = object;
		insertNodeLast(node);
		count++;
		return true;
	}

	bool insertBeforeCurrent(const T& object)
	{
		ListNode <T> *node = new ListNode <T>;
		node->object = object;
		insertNodeBefore(curr, node);
		count++;
		return true;
	}

	bool insertAfterCurrent(const T& object)
	{
		ListNode <T> *node = new ListNode <T>;
		node->object = object;
		insertNodeAfter(curr, node);
		count++;
		return true;
	}

	bool insertSorted(const T& object, int (* comparator )(const T &a, const T &b) )
	{
		ListNode <T>* newnode = new ListNode <T>;
		newnode->object = object;

		ListNode <T>* c = first;
		while (c != nullptr && (comparator)(c->object, object) <= 0)
		{
			c = c->next;
		}

		if(c)
			insertNodeBefore(c, newnode);
		else
			insertNodeLast(newnode);

		count++;
		return true;
	}

	bool removeCurrent()
	{
		if (curr != nullptr)
		{
			releaseNode(curr);

			if (del)
				delete del;
			del = curr;
			count--;
		}
		return (curr != nullptr);
	}

	ListNode<T>* goToFirst() { return curr = first; }
	ListNode<T>* goToLast () { return curr = last; }
	ListNode<T>* goToPrev () { return curr = curr->prev; }
	ListNode<T>* goToNext () { return curr = curr->next; }

	bool goToObject(const T& object)
	{
		curr = first;
		while (curr != nullptr)
		{
			if (object == curr->object) return true;
			curr = curr->next;
		}
		return false;
	}

	T & getCurrent() const
	{
		return curr->object;
	}

	ListNode<T>* getCurrentNode() const
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
		return ((curr->prev != nullptr)? curr->prev : last)->object;
	}

	const T & getNextWrap() const
	{
		return ((curr->next != nullptr)? curr->next : first)->object;
	}

	void clear()
	{
		delete del;
		del = nullptr;
		while (first)
		{
			curr = first;
			first = first->next;
			delete curr;
		}
		last = curr = nullptr;
		count = 0;
	}

	void moveCurrentToTop()
	{
		if (curr != nullptr)
		{
			releaseNode(curr);
			insertNodeFirst(curr);
		}
	}

protected:
	void insertNodeFirst(ListNode <T> *node)
	{
		if (first != nullptr)
			first->prev = node;
		else
			last = node;

		node->next = first;
		node->prev = nullptr;

		first = node;
	}

	void insertNodeLast(ListNode <T> *node)
	{
		if (last != nullptr)
			last->next = node;
		else
			first = node;

		node->prev = last;
		node->next = nullptr;

		last = node;
	}

	void insertNodeBefore(ListNode <T> *at, ListNode <T> *node)
	{
		ListNode <T> *prev = at->prev;
		at->prev = node;
		if (prev)
			prev->next = node;
		else
			first = node;

		node->next = at;
		node->prev = prev;
	}

	void insertNodeAfter(ListNode <T> *at, ListNode <T> *node)
	{
		ListNode <T> *next = at->next;
		at->next = node;
		if (next)
			next->prev = node;
		else
			last = node;

		node->prev = at;
		node->next = next;
	}

	void releaseNode(ListNode <T> *node)
	{
		if (node->prev == nullptr)
			first = node->next;
		else
			node->prev->next = node->next;

		if (node->next == nullptr)
			last = node->prev;
		else
			node->next->prev = node->prev;

		node->next = nullptr;
		node->prev = nullptr;
	}

	ListNode <T> *first, *last, *curr, *del;
	int count;
};

//--------------------------------------------------------------------------------------

template <class T, int MAXSIZE>
class FixedList
{
public:
	FixedList()
	{
		count = 0;
		first = nullptr;
		last = nullptr;
		curr = nullptr;
		del = nullptr;
	}

	virtual ~FixedList()
	{
	}

	int getCount() const { return count; }

	bool addFirst(const T& object)
	{
		ListNode <T> *node = getFreeNode();
		if(!node)
			return false;
		node->object = object;
		insertNodeFirst(node);
		count++;
		return true;
	}

	bool addLast(const T& object)
	{
		ListNode <T> *node = getFreeNode();
		if(!node)
			return false;
		node->object = object;
		insertNodeLast(node);
		count++;
		return true;
	}

	bool insertBeforeCurrent(const T& object)
	{
		ListNode <T> *node = getFreeNode();
		if(!node)
			return false;
		node->object = object;
		insertNodeBefore(curr, node);
		count++;
		return true;
	}

	bool insertAfterCurrent(const T& object)
	{
		ListNode <T> *node = getFreeNode();
		if(!node)
			return false;

		node->object = object;
		insertNodeAfter(curr, node);
		count++;
		return true;
	}

	bool insertSorted(const T& object, int (* comparator )(const T &a, const T &b) )
	{
		ListNode <T>* newnode = getFreeNode();
		if(!newnode)
			return false;
		newnode->object = object;

		ListNode <T>* c = first;
		while (c != nullptr && (comparator)(c->object, object) <= 0)
			c = c->next;

		if(c)
			insertNodeBefore(c, newnode);
		else
			insertNodeLast(newnode);

		count++;
		return true;
	}

	bool removeCurrent()
	{
		if (curr != nullptr)
		{
			releaseNode(curr);
			count--;
		}
		return (curr != nullptr);
	}

	ListNode<T>* goToFirst() { return curr = first; }
	ListNode<T>* goToLast() { return curr = last; }
	ListNode<T>* goToPrev() { return curr = curr->prev; }
	ListNode<T>* goToNext() { return curr = curr->next; }

	bool goToObject(const T& object)
	{
		curr = first;
		while (curr != nullptr)
		{
			if (object == curr->object) return true;
			curr = curr->next;
		}
		return false;
	}

	T& getCurrent() const
	{
		return curr->object;
	}

	ListNode<T>* getCurrentNode() const
	{
		return curr;
	}

	void setCurrent(const T& object)
	{
		curr->object = object;
	}

	const T& getPrev() const
	{
		return curr->prev->object;
	}

	const T& getNext() const
	{
		return curr->next->object;
	}

	const T& getPrevWrap() const
	{
		return ((curr->prev != nullptr) ? curr->prev : last)->object;
	}

	const T& getNextWrap() const
	{
		return ((curr->next != nullptr) ? curr->next : first)->object;
	}

	void clear()
	{
		del = nullptr;
		while (first)
		{
			curr = first;
			first = first->next;
			curr->next = nullptr;
			curr->prev = nullptr;
		}
		last = curr = nullptr;
		count = 0;
	}

protected:
	void insertNodeFirst(ListNode <T>* node)
	{
		if (first != nullptr)
			first->prev = node;
		else
			last = node;

		node->next = first;
		node->prev = nullptr;

		first = node;
	}

	void insertNodeLast(ListNode <T>* node)
	{
		if (last != nullptr)
			last->next = node;
		else
			first = node;

		node->prev = last;
		node->next = nullptr;

		last = node;
	}

	void insertNodeBefore(ListNode <T>* at, ListNode <T>* node)
	{
		ListNode <T>* prev = at->prev;
		at->prev = node;
		if (prev)
			prev->next = node;
		else
			first = node;

		node->next = at;
		node->prev = prev;
	}

	void insertNodeAfter(ListNode <T>* at, ListNode <T>* node)
	{
		ListNode <T>* next = at->next;
		at->next = node;
		if (next)
			next->prev = node;
		else
			last = node;

		node->prev = at;
		node->next = next;
	}

	void releaseNode(ListNode <T>* node)
	{
		if (node->prev == nullptr)
			first = node->next;
		else
			node->prev->next = node->next;

		if (node->next == nullptr)
			last = node->prev;
		else
			node->next->prev = node->prev;

		node->next = nullptr;
		node->prev = nullptr;
	}

	ListNode<T>* getFreeNode()
	{
		for(int i = 0; i < MAXSIZE; i++)
		{
			if(m_fixedarr[i].prev || m_fixedarr[i].next || first == &m_fixedarr[i])
				continue;

			return &m_fixedarr[i];
		}

		return nullptr;
	}

	ListNode<T> m_fixedarr[MAXSIZE];
	ListNode<T>* first, * last, * curr, * del;
	int count;
};

//--------------------------------------------------------------------------------------

template <class T>
class ListIterator
{
public:
	template <class> friend class List;

	ListIterator(List<T> &list)
	{
		first = list.first;
		last = list.last;
		curr = list.curr;
	}

	bool goToFirst() { return (curr = first) != nullptr; }
	bool goToLast () { return (curr = last ) != nullptr; }
	bool goToPrev () { return (curr = curr->prev) != nullptr; }
	bool goToNext () { return (curr = curr->next) != nullptr; }

	bool goToObject(const T& object)
	{
		curr = first;
		while (curr != nullptr)
		{
			if (object == curr->object) return true;
			curr = curr->next;
		}
		return false;
	}

	const T & getCurrent() const { return curr->object; }

	const T & getPrev() const { return curr->prev->object; }
	const T & getNext() const { return curr->next->object; }
	const T & getPrevWrap() const { return ((curr->prev != nullptr)? curr->prev : last)->object; }
	const T & getNextWrap() const { return ((curr->next != nullptr)? curr->next : first)->object; }

private:

	ListNode <T> *first, *last, *curr;
	int count;
};

#endif // LIST_H
