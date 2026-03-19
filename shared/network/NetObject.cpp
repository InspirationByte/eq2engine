#include "core/core_common.h"
#include "NetObject.h"

void PackNetworkVariables(const void* objectPtr, const NetPropertyMap* map, IFileStream& stream, ArrayCRef<uint> changeList)
{
#ifndef EDITOR
	if (!map->props.numElem())
		return;

	const int startPos = stream.Tell();

	int numWrittenProps = 0;
	stream.Write(&numWrittenProps, 1, sizeof(numWrittenProps));
	for (const NetProperty& prop : map->props)
	{
		if (changeList.numElem() && arrayFindIndex(changeList, prop.offset) == -1)
			continue;

		++numWrittenProps;
		stream.Write(&prop.nameHash, 1, sizeof(int));

		const void* varPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(objectPtr) + prop.offset);
		if (prop.type == NETPROP_NETPROP)
			PackNetworkVariables(varPtr, prop.nestedMap, stream, ArrayCRef<uint>(nullptr));
		else
			stream.Write(varPtr, 1, prop.size);
	}

	const int lastPos = stream.Tell();
	stream.Seek(startPos, FS_SEEK_SET);
	stream.Write(&numWrittenProps, 1, sizeof(numWrittenProps));

	stream.Seek(lastPos, FS_SEEK_SET);
#endif
}

void UnpackNetworkVariables(void* objectPtr, const NetPropertyMap* map, IFileStream& stream)
{
#ifndef EDITOR
	if (!map->props.numElem())
		return;

	int numWrittenProps = 0;
	stream.ReadObj(numWrittenProps);
	for (int i = 0; i < numWrittenProps; i++)
	{
		const NetProperty* found = nullptr;
		int nameHash = 0;
		stream.ReadObj(nameHash);
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

		void* varPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(objectPtr) + found->offset);
		if (found->type == NETPROP_NETPROP)
			UnpackNetworkVariables(varPtr, found->nestedMap, stream);
		else
			stream.Read(varPtr, found->size, 1);
	}
#endif
}