# framework/ds — engine data structures

`framework/ds` contains the engine's containers, strings, smart pointers and small utilities. They
replace the C++ standard library in engine code (see `codestyle.md`). Most headers are included by
`core/core_common.h`, so `.cpp` files normally need no extra includes for them.

Included by `core_common.h`: `Array.h`, `ArrayRef.h`, `List.h`, `Map.h`, `BitArray.h`, `mempool.h`,
`stringref.h`, `eqstring.h`, `string_utils.h`, `refcounted.h`, `weakptr.h`, `function.h`,
`future.h`, `defer.h`, `range_for.h`, `fluent.h`, `singleton.h`, `rawitem.h`, `IFileStream.h`,
`MemoryStream.h`, `common_types.h`.
Include explicitly when needed: `SlottedArray.h`, `event.h`, `SimpleBinaryHeap.h`, `graph.h`,
`boundedqueue.h`, `align.h`, `sort.h` (pulled in by `Array.h`).

## Naming of methods

Containers (`Array`, `List`, `Map`, `BitArray`, `SlottedArray`, `MemoryPool`) use camelCase methods:
`numElem()`, `append()`, `atEnd()`. Strings, smart pointers, futures and events use PascalCase:
`Length()`, `ToCString()`, `Ptr()`, `GetResult()`, `Subscribe()`. Keep to the existing API.

## Memory tracking (`PP_SL`)

Dynamically allocating containers take a `PPSourceLine` so PPMem can attribute their memory. Pass
`PP_SL` at the construction site; `Array<T>` and `Map<K, V>` have no default constructor.

```cpp
Array<int>			m_items{ PP_SL };
Map<int, EqString>	m_names{ PP_SL };

Array<EqString> list(PP_SL);
```

Nested containers (e.g. `Map<int, Array<int>>`) receive the source line automatically through
`PPSLValueCtor`.

## Arrays — `Array.h`

| Type | Storage |
|---|---|
| `Array<T>` | Heap, grows by granularity (default 16). Requires `PP_SL`. |
| `FixedArray<T, N>` | Inline storage of `N` items, no heap. Asserts on overflow. |
| `BufferedArray<T, N>` | Inline storage of `N` items, switches to heap when exceeded. |

Main operations:

- Size: `numElem()`, `numAllocated()`, `isEmpty()`, `isFull()` (for fixed arrays), `inRange(i)`.
- Access: `operator[]` (asserts on bad index), `front()`, `back()`, `ptr()`.
- Add: `append(obj)`, `append()` (returns reference to a new default item), `appendEmplace(args...)`,
  `append(otherArray)`, `append(ptr, count)`, `insert(obj, index)`, `addUnique(obj)`.
- Remove: `removeIndex(i)`, `fastRemoveIndex(i)` (swap with last, order not kept), `remove(obj)`,
  `fastRemove(obj)`, `removeRange(i, count)`, `popBack()`, `popFront()`.
- Capacity: `reserve(n)`, `resize(n)`, `setNum(n)`, `assureSize(n)`, `assureSizeEmplace(n, args...)`,
  `clear(deallocate = false)`.
- Search: `find(obj)` / `find(predicate)` return an iterator; `numElem(predicate)` counts matches.
- Range-based `for` is supported.

```cpp
FixedArray<EqString, 2> tokens;
tokens.append(text);
if (tokens.isFull())
	...
```

### Array views — `ArrayRef.h`

`ArrayRef<T>` (mutable) and `ArrayCRef<T>` (read-only) are non-owning views over any array type,
a C array or a pointer + count. Use them as function parameters instead of `const Array<T>&` when
the function does not change the array size.

```cpp
void ProcessItems(ArrayCRef<int> items);

ProcessItems(m_items);		// Array<int>
ProcessItems(fixedItems);	// FixedArray<int, 8>
ProcessItems(cArray);		// int[16]
```

### Algorithms — `sort.h`

Free functions working on arrays, views and C arrays:

