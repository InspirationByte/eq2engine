//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Key-Value map
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template<typename K, typename V>
class Map
{
private:
	struct Item;
public:
	class Iterator
	{
	public:
		Iterator() = default;

		const K&		key() const { return item->key; }
		const V&		value() const { return *item->value; }

		bool			atEnd() const { return !item || !item->value; };

		const V&		operator*() const { return *item->value; }
		V&				operator*() { return *item->value; }
		const V*		operator->() const { return item->value; }
		V*				operator->() { return item->value; }
		bool			operator==(const Iterator& other) const { return item == other.item; }
		bool			operator!=(const Iterator& other) const { return item != other.item; }

		const Iterator& operator++() { item = item->next; return *this; }
		const Iterator& operator--() { item = item->prev; return *this; }
		Iterator		operator++() const { return item->next; }
		Iterator		operator--() const { return item->prev; }

	private:
		Item*			item{ nullptr };

		Iterator(Item* item)
			: item(item) 
		{}

		friend class Map;
	};

public:
	Map(const PPSourceLine& sl)
		: m_pool(sl), m_end(&m_endItem), m_begin(&m_endItem)
	{
		m_endItem.parent = nullptr;
		m_endItem.prev = nullptr;
		m_endItem.next = nullptr;
	}

	Map(const Map& other)
		: m_pool(other.m_pool.getSL()), m_end(&m_endItem), m_begin(&m_endItem)
	{
		for (const Item* i = other.m_begin.item, *end = &other.m_endItem; i != end; i = i->next)
			insert(i->key, *i->value);
	}

	Map(Map&& other) noexcept
		: m_pool(std::move(other.m_pool)), m_end(&m_endItem), m_begin(&m_endItem)
		, m_endItem(std::move(other.m_endItem))
		, m_root(other.m_root)
		, m_size(other.m_size)

	{
		if(other.m_begin.item != &other.m_endItem)
			m_begin = Iterator(other.m_begin.item);

		// fix up references
		for (Item* i = m_begin.item, *end = &m_endItem; i != end; i = i->next)
		{
			if(&other.m_endItem == i->parent)
				i->parent = &m_endItem;

			if(&other.m_endItem == i->right)
				i->right = &m_endItem;

			if(&other.m_endItem == i->next)
				i->next = &m_endItem;
		}

		other.m_root = nullptr;
		other.m_size = 0;
	}

	~Map()
	{
		for (Item* i = m_begin.item, *end = &m_endItem; i != end; i = i->next)
			i->~Item();
	}

	Map& operator=(const Map& other)
	{
		clear();
		for (const Item* i = other.m_begin.item, *end = &other.m_endItem; i != end; i = i->next)
			insert(i->key, *i->value);
		return *this;
	}

	const Iterator& begin() const { return m_begin; }
	const Iterator& end() const { return m_end; }

	const V& front() const { return *m_begin.item->value; }
	const V& back() const { return *m_end.item->prev->value; }

	V& front() { return *m_begin.item->value; }
	V& back() { return *m_end.item->prev->value; }

	Iterator removeFront() { return remove(m_begin); }
	Iterator removeBack() { return remove(m_end.item->prev); }

	int size() const { return m_size; }
	bool isEmpty() const { return !m_root; }

	void swap(Map& other)
	{
		QuickSwap(m_root, other.m_root);
		QuickSwap(m_pool, other.m_pool);
		QuickSwap(m_size, other.m_size);
		QuickSwap(m_begin, other.m_begin);
		QuickSwap(m_end, other.m_end);
		QuickSwap(m_endItem, other.m_endItem);

		// Fix up references to m_endItem
		for (Item* i = m_begin.item, *end = &m_endItem; i != end; i = i->next)
		{
			if (i->parent == &other.m_endItem)
				i->parent = &m_endItem;
			if (i->left == &other.m_endItem)
				i->left = &m_endItem;
			if (i->right == &other.m_endItem)
				i->right = &m_endItem;
			if (i->next == &other.m_endItem)
				i->next = &m_endItem;
			if (i->prev == &other.m_endItem)
				i->prev = &m_endItem;
		}

		for (Item* i = other.m_begin.item, *end = &other.m_endItem; i != end; i = i->next)
		{
			if (i->parent == &m_endItem)
				i->parent = &other.m_endItem;
			if (i->left == &m_endItem)
				i->left = &other.m_endItem;
			if (i->right == &m_endItem)
				i->right = &other.m_endItem;
			if (i->next == &m_endItem)
				i->next = &other.m_endItem;
			if (i->prev == &m_endItem)
				i->prev = &other.m_endItem;
		}
	}

