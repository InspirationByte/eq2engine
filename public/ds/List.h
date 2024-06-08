//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Double Linked list
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename T, typename STORAGE_TYPE>
class ListBase;

template <typename T, typename STORAGE_TYPE>
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

	friend class ListBase<T, STORAGE_TYPE>;
};

//------------------------------------------------------

template <typename T, typename STORAGE_TYPE>
struct ListAllocatorBase
{
	using Node = ListNode<T, STORAGE_TYPE>;

	virtual			~ListAllocatorBase() {}

	virtual Node*	alloc() = 0;
	virtual void	free(Node* node) = 0;
};

template<typename T>
class DynamicListAllocator : public ListAllocatorBase<T, DynamicListAllocator<T>>
{
public:
	using Node = ListNode<T, DynamicListAllocator<T>>;

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
class FixedListAllocator : public ListAllocatorBase<T, FixedListAllocator<T, SIZE>>
{
public:
	using Node = ListNode<T, FixedListAllocator<T, SIZE>>;

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

template <typename T, typename STORAGE_TYPE>
class ListBase : STORAGE_TYPE
{
public:
	using Node = ListNode<T, STORAGE_TYPE>;

	struct Iterator
	{
		Node* current{ nullptr };
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
		m_first = other.m_first;
		m_last = other.m_last;
		m_count = other.m_count;
		other.m_first = nullptr;
		other.m_last = nullptr;
		other.m_count = 0;
	}

	ListBase(ListBase&& other) noexcept
		: STORAGE_TYPE(std::move(other))
	{
		m_first = other.m_first;
		m_last = other.m_last;
		m_count = other.m_count;
		other.m_first = nullptr;
		other.m_last = nullptr;
		other.m_count = 0;
	}

	int getCount() const { return m_count; }

	bool prepend(const T& value)
	{
		Node* node = allocNode();
		node->value = value;
		insertNodeFirst(node);
		return true;
	}

	bool prepend(T&& value)
	{
		Node* node = allocNode();
		node->value = std::move(value);
		insertNodeFirst(node);
		return true;
	}

	T& prepend()
	{
		Node* node = allocNode();
		insertNodeFirst(node);
		return node->value;
	}

	bool append(const T& value)
	{
		Node* node = allocNode();
		node->value = value;
		insertNodeLast(node);
		return true;
	}

	bool append(T&& value)
	{
		Node* node = allocNode();
		node->value = std::move(value);
		insertNodeLast(node);
		return true;
	}

	T& append()
	{
		Node* node = allocNode();
		insertNodeLast(node);
		return node->value;
	}

	template<typename COMPARATOR>
	bool insertSorted(const T& value, COMPARATOR compareFunc)
	{
		Node* node = allocNode();
		node->value = value;

		Node* c = m_first;
		while (c != nullptr && compareFunc(c->value, value) <= 0)
		{
			c = c->m_next;
		}

		if (c)
			insertNodeBefore(c, node);
		else
			insertNodeLast(node);

		return true;
	}

	Iterator		first() const { return Iterator(m_first); }
	Iterator		last() const { return Iterator(m_last); }

	Iterator		begin() const { return Iterator(m_first); }
	Iterator		end() const { return Iterator(); }

	const T&		front() const { return m_first->value; }
	T&				front() { return m_first->value; }

	const T&		back() const { return m_last->value; }
	T&				back() { return m_last->value; }

	T popFront()
	{
		Node* firstNode = m_first;
		ASSERT(firstNode);
		T value = firstNode->value;
		remove(Iterator(firstNode));
		return value;
	}

	T popBack()
	{
		Node* lastNode = m_last;
		ASSERT(lastNode);
		T value = lastNode->value;
		remove(Iterator(lastNode));
		return value;
	}

	Iterator findFront(const T& value) const
	{
		Node* n = m_first;
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
		Node* n = m_last;
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
		Node* n = m_first;
		while (n != nullptr)
		{
			Node* next = n->next;
			freeNode(n);
			n = next;
		}

		m_first = m_last = nullptr;
		m_count = 0;
	}

	void remove(Iterator incidentNode)
	{
		if (incidentNode.atEnd())
			return;

		releaseNode(incidentNode.current);
		freeNode(incidentNode.current);
	}

	Iterator insertBefore(const T& value, Iterator incidentNode)
	{
		if (incidentNode.atEnd())
			return Iterator();

		Node* node = allocNode();
		node->value = value;
		insertNodeBefore(incidentNode.current, node);
		return Iterator(node);
	}

	Iterator insertAfter(const T& value, Iterator incidentNode)
	{
		if (incidentNode.atEnd())
			return Iterator();

		Node* node = allocNode();
		node->value = value;
		insertNodeAfter(incidentNode.current, node);
		return Iterator(node);
	}

	void moveToFront(Iterator incidentNode)
	{
		if (incidentNode.atEnd())
			return;

		releaseNode(incidentNode.current);
		insertNodeFirst(incidentNode.current);
	}

	void moveToBack(Iterator incidentNode)
	{
		if (incidentNode.atEnd())
			return;

		releaseNode(incidentNode.current);
		insertNodeLast(incidentNode.current);
	}

private:
	Node*	m_first{ nullptr };
	Node*	m_last{ nullptr };
	int		m_count{ 0 };

	void insertNodeFirst(Node* node)
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

	void insertNodeLast(Node* node)
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

	void insertNodeBefore(Node* at, Node* node)
	{
		Node* prev = at->prev;
		at->prev = node;
		if (prev)
			prev->next = node;
		else
			m_first = node;

		node->next = at;
		node->prev = prev;
		m_count++;
	}

	void insertNodeAfter(Node* at, Node* node)
	{
		Node* next = at->next;
		at->next = node;
		if (next)
			next->prev = node;
		else
			m_last = node;

		node->prev = at;
		node->next = next;
		m_count++;
	}

	void releaseNode(Node* node)
	{
		if (m_first != m_last)
		{
			ASSERT_MSG(!(node->next == nullptr && node->prev == nullptr), "releaseNode - already released node")
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
