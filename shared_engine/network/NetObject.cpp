#include "core/core_common.h"
#include "NetObject.h"

void CNetworkedObject::OnPackMessage(Networking::Buffer* buffer, Array<uint>& changeList)
{
	// packs all data
	netvariablemap_t* map = GetNetworkTableMap();
	while (map)
	{
		PackNetworkVariables(map, buffer, changeList);
		map = map->m_baseMap;
	}
}

void CNetworkedObject::OnUnpackMessage(Networking::Buffer* buffer)
{
	// unpacks all data
	netvariablemap_t* map = GetNetworkTableMap();
	while (map)
	{
		UnpackNetworkVariables(map, buffer);
		map = map->m_baseMap;
	}
}

inline void PackNetworkVariables(void* object, const netvariablemap_t* map, IVirtualStream* stream, Array<uint>& changeList)
{
	if (map->m_numProps == 1 && map->m_props[0].size == 0)
		return;

	int numWrittenProps = 0;

	const int startPos = stream->Tell();
	stream->Write(&numWrittenProps, 1, sizeof(numWrittenProps));

	for (int i = 0; i < map->m_numProps; i++)
	{
		const netprop_t& prop = map->m_props[i];

		if (changeList.numElem() && arrayFindIndex(changeList, prop.offset) == -1)
			continue;

		// write offset
		stream->Write(&prop.nameHash, 1, sizeof(int));

		char* varData = ((char*)object) + prop.offset;

		if (prop.type == NETPROP_NETPROP)
		{
			static Array<uint> _empty(PP_SL);
			PackNetworkVariables(varData, prop.nestedMap, stream, _empty);

			numWrittenProps++;

			continue;
		}

		// write data
		stream->Write(varData, 1, prop.size);
		numWrittenProps++;
	}

	const int lastPos = stream->Tell();
	stream->Seek(startPos, VS_SEEK_SET);
	stream->Write(&numWrittenProps, 1, sizeof(numWrittenProps));

	stream->Seek(lastPos, VS_SEEK_SET);
}

template <class T>
static void UnpackNetworkVariables(T object, const netvariablemap_t* map, Networking::Buffer* buffer)
{
#ifndef EDITOR
	if (map->m_numProps == 1 && map->m_props[0].size == 0)
		return;

	int numWrittenProps = buffer->ReadInt();

	for (int i = 0; i < numWrittenProps; i++)
	{
		netprop_t* found = nullptr;

		int nameHash = buffer->ReadInt();

		for (int j = 0; j < map->m_numProps; j++)
		{
			if (map->m_props[j].nameHash == nameHash)
			{
				found = &map->m_props[j];
				break;
			}
		}

		if (!found)
		{
			MsgError("UnpackNetworkVariables - invalid prop!\n");
			continue;
		}

		uintptr_t varPtr = ((uintptr_t)object + found->offset);

		if (found->type == NETPROP_NETPROP)
		{
			UnpackNetworkVariables(varPtr, found->nestedMap, buffer);
		}
		else
		{
			void* stackBuf = stackalloc(found->size);

			buffer->ReadData(stackBuf, found->size);

			memcpy((void*)varPtr, stackBuf, found->size);
		}
	}
#endif
}

void CNetworkedObject::PackNetworkVariables(const netvariablemap_t* map, Networking::Buffer* buffer, Array<uint>& changeList)
{
#ifndef EDITOR
	CMemoryStream varsData(nullptr, VS_OPEN_WRITE | VS_OPEN_READ, 2048, PP_SL);
	::PackNetworkVariables(this, map, &varsData, changeList);

	// write to net buffer
	buffer->WriteData(varsData.GetBasePointer(), varsData.Tell());
#endif
}

void CNetworkedObject::UnpackNetworkVariables(const netvariablemap_t* map, Networking::Buffer* buffer)
{
	::UnpackNetworkVariables(this, map, buffer);
}