	void clear(bool deallocate = false)
	{
		for (Item* i = m_begin.item, *end = &m_endItem; i != end;)
		{
			i->~Item();
			Item* next = i->next;
			m_pool.deallocate((ItemVal*)i);
			i = next;
		}

		if (deallocate)
			m_pool.clear();

		m_begin.item = &m_endItem;
		m_endItem.prev = nullptr;
		m_size = 0;
		m_root = nullptr;
	}

	Iterator find(const K& key) const // TODO: multi-map's findAll which returns MultiIterator
	{
		for (Item* item = m_root; item; )
		{
			if (key > item->key)
			{
				item = item->right;
				continue;
			}
			else if (key < item->key)
			{
				item = item->left;
				continue;
			}
			else
				return item;
		}
		return m_end;
	}

	bool contains(const K& key) const
	{
		return find(key) != m_end; 
	}

	int count(const K& key) const
	{
		Iterator it = find(key);
		if (it == m_end)
			return 0;
		int count = 1;
		for (Item* item = it.item->next; item && item->key == key; item = item->next)
			++count;
		return count;
	}

	V& operator[](const K& key)
	{
		Iterator it = find(key);

		if (it != m_end)
			return *it;
		return *insert(key);
	}

	Iterator insert(const Iterator& position, const K& key)
	{
		Item* insertPos = position.item;
		if (insertPos == &m_endItem)
		{
			Item* prev = insertPos->prev;
			if (prev && key > prev->key)
				return insert(&prev->right, prev, key);
		}
		else
		{
			//if (MULTIMAP)
			//{
			//	if (key < insertPos->key)
			//	{
			//		Item* prev = insertPos->prev;
			//		if (!prev || key >= prev->key)
			//			return insert(&insertPos->left, insertPos, key);
			//	}
			//	else
			//	{
			//		Item* next = insertPos->next;
			//		if (next == &m_endItem || key <= next->key)
			//			return insert(&insertPos->right, insertPos, key);
			//	}
			//}
			//else
			{
				if (key < insertPos->key)
				{
					Item* prev = insertPos->prev;
					if (!prev || key > prev->key)
						return insert(&insertPos->left, insertPos, key);
				}
				else if (key > insertPos->key)
				{
					Item* next = insertPos->next;
					if (next == &m_endItem || key < next->key)
						return insert(&insertPos->right, insertPos, key);
				}
				else
				{
					return insertPos;
				}
			}
		}
		return insert(&m_root, nullptr, key);
	}

	Iterator insert(const K& key, const V& value)
	{
		Iterator it = insert(key);
		*it.item->value = value;
		return it;
	}

	Iterator insert(const K& key, V&& value)
	{
		Iterator it = insert(key);
		*it.item->value = std::move(value);
		return it;
	}

	Iterator insert(const K& key)
	{
		return insert(&m_root, nullptr, key);
	}

	void insert(const Map& other)
	{
		if (other.m_root)
		{
			Iterator i = other.m_begin;
			Iterator it = insert(&m_root, nullptr, i.item->key, *i.item->value);
			for (++i; i != other.m_end; ++i)
				it = insert(it, i.item->key, *i.item->value);
		}
	}

	//void checkTree(Item* item, Item* parent)
	//{
	//  ASSERT(item->parent == parent);
	//  int height = item->height;
	//  int slope = item->slope;
	//  if(item->left)
	//    checkTree(item->left, item);
	//  if(item->right)
	//    checkTree(item->right, item);
	//  int leftHeight = item->left ? item->left->height : 0;
	//  int rightHeight = item->right ? item->right->height : 0;
	//  ASSERT(height == (leftHeight > rightHeight ? leftHeight : rightHeight) + 1);
	//  ASSERT(slope == leftHeight - rightHeight);
	//  ASSERT(slope <= 1);
	//  ASSERT(slope >= -1);
	//}

	void remove(const K& key)
	{
		Iterator it = find(key);
		if (it != m_end)
			remove(it);
	}

