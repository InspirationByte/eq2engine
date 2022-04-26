//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: RB-tree map
//////////////////////////////////////////////////////////////////////////////////

#ifndef MAP_H
#define MAP_H

#include <new>
#include "core/dktypes.h"
#include "core/platform/assert.h"

template<typename K, typename V>
class Map
{
private:
	struct Item;
public:
	class Iterator
	{
	public:
		Iterator() : item(nullptr) {}
		const K& key() const { return item->key; }
		const V& value() const { return item->value; }
		const V& operator*() const { return item->value; }
		V& operator*() { return item->value; }
		const V* operator->() const { return &item->value; }
		V* operator->() { return &item->value; }
		const Iterator& operator++() { item = item->next; return *this; }
		const Iterator& operator--() { item = item->prev; return *this; }
		Iterator operator++() const { return item->next; }
		Iterator operator--() const { return item->prev; }
		bool operator==(const Iterator& other) const { return item == other.item; }
		bool operator!=(const Iterator& other) const { return item != other.item; }

	private:
		Item* item;

		Iterator(Item* item) : item(item) {}

		friend class Map;
	};

public:
	Map() : m_end(&m_endItem), m_begin(&m_endItem), m_root(nullptr), m_size(0), m_freeItem(nullptr), m_blocks(nullptr)
	{
		m_endItem.parent = nullptr;
		m_endItem.prev = nullptr;
		m_endItem.next = nullptr;
	}

	Map(const Map& other) : m_end(&m_endItem), m_begin(&m_endItem), m_root(nullptr), m_size(0), m_freeItem(nullptr), m_blocks(nullptr)
	{
		m_endItem.parent = nullptr;
		m_endItem.prev = nullptr;
		m_endItem.next = nullptr;
		for (const Item* i = other.m_begin.item, *end = &other.m_endItem; i != end; i = i->next)
			insert(i->key, i->value);
	}

	~Map()
	{
		for (Item* i = m_begin.item, *end = &m_endItem; i != end; i = i->next)
			i->~Item();
		for (ItemBlock* i = m_blocks, *next; i; i = next)
		{
			next = i->next;
			free(i);
		}
	}

	Map& operator=(const Map& other)
	{
		clear();
		for (const Item* i = other.m_begin.item, *end = &other.m_endItem; i != end; i = i->next)
			insert(i->key, i->value);
		return *this;
	}

	const Iterator& begin() const { return m_begin; }
	const Iterator& end() const { return m_end; }

	const V& front() const { return m_begin.item->value; }
	const V& back() const { return m_end.item->prev->value; }

	V& front() { return m_begin.item->value; }
	V& back() { return m_end.item->prev->value; }

	Iterator removeFront() { return remove(m_begin); }
	Iterator removeBack() { return remove(m_end.item->prev); }

	int size() const { return m_size; }
	bool isEmpty() const { return !m_root; }

	void clear()
	{
		for (Item* i = m_begin.item, *end = &m_endItem; i != end; i = i->next)
		{
			i->~Item();
			i->prev = m_freeItem;
			m_freeItem = i;
		}
		m_begin.item = &m_endItem;
		m_endItem.prev = nullptr;
		m_size = 0;
		m_root = nullptr;
	}

	Iterator find(const K& key) const
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
		return insert(&m_root, nullptr, key);
	}

	Iterator insert(const K& key, const V& value)
	{
		Iterator it = insert(&m_root, nullptr, key);
		it.item->value = value;
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
			Iterator it = insert(&m_root, nullptr, i.item->key, i.item->value);
			for (++i; i != other.m_end; ++i)
				it = insert(it, i.item->key, i.item->value);
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
		item->prev = m_freeItem;
		m_freeItem = item;
		return item->next;
	}

private:
	struct Item
	{
		K key;
		V value;
		Item* parent;
		Item* left;
		Item* right;
		Item* next;
		Item* prev;
		int height;
		int slope;

		Item()
			: key(), value()
		{}

		void set(Item* _parent, const K& _key)
		{
			new(this) Item(); // quite interesting but this is the only case where it works!!!
			key = _key;
			parent = _parent;
			left = nullptr;
			right = nullptr;
			height = 1;
			slope = 0;
		}

		void updateHeightAndSlope()
		{
			ASSERT(!parent || this == parent->left || this == parent->right);
			int leftHeight = left ? left->height : 0;
			int rightHeight = right ? right->height : 0;
			slope = leftHeight - rightHeight;
			height = (leftHeight > rightHeight ? leftHeight : rightHeight) + 1;
		}
	};
	struct ItemBlock
	{
		ItemBlock* next;
	};

private:
	Iterator m_end;
	Iterator m_begin;
	Item m_endItem;
	Item* m_root;
	Item* m_freeItem;
	ItemBlock* m_blocks;
	int m_size;

private:
	Iterator insert(Item** cell, Item* parent, const K& key)
	{
	begin:
		ASSERT(!parent || cell == &parent->left || cell == &parent->right);
		Item* position = *cell;
		if (!position)
		{
			Item* item = m_freeItem;
			if (!item)
			{
				const int allocatedSize = sizeof(ItemBlock) + sizeof(Item);
				ItemBlock* itemBlock = (ItemBlock*)malloc(allocatedSize);
				itemBlock->next = m_blocks;
				m_blocks = itemBlock;
				for (Item* i = (Item*)(itemBlock + 1), *end = i + (allocatedSize - sizeof(ItemBlock)) / sizeof(Item); i < end; ++i)
				{
					i->prev = item;
					item = i;
				}
				m_freeItem = item;
			}

			item->set(parent, key);
			m_freeItem = item->prev;

			*cell = item;
			++m_size;
			if (!parent)
			{ // first item
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
			if (key > position->key)
			{
				cell = &position->right;
				parent = position;
				goto begin;
			}
			else if (key < position->key)
			{
				cell = &position->left;
				parent = position;
				goto begin;
			}
			else
			{
				return position;
			}
		}
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

using _EMPTY_VALUE = struct {};

template<typename T>
using Set = Map<T, _EMPTY_VALUE>;

#endif // MAP_H