//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Double Linked list
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename T>
struct ListNode
{
public:
	const T&	operator*() const { return value; }
	T&			operator*() { return value; }
	const T*	operator->() const { return &value; }
	T*			operator->() { return &value; }

	T			value;
	ListNode*	prev{ nullptr };
	ListNode*	next{ nullptr };
};

//------------------------------------------------------

template <typename T, typename STORAGE_TYPE>
struct ListAllocatorBase
{
	using Node = ListNode<T>;

	virtual			~ListAllocatorBase() {}

	virtual Node*	alloc() = 0;
	virtual void	free(Node* node) = 0;
};

template<typename T>
class DynamicListAllocator 
	: public ListAllocatorBase<T, DynamicListAllocator<T>>
{
public:
	using Node = ListNode<T>;

	DynamicListAllocator(const PPSourceLine& sl)
		: m_pool(sl)
	{
	}

	DynamicListAllocator(DynamicListAllocator&& other)
		: m_pool(std::move(other.m_pool))
	{
	}

	Node* alloc()
	{
		Node* node = m_pool.allocate();
		new(node) Node();
		return node;
	}

	void free(Node* node)
	{
		node->~Node();
		m_pool.deallocate(node);
	}

private:
	MemoryPool<Node, 32> m_pool;
};


template<typename T, int SIZE>
class FixedListAllocator 
	: public ListAllocatorBase<T, FixedListAllocator<T, SIZE>>
{
public:
	using Node = ListNode<T>;

	FixedListAllocator() = default;

	Node* alloc()
	{
		Node* newNode = getNextNodeFromPool();

		ASSERT_MSG(newNode, "FixedListAllocator - No more free nodes in pool (%d)!", SIZE);

		new(newNode) Node();
		return newNode;
	}

	void free(Node* node)
	{
		const int idx = node - m_nodePool;
		ASSERT_MSG(idx >= 0 && idx < SIZE, "FixedListAllocator tried to remove Node that doesn't belong it's list!");

		node->~Node();

		*(int*)node = m_firstFreeNode;
		m_firstFreeNode = idx;
	}

	Node* getNextNodeFromPool()
	{
		if (m_firstFreeNode != -1)
		{
			Node* node = &m_nodePool[m_firstFreeNode];
			ASSERT_MSG(m_firstFreeNode >= -1 && m_firstFreeNode < SIZE, "getNextNodeFromPool m_firstFreeNode==%d, m_usedNodes==%d", m_firstFreeNode, m_usedNodes);
			m_firstFreeNode = *(int*)node;
			return node;
		}

		if(m_usedNodes + 1 <= SIZE)
			return &m_nodePool[m_usedNodes++];
		return nullptr;
	}

	int		m_firstFreeNode{ -1 };
	int		m_usedNodes{ 0 };
	Node	m_nodePool[SIZE];
};

// TNODE is abstract type that has prev & next fields
template<typename TNODE>
class LinkedListImpl
{
public:
	TNODE*	getFirst() const { return m_first; }
	TNODE*	getLast() const { return m_last; }

	int getCount() const { return m_count; }

	void insertNodeFirst(TNODE* node)
	{
		if (m_first != nullptr)
			m_first->prev = node;
		else
			m_last = node;

		node->next = m_first;
		node->prev = nullptr;

		m_first = node;
		m_count++;
	}

	void insertNodeLast(TNODE* node)
	{
		if (m_last != nullptr)
			m_last->next = node;
		else
			m_first = node;

		node->prev = m_last;
		node->next = nullptr;

		m_last = node;
		m_count++;
	}

	void insertNodeBefore(TNODE* at, TNODE* node)
	{
		TNODE* prev = at->prev;
		at->prev = node;
		if (prev)
			prev->next = node;
		else
			m_first = node;

		node->next = at;
		node->prev = prev;
		m_count++;
	}

	void insertNodeAfter(TNODE* at, TNODE* node)
	{
		TNODE* next = at->next;
		at->next = node;
		if (next)
			next->prev = node;
		else
			m_last = node;

		node->prev = at;
		node->next = next;
		m_count++;
	}

	void unlinkNode(TNODE* node)
	{
		if (m_first != m_last)
		{
			ASSERT_MSG(node->next != nullptr || node->prev != nullptr, "unlinkNode - already released node");
		}

		if (node->prev == nullptr)
			m_first = node->next;
		else
			node->prev->next = node->next;

		if (node->next == nullptr)
			m_last = node->prev;
		else
			node->next->prev = node->prev;

		node->next = nullptr;
		node->prev = nullptr;
		m_count--;
	}

protected:
	TNODE*	m_first{ nullptr };
	TNODE*	m_last{ nullptr };
	int		m_count{ 0 };
};

