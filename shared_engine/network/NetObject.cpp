#include "core/core_common.h"
#include "NetObject.h"
#include "Buffer.h"

void PackNetworkVariables(void* objectPtr, const NetPropertyMap* map, Networking::Buffer* buffer, Array<uint>& changeList)
{
#ifndef EDITOR
	if (!map->props.numElem())
		return;

	int numWrittenProps = 0;
	CMemoryStream& stream = buffer->GetData();

	const int startPos = stream.Tell();
	stream.Write(&numWrittenProps, 1, sizeof(numWrittenProps));

	for (const NetProperty& prop : map->props)
	{
		if (changeList.numElem() && arrayFindIndex(changeList, prop.offset) == -1)
			continue;

		// write offset
		stream.Write(&prop.nameHash, 1, sizeof(int));

		uintptr_t varPtr = ((uintptr_t)objectPtr) + prop.offset;
		if (prop.type == NETPROP_NETPROP)
		{
			static Array<uint> _empty(PP_SL);
			PackNetworkVariables((void*)varPtr, prop.nestedMap, buffer, _empty);
		}
		else
		{
			stream.Write((void*)varPtr, 1, prop.size);
		}
		numWrittenProps++;
	}

	const int lastPos = stream.Tell();
	stream.Seek(startPos, VS_SEEK_SET);
	stream.Write(&numWrittenProps, 1, sizeof(numWrittenProps));

	stream.Seek(lastPos, VS_SEEK_SET);
#endif
}

void UnpackNetworkVariables(void* objectPtr, const NetPropertyMap* map, Networking::Buffer* buffer)
{
#ifndef EDITOR
	if (!map->props.numElem())
		return;

	const int numWrittenProps = buffer->ReadInt();
	for (int i = 0; i < numWrittenProps; i++)
	{
		const NetProperty* found = nullptr;
		const int nameHash = buffer->ReadInt();
		for (int j = 0; j < map->props.numElem(); j++)
		{
			if (map->props[j].nameHash == nameHash)
			{
				found = &map->props[j];
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