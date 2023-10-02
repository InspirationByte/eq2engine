#include "core/core_common.h"
#include "NetObject.h"
#include "Buffer.h"

void PackNetworkVariables(void* objectPtr, const netvariablemap_t* map, Networking::Buffer* buffer, Array<uint>& changeList)
{
#ifndef EDITOR
	if (map->m_numProps == 1 && map->m_props[0].size == 0)
		return;

	int numWrittenProps = 0;
	CMemoryStream& stream = buffer->GetData();

	const int startPos = stream.Tell();
	stream.Write(&numWrittenProps, 1, sizeof(numWrittenProps));

	for (int i = 0; i < map->m_numProps; i++)
	{
		const netprop_t& prop = map->m_props[i];

		if (changeList.numElem() && arrayFindIndex(changeList, prop.offset) == -1)
			continue;

		// write offset
		stream.Write(&prop.nameHash, 1, sizeof(int));

		uintptr_t varData = ((uintptr_t)objectPtr) + prop.offset;
		if (prop.type == NETPROP_NETPROP)
		{
			static Array<uint> _empty(PP_SL);
			PackNetworkVariables((void*)varData, prop.nestedMap, buffer, _empty);
		}
		else
		{
			stream.Write((void*)varData, 1, prop.size);
		}
		numWrittenProps++;
	}

	const int lastPos = stream.Tell();
	stream.Seek(startPos, VS_SEEK_SET);
	stream.Write(&numWrittenProps, 1, sizeof(numWrittenProps));

	stream.Seek(lastPos, VS_SEEK_SET);
#endif
}

void UnpackNetworkVariables(void* objectPtr, const netvariablemap_t* map, Networking::Buffer* buffer)
{
#ifndef EDITOR
	if (map->m_numProps == 1 && map->m_props[0].size == 0)
		return;

	const int numWrittenProps = buffer->ReadInt();
	for (int i = 0; i < numWrittenProps; i++)
	{
		const netprop_t* found = nullptr;
		const int nameHash = buffer->ReadInt();
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

		uintptr_t varPtr = ((uintptr_t)objectPtr + found->offset);
		if (found->type == NETPROP_NETPROP)
		{
			UnpackNetworkVariables((void*)varPtr, found->nestedMap, buffer);
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

void CNetworkedObject::OnPackMessage(netvariablemap_t* map, Networking::Buffer* buffer, Array<uint>& changeList)
{
	// packs all data
	while (map)
	{
		PackNetworkVariables(this, map, buffer, changeList);
		map = map->m_baseMap;
	}
}

void CNetworkedObject::OnUnpackMessage(netvariablemap_t* map, Networking::Buffer* buffer)
{
	// unpacks all data
	while (map)
	{
		UnpackNetworkVariables(this, map, buffer);
		map = map->m_baseMap;
	}
}