- `arraySort(arr, comparator)` — introsort; comparator returns `int` like `sortCompare`.
- `arrayFindIndex(arr, key)` — linear search, returns index or `-1`.
- `arrayFindIndexF(arr, predicate)` / `arrayFindIndexF(arr, key, comparator)`.
- `arraySortedFindIndexExt<ESortedFind::...>(arr, key, comparator)` — binary search on a sorted array.
- `arrayReverse(arr)`, `arrayIsSorted(begin, end, comparator)`.
- `arrayEnumerate(arr)` — range-for with index: `for (auto [i, item] : arrayEnumerate(arr))`.

## Lists — `List.h`

Doubly linked list. `List<T>` allocates nodes on the heap (`PP_SL`), `FixedList<T, N>` from an
inline pool. Operations: `append`, `prepend`, `insertBefore`, `insertAfter`, `insertSorted`,
`remove(iterator)`, `popFront`, `popBack`, `findFront`, `findBack`, `moveToFront`, `moveToBack`,
`getCount()`. `LinkedListImpl<TNODE>` links user-defined intrusive nodes that have `prev`/`next`.

## Map — `Map.h`

Ordered map on an AVL tree; keys need `<`, `>` and `==`. Iteration is in key order.

- `insert(key, value)` — inserts or overwrites the existing key; returns an iterator.
- `insertNew(key, value)` — always inserts (multi-map style, duplicate keys allowed);
  `count(key)` returns the number of equal keys.
- `find(key)` returns an iterator; check with `atEnd()` or compare to `end()`.
- `contains(key)`, `operator[]` (inserts a default value when the key is missing), `size()`,
  `isEmpty()`, `clear(deallocate)`.
- `remove(key)`; `remove(iterator)` returns the next iterator.

```cpp
auto it = m_tokens.find(hash);
if (!it.atEnd())
	return &it.value();

for (auto it = m_tokens.begin(); !it.atEnd(); ++it)
	DoSomething(it.key(), *it);

// removing while iterating: continue from the returned iterator
for (auto it = m_tokens.begin(); !it.atEnd();)
{
	if (ShouldRemove(*it))
		it = m_tokens.remove(it);
	else
		++it;
}
```

String keys are usually hashed first: `Map<int, T>` with `StringId24(name)` / `StringId(name)`.

## Other containers

| Type | Header | Purpose |
|---|---|---|
| `SlottedArray<T>` | `SlottedArray.h` | Array with stable indices: `add()` returns a slot index, `remove(idx)` frees it for reuse; `operator()(idx)` / `isValidIdx(idx)` check whether a slot is used |
| `BitArray` | `BitArray.h` | Dynamic bit set: `set`, `operator[]`, `numTrue`, `numFalse`, `resize`, `reset` |
| `MemoryPool<T, N>` | `mempool.h` | Chunked pool allocator for fixed-size objects: `allocate()`, `deallocate()` |
| `SimpleBinaryHeap<T>` | `SimpleBinaryHeap.h` | Min-heap: `Add`, `PopMin`, `HasItems` |
| `BoundedQueue<T>` | `boundedqueue.h` | Fixed-size queue: `enqueue`, `dequeue` |
| `IGraph` | `graph.h` | Interface for Dijkstra search over user graph data |

## Strings — `eqstring.h`, `stringref.h`, `string_utils.h`

| Type | Purpose |
|---|---|
| `EqString` / `EqWString` | Owning `char` / `wchar_t` string |
| `EqStringRef` / `EqWStringRef` | Non-owning view (pointer + length); `_Es("...")` is a shorthand |

- Use `EqStringRef` for read-only string parameters; it accepts `const char*`, `EqString` and literals.
- Access: `ToCString()`, `GetData()`, `Length()`, `IsValid()`; both convert implicitly to `const CH*`.
- Editing `EqString`: `Assign`, `Append`, `AppendFmt`, `Insert`, `Remove`, `ReplaceChar`,
  `ReplaceSubstr`, `Empty`, `Clear`.
- Queries: `Compare`, `CompareCaseIns`, `Find`, `Left`, `Right`, `Mid`, `TrimSpaces`, `TrimChar`,
  `LowerCase`, `UpperCase`.
- Formatting: `EqString::Format("%s/%d", name, value)` — `EqString` arguments are converted with
  `ToCString()` automatically.
- Functions returning `EqStringRef` built with `GetTempString` (`Format`, `LowerCase`, `fnmPath*`,
  ...) point to a temporary buffer. Using the result within the same expression is relatively safe,
  even when several such calls are combined. Assign the result to an `EqString` if it must outlive
  the expression.