template <typename T, typename STORAGE_TYPE>
class ListBase
	: LinkedListImpl<ListNode<T>>
	, STORAGE_TYPE
{
public:
	using Node = ListNode<T>;
	using Impl = LinkedListImpl<Node>;

	struct Iterator
	{
		Node*	current{ nullptr };
		Iterator() = default;
		Iterator(Node* node)
			: current(node)
		{}

		bool		operator==(Iterator& it) const { return it.current == current; }
		bool		operator!=(Iterator& it) const { return it.current != current; }

		bool		atEnd() const		{ return current == nullptr; }
		T&			operator*()			{ return current->value; }
		const T&	operator*()	const	{ return current->value; }
		void		operator++()		{ current = current->next; }
		void		operator--()		{ current = current->prev; }
	};

	Iterator		first() const { return Iterator(Impl::m_first); }
	Iterator		last() const { return Iterator(Impl::m_last); }

	Iterator		begin() const { return Iterator(Impl::m_first); }
	Iterator		end() const { return Iterator(); }

	const T&		front() const { return Impl::m_first->value; }
	T&				front() { return Impl::m_first->value; }

	const T&		back() const { return Impl::m_last->value; }
	T&				back() { return Impl::m_last->value; }

	virtual ~ListBase()
	{
		clear();
	}

	ListBase()
	{
		static_assert(!std::is_same_v<STORAGE_TYPE, DynamicListAllocator<T>>, "PP_SL constructor is required to use");
	}

	ListBase(const PPSourceLine& sl) 
		: STORAGE_TYPE(sl)
	{
	}

	ListBase(const ListBase& other)
		: STORAGE_TYPE(other.sl)
	{
		Impl::m_first = other.Impl::m_first;
		Impl::m_last = other.Impl::m_last;
		Impl::m_count = other.Impl::m_count;
		other.Impl::m_first = nullptr;
		other.Impl::m_last = nullptr;
		other.Impl::m_count = 0;
	}

	ListBase(ListBase&& other) noexcept
		: STORAGE_TYPE(std::move(other))
	{
		Impl::m_first = other.Impl::m_first;
		Impl::m_last = other.Impl::m_last;
		Impl::m_count = other.Impl::m_count;
		other.Impl::m_first = nullptr;
		other.Impl::m_last = nullptr;
		other.Impl::m_count = 0;
	}

	int getCount() const { return Impl::m_count; }

	Iterator prepend(const T& value)
	{
		Node* node = allocNode();
		node->value = value;
		Impl::insertNodeFirst(node);
		return Iterator(node);
	}

	Iterator prepend(T&& value)
	{
		Node* node = allocNode();
		node->value = std::move(value);
		Impl::insertNodeFirst(node);
		return Iterator(node);
	}

	T& prepend()
	{
		Node* node = allocNode();
		Impl::insertNodeFirst(node);
		return node->value;
	}

	Iterator append(const T& value)
	{
		Node* node = allocNode();
		node->value = value;
		Impl::insertNodeLast(node);
		return Iterator(node);
	}

	Iterator append(T&& value)
	{
		Node* node = allocNode();
		node->value = std::move(value);
		Impl::insertNodeLast(node);
		return Iterator(node);
	}

	T& append()
	{
		Node* node = allocNode();
		Impl::insertNodeLast(node);
		return node->value;
	}

	template<typename COMPARATOR>
	Iterator insertSorted(const T& value, COMPARATOR compareFunc)
	{
		Node* node = allocNode();
		node->value = value;

		Node* c = Impl::m_first;
		while (c != nullptr && compareFunc(c->value, value) <= 0)
		{
			c = c->m_next;
		}

		if (c)
			Impl::insertNodeBefore(c, node);
		else
			Impl::insertNodeLast(node);

		return Iterator(node);
	}

	T popFront()
	{
		Node* firstNode = Impl::m_first;
		ASSERT(firstNode);
		T value = firstNode->value;
		remove(Iterator(firstNode));
		return value;
	}

	T popBack()
	{
		Node* lastNode = Impl::m_last;
		ASSERT(lastNode);
		T value = lastNode->value;
		remove(Iterator(lastNode));
		return value;
	}

	Iterator findFront(const T& value) const
	{
		Node* n = Impl::m_first;
		while (n != nullptr)
		{
			if (value == n->value)
				return Iterator(n);
			n = n->next;
		}
		return Iterator();
	}

	Iterator findBack(const T& value) const
	{
		Node* n = Impl::m_last;
		while (n != nullptr)
		{
			if (value == n->value)
				return Iterator(n);
			n = n->prev;
		}
		return Iterator();
	}

	void clear()
	{
		Node* n = Impl::m_first;
		while (n != nullptr)
		{
			Node* next = n->next;
			freeNode(n);
			n = next;
		}

		Impl::m_first = Impl::m_last = nullptr;
		Impl::m_count = 0;
	}

	void remove(Iterator incidentNode)
	{
		if (incidentNode.atEnd())
			return;

		Impl::unlinkNode(incidentNode.current);
		freeNode(incidentNode.current);
	}

	Iterator insertBefore(const T& value, Iterator incidentNode)
	{
		if (incidentNode.atEnd())
			return Iterator();

		Node* node = allocNode();
		node->value = value;
		Impl::insertNodeBefore(incidentNode.current, node);
		return Iterator(node);
	}

	Iterator insertAfter(const T& value, Iterator incidentNode)
	{
		if (incidentNode.atEnd())
			return Iterator();

		Node* node = allocNode();
		node->value = value;
		Impl::insertNodeAfter(incidentNode.current, node);
		return Iterator(node);
	}

	void moveToFront(Iterator incidentNode)
	{
		if (incidentNode.atEnd())
			return;

		Impl::unlinkNode(incidentNode.current);
		Impl::insertNodeFirst(incidentNode.current);
	}

	void moveToBack(Iterator incidentNode)
	{
		if (incidentNode.atEnd())
			return;

		Impl::unlinkNode(incidentNode.current);
		Impl::insertNodeLast(incidentNode.current);
	}

	Node* allocNode()
	{
		return STORAGE_TYPE::alloc();
	}

	void freeNode(Node* node)
	{
		return STORAGE_TYPE::free(node);
	}
};

template<typename T>
using List = ListBase<T, DynamicListAllocator<T>>;

template<typename T, int SIZE>
using FixedList = ListBase<T, FixedListAllocator<T, SIZE>>;