	Iterator remove(const Iterator& it)
	{
		Item* item = it.item;
		Item* origParent;
		Item* parent = origParent = item->parent;
		Item** cell = parent ? (item == parent->left ? &parent->left : &parent->right) : &m_root;
		ASSERT(*cell == item);
		Item* left = item->left;
		Item* right = item->right;

		if (!left && !right)
		{
			*cell = nullptr;
			goto rebalParentUpwards;
		}

		if (!left)
		{
			*cell = right;
			right->parent = parent;
			goto rebalParentUpwards;
		}

		if (!right)
		{
			*cell = left;
			left->parent = parent;
			goto rebalParentUpwards;
		}

		if (left->height < right->height)
		{
			Item* next = item->next;
			ASSERT(!next->left);
			Item* nextParent = next->parent;
			if (nextParent == item)
			{
				ASSERT(next == right);
				*cell = next;
				next->parent = parent;

				next->left = left;
				left->parent = next;
				parent = next;
				goto rebalParent;
			}

			// unlink next
			ASSERT(next == nextParent->left);
			Item* nextRight = next->right;
			nextParent->left = nextRight;
			if (nextRight)
				nextRight->parent = nextParent;

			// put next in cell
			*cell = next;
			next->parent = parent;

			// link left to next left
			next->left = left;
			left->parent = next;

			// link right to next right
			next->right = right;
			right->parent = next;

			// rebal old next->parent
			parent = nextParent;
			ASSERT(parent);
			goto rebalParent;
		}
		else
		{
			Item* prev = item->prev;
			ASSERT(!prev->right);
			Item* prevParent = prev->parent;
			if (prevParent == item)
			{
				ASSERT(prev == left);
				*cell = prev;
				prev->parent = parent;

				prev->right = right;
				right->parent = prev;
				parent = prev;
				goto rebalParent;
			}

			// unlink prev
			ASSERT(prev == prevParent->right);
			Item* prevLeft = prev->left;
			prevParent->right = prevLeft;
			if (prevLeft)
				prevLeft->parent = prevParent;

			// put prev in cell
			*cell = prev;
			prev->parent = parent;

			// link right to prev right
			prev->right = right;
			right->parent = prev;

			// link left to prev left
			prev->left = left;
			left->parent = prev;

			// rebal old prev->parent
			parent = prevParent;
			ASSERT(parent);
			goto rebalParent;
		}

		// rebal parent upwards
	rebalParent:
		do
		{
			int oldHeight = parent->height;
			parent->updateHeightAndSlope();
			parent = rebal(parent);
			if (oldHeight == parent->height)
			{
				parent = *cell;
				parent->updateHeightAndSlope();
				parent = rebal(parent);
				parent = parent->parent;
				break;
			}
			parent = parent->parent;
		} while (parent != origParent);
	rebalParentUpwards:
		while (parent)
		{
			int oldHeight = parent->height;
			parent->updateHeightAndSlope();
			parent = rebal(parent);
			if (oldHeight == parent->height)
				break;
			parent = parent->parent;
		}

		if (!item->prev)
			(m_begin.item = item->next)->prev = nullptr;
		else
			(item->prev->next = item->next)->prev = item->prev;
		--m_size;

		item->~Item();
		Item* next = item->next;
		m_pool.deallocate((ItemVal*)item);

		return next;
	}

private:
	struct Item
	{
		K		key;
		V*		value{ nullptr };
		Item*	parent{ nullptr };
		Item*	left{ nullptr };
		Item*	right{ nullptr };
		Item*	next{ nullptr };
		Item*	prev{ nullptr };
		int		height{ 1 };
		int		slope{ 0 };

		Item()
			: key()
		{}

		Item(Item* parent, const K& key) 
			: parent(parent), key(key), value((V*)(this + 1))
		{}

		~Item()
		{
			if (value) { value->~V(); }
		}

		void updateHeightAndSlope()
		{
			ASSERT(!parent || this == parent->left || this == parent->right);
			const int leftHeight = left ? left->height : 0;
			const int rightHeight = right ? right->height : 0;
			slope = leftHeight - rightHeight;
			height = (leftHeight > rightHeight ? leftHeight : rightHeight) + 1;
		}
	};

private:
	struct ItemVal {
		Item i;
		V v;
	};
	MemoryPool<ItemVal, 32>	m_pool;
	Iterator 			m_end;
	Iterator 			m_begin;
	Item 				m_endItem;
	Item* 				m_root{ nullptr };
	int 				m_size{ 0 };

private:
	Iterator insert(Item** cell, Item* parent, const K& key)
	{
	begin:
		ASSERT(!parent || cell == &parent->left || cell == &parent->right);
		Item* position = *cell;
		if (!position)
		{
			ItemVal* iv = m_pool.allocate();
			Item* item = &iv->i;

			new (item) Item(parent, key);
			PPSLPlacementNew<V>(item + 1, m_pool.getSL());

			*cell = item;
			++m_size;
			if (!parent)
			{
				// first item
				ASSERT(m_begin.item == &m_endItem);
				item->prev = nullptr;
				item->next = m_begin.item;
				m_begin.item = item;
				m_endItem.prev = item;
			}
			else
			{
				Item* insertPos = cell == &parent->right ?
					parent->next : // insert after parent
					parent; // insert before parent

				if ((item->prev = insertPos->prev))
					insertPos->prev->next = item;
				else
					m_begin.item = item;
				item->next = insertPos;
				insertPos->prev = item;

				// neu:
				int oldHeight;
				do
				{
					oldHeight = parent->height;
					parent->updateHeightAndSlope();
					parent = rebal(parent);
					if (oldHeight == parent->height)
						break;
					parent = parent->parent;
				} while (parent);
			}
			return item;
		}
		else
		{
			if (nextCell(key, position, &cell, &parent))
			{
				return position;
			}
			goto begin;
		}
	}

	//template<bool MULTI>
	//bool nextCell(const K& key, Item* position, Item*** cell, Item** parent);

	// multi-map
	//template<>
	//bool nextCell<true>(const K& key, Item* position, Item*** cell, Item** parent)
	//{
	//	if (key < position->key)
	//	{
	//		*cell = &position->left;
	//		*parent = position;
	//	}
	//	else
	//	{
	//		*cell = &position->right;
	//		*parent = position;
	//	}
	//	return false;
	//}

	// map
	bool nextCell(const K& key, Item* position, Item*** cell, Item** parent)
	{
		if (key > position->key)
		{
			*cell = &position->right;
			*parent = position;
			return false;
		}
		else if (key < position->key)
		{
			*cell = &position->left;
			*parent = position;
			return false;
		}

		return true;
	}

	Item* rebal(Item* item)
	{
		if (item->slope > 1)
		{
			ASSERT(item->slope == 2);
			Item* parent = item->parent;
			Item*& cell = parent ? (parent->left == item ? parent->left : parent->right) : m_root;
			ASSERT(cell == item);
			shiftr(cell);
			ASSERT((cell->slope < 0 ? -cell->slope : cell->slope) <= 1);
			return cell;
		}
		else if (item->slope < -1)
		{
			ASSERT(item->slope == -2);
			Item* parent = item->parent;
			Item*& cell = parent ? (parent->left == item ? parent->left : parent->right) : m_root;
			ASSERT(cell == item);
			shiftl(cell);
			ASSERT((cell->slope < 0 ? -cell->slope : cell->slope) <= 1);
			return cell;
		}
		return item;
	}

	static void rotr(Item*& cell)
	{
		Item* oldTop = cell;
		Item* result = oldTop->left;
		Item* tmp = result->right;
		result->parent = oldTop->parent;
		result->right = oldTop;
		if ((oldTop->left = tmp))
			tmp->parent = oldTop;
		oldTop->parent = result;
		cell = result;
		oldTop->updateHeightAndSlope();
		result->updateHeightAndSlope();
	}

	static void rotl(Item*& cell)
	{
		Item* oldTop = cell;
		Item* result = oldTop->right;
		Item* tmp = result->left;
		result->parent = oldTop->parent;
		result->left = oldTop;
		if ((oldTop->right = tmp))
			tmp->parent = oldTop;
		oldTop->parent = result;
		cell = result;
		oldTop->updateHeightAndSlope();
		result->updateHeightAndSlope();
	}

	static void shiftr(Item*& cell)
	{
		Item* oldTop = cell;
		if (oldTop->left->slope == -1)
			rotl(oldTop->left);
		rotr(cell);
	}

	static void shiftl(Item*& cell)
	{
		Item* oldTop = cell;
		if (oldTop->right->slope == 1)
			rotr(oldTop->right);
		rotl(cell);
	}
};

struct _EMPTY_VALUE{}; // GCC doesn't like using = struct {}, produces linkage errors

template<typename K>
using Set = Map<K, _EMPTY_VALUE>;

//template<typename K, typename V>
//using MultiMap = Map<K, V>;


// nesting Source-line constructor helper
template<typename K, typename V>
struct PPSLValueCtor<Map<K, V>>
{
	Map<K,V> x;
	PPSLValueCtor<Map<K,V>>(const PPSourceLine& sl) : x(sl) {}
};