```cpp
const EqString filePath = EqString::Format("resources/text_%s/%s.txt", language, prefix);
m_language = m_language.LowerCase();
```

`string_utils.h` also provides:

- `StringId24(str, caseIns)`, `StringId(str, caseIns)` — string hashes; `StringIdConst24("x")` /
  `StringIdConst("x")` compute them at compile time.
- `StringSplit(...)` — split into `Array<EqString>`.
- `fnmPath*` — path helpers: `fnmPathCombine`, `fnmPathExtractExt`, `fnmPathStripExt`,
  `fnmPathExtractName`, `fnmPathFixSeparators`, ...
- `AnsiUnicodeConverter(out, in)` — UTF-8 ⇄ wide string conversion.
- `CString::Compare`, `CString::CompareCaseIns`, `SubString` and other C-string helpers (in `stringref.h`).

## Reference counting and weak pointers — `refcounted.h`, `weakptr.h`

- Derive ref-counted classes from `RefCountedObject<T>`; hold them with `CRefPtr<T>`. Create with
  `CRefPtr_new(T, args...)` or `RefPtr_new<T>(args...)` (both allocate through `PPNew`).
  The object is deleted when the last `CRefPtr` releases it.
- Policies: `RefCountedKeepPolicy` disables auto-deletion; `RefCountedUnsafe` uses non-atomic
  counting for single-threaded objects. `Ref_DeleteObject()` can be overridden.
- Derive from `WeakRefObject<T>` to allow `CWeakPtr<T>`. A weak pointer becomes null when the object
  is destroyed; check it before use. A class may derive from both `RefCountedObject<T>` and
  `WeakRefObject<T>`.
- `CRefPtr`/`CWeakPtr` API: `Ptr()`, `Ref()`, `operator->`, `Assign()`, `Release()`, bool conversion;
  `CWeakPtr::IsSet()` tells whether it was ever assigned.

```cpp
class CMyResource : public RefCountedObject<CMyResource>, public WeakRefObject<CMyResource> { ... };
using CMyResourcePtr = CRefPtr<CMyResource>;

CMyResourcePtr res = CRefPtr_new(CMyResource, args);
CWeakPtr<CMyResource> weak(res.Ptr());
```

## Callables, events, futures

- `EqFunction<R(Args...)>` (`function.h`) — callable wrapper with small-buffer storage; use instead
  of `std::function`.
- `Event<void(Args...)>` (`event.h`) — multicast event. `Subscribe(func, runOnce)` / `operator+=`
  return a subscription; keep it in `Event<...>::Sub` to unsubscribe automatically when `Sub` is
  destroyed. Raise with `operator()(args...)`.
- `Promise<T>` / `Future<T>` (`future.h`) — thread-safe asynchronous result. The producer calls
  `SetResult` or `SetError`; the consumer uses `HasResult`, `GetResult`, `HasErrors`, `Wait`, or
  `AddCallback`. `Future<T>::Succeed(value)` / `Future<T>::Failure(code, message)` create resolved
  futures.

## Utilities

| Facility | Header | Usage |
|---|---|---|
| `defer { ... };` | `defer.h` | Runs the block at scope exit |
| `RangeFor(from, to)` | `range_for.h` | Inclusive integer range for range-based `for`; `RangeForMixin` builds custom ranges |
| `FLUENT_BEGIN_TYPE` / `FLUENT_SET` | `fluent.h` | Generates `Builder<T>` with chained setters |
| `CStaticAutoPtr<T>`, `CSingletonAbstract<T>` | `singleton.h` | Lazily constructed static instances |
| `RawItem<T, Align>` | `rawitem.h` | Uninitialized aligned storage for manual construction |
| `IFileStream`, `CMemoryStream` | `IFileStream.h`, `MemoryStream.h` | Stream interface and in-memory stream; `VSRead`/`VSWrite` serialize objects, arrays and strings |
| `QuickSwap`, `min`, `max`, `MAKECHAR4` | `common_types.h` | Basic helpers and integer types (`int64`, `uint`, `ubyte`, ...) |
| `ALIGN`, `ALIGNED_TYPE` | `align.h` | Alignment helpers |
