#pragma once

// Ideas from https://rsdn.org/forum/info/FAQ.cpp.lock-free

template<typename T>
class BoundedQueue
{
public:
	BoundedQueue(BoundedQueue const&) = delete;
	void operator = (BoundedQueue const&) = delete;
	
	BoundedQueue(uint64 bufferSize)
		: m_buffer(new Cell [bufferSize])
		, m_buffer_mask(bufferSize - 1)
	{
		typedef char assert_nothrow [__has_nothrow_assign(T) || __has_trivial_assign(T) || !__is_class(T) ? 1 : -1];
		ASSERT((bufferSize >= 2) && ((bufferSize & (bufferSize - 1)) == 0));

		for (uint64 i = 0; i != bufferSize; i += 1)
			Atomic::Store(m_buffer[i].sequence, i);

		const uint64 _zero = 0;
		Atomic::Store(m_enqueue_pos, _zero);
		Atomic::Store(m_dequeue_pos, _zero);
	}

	~BoundedQueue()
	{
		delete [] m_buffer;
	}

	bool enqueue(T const& data)
	{
		Cell* cell;

		// load current position for adding into queue
		uint64 pos = Atomic::Load(m_enqueue_pos);
		for (;;)
		{
			// getting current element
			cell = &m_buffer[pos & m_buffer_mask];

			// loading state (sequence) of current element
			uint64 seq = Atomic::Load(cell->sequence);
			int64 dif = (int64)seq - (int64)pos;
			
			// element is ready for store?
			if (dif == 0)
			{
				// trying to advance position for adding
				const uint64 newPos = pos + 1;
				if (Atomic::CompareExchangeWeak(m_enqueue_pos, pos, newPos))
					break;
				// if failed start over
			}
			else if (dif < 0)	// element is not ready to be consumed (queue is full or something)
				return false;
			else /* if (dif > 0) */ // someone went ahead - reloading current element and starting over
				pos = Atomic::Load(m_enqueue_pos);
		}

		// at this point we did reserve element for writing

		// storing data
		cell->data = data;

		// marking element as ready for use
		Atomic::Store(cell->sequence, pos + 1);

		return true;
	}

	bool dequeue(T& data)
	{
		Cell* cell;

		// load current position for extracting from queue
		uint64 pos = Atomic::Load(m_dequeue_pos);
		for (;;)
		{
			// getting current element
			cell = &m_buffer[pos & m_buffer_mask];

			// loading state (sequence) of current element
			uint64 seq = Atomic::Load(cell->sequence);
			int64 dif = (int64)seq - (int64)(pos + 1);

			// element is ready for store?
			if (dif == 0)
			{
				// trying to advance position for extracting
				const uint64 newPos = pos + 1;
				if (Atomic::CompareExchangeWeak(m_dequeue_pos, pos, newPos))
					break;
				// if failed start over
			}
			else if (dif < 0) // element is not ready to be consumed (queue is full or something)
				return false;
			else /* if (dif > 0) */ // someone went ahead - reloading current element and starting over
				pos = Atomic::Load(m_dequeue_pos);
		}

		// at this point we did reserve element for reading

		// reading data
		data = cell->data;

		// marking element as ready for next write
		Atomic::Store(cell->sequence, pos + m_buffer_mask + 1);

		return true;
	}

	private:
	struct Cell
	{
		volatile uint64	sequence;
		T				data;
	};

	// TODO: CPUUtils?
	static constexpr const uint64 CACHELINE_SIZE = 64;

	using CachelinePadding = char[CACHELINE_SIZE];

	CachelinePadding	m_pad0;
	Cell* const			m_buffer;
	uint64 const		m_buffer_mask;
	CachelinePadding	m_pad1;
	volatile uint64		m_enqueue_pos;
	CachelinePadding	m_pad2;
	volatile uint64		m_dequeue_pos;
	CachelinePadding	m_pad3;
};
