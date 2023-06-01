#pragma once

// based on https://github.com/moya-lang/Allocator

template <typename T, int CHUNK_ITEMS = 1024>
class MemoryPool
{
public:
	MemoryPool(const MemoryPool& memoryPool) = delete;
	MemoryPool operator =(MemoryPool&& memoryPool) = delete;
	MemoryPool operator =(const MemoryPool& memoryPool) = delete;

	MemoryPool(PPSourceLine sl)
		: m_sl(sl)
	{
	}

	MemoryPool(MemoryPool&& other) 
		: m_sl(other.m_sl)
		, m_firstFreeBlock(other.m_firstFreeBlock)
		, m_firstBuffer(other.m_firstBuffer)
		, m_bufferedBlocks(other.m_bufferedBlocks)
	{
		m_firstFreeBlock = nullptr;
		m_firstBuffer = nullptr;
		m_bufferedBlocks = CHUNK_ITEMS;
	}

	~MemoryPool()
	{
		clear();
	}

	void clear()
	{
		while (m_firstBuffer)
		{
			Buffer* buffer = m_firstBuffer;
			m_firstBuffer = buffer->next;
			delete buffer;
		}
		m_firstFreeBlock = nullptr;
		m_firstBuffer = nullptr;
		m_bufferedBlocks = CHUNK_ITEMS;
	}

	T* allocate()
	{
		if (m_firstFreeBlock)
		{
			Block* block = m_firstFreeBlock;
			m_firstFreeBlock = block->next;
			return reinterpret_cast<T*>(block);
		}

		if (m_bufferedBlocks >= CHUNK_ITEMS)
		{
			m_firstBuffer = PPNewSL(m_sl) Buffer(m_firstBuffer);
			m_bufferedBlocks = 0;
		}

		T* ptr = m_firstBuffer->getBlock(m_bufferedBlocks++);
		return ptr;
	}

	void deallocate(T* pointer)
	{
		Block* block = reinterpret_cast<Block*>(pointer);
		block->next = m_firstFreeBlock;
		m_firstFreeBlock = block;
	}

	const PPSourceLine getSL() const { return m_sl; }

private:
	// internal structure to link free item data
	struct Block
	{
		Block*			next{ nullptr };
	};

	static constexpr const int blockSize = max(sizeof(T), sizeof(Block));

	struct Buffer
	{
		uint8			data[blockSize * CHUNK_ITEMS];
		Buffer* const	next{ nullptr };

		Buffer(Buffer* next) : next(next) {}
		T*				getBlock(int index) { return reinterpret_cast<T*>(&data[blockSize * index]); }
	};

	const PPSourceLine 	m_sl;
	Block*				m_firstFreeBlock{ nullptr };
	Buffer*				m_firstBuffer{ nullptr };
	int					m_bufferedBlocks{ CHUNK_ITEMS };
};


// std-compatible allocator
template <class T, int CHUNK_ITEMS = 1024>
class PoolAllocator : private MemoryPool<T, CHUNK_ITEMS>
{
public:

	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

	template <class U>
	struct rebind
	{
		typedef PoolAllocator<U, CHUNK_ITEMS> other;
	};

	pointer allocate(size_type n, const void* hint = 0)
	{
		if (n != 1 || hint)
			return nullptr;

		return MemoryPool<T, CHUNK_ITEMS>::allocate();
	}

	void deallocate(pointer p, size_type n)
	{
		MemoryPool<T, CHUNK_ITEMS>::deallocate(p);
	}

	void construct(pointer p, const_reference val)
	{
		new (p) T(val);
	}

	void destroy(pointer p)
	{
		p->~T();
	}
};