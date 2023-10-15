//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: a mesh builder designed for dynamic meshes (material system)
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "materialsystem1/renderers/ShaderAPI_defs.h"

/*

USAGE:

#include "materialsystem/IMaterialSystem.h"
#include "VertexFormatBuilder.h"			// always include after materialsystem

...

CVertexFormatBuilder vfmt;
vfmt.SetStream(0, g_EGFHwVertexFormat, "EGFVertex");
vfmt.SetStream(2, g_MatrixInstanceFormat, "Instance");

ArrayCRef<VertexFormatDesc> genFmt = vfmt.Build();

g_renderAPI->CreateVertexFormat("EGFVertex," genFmt);

*/

const int VERTEXFORMAT_MAX_ELEMS_PER_STREAM = 64;

struct VertexFormatDesc;

class CVertexFormatBuilder
{
public:
	CVertexFormatBuilder()
	{
		memset(m_streams, 0, sizeof(m_streams));
		memset(m_enabledComponents, 0xFFFFFFFF, sizeof(m_enabledComponents));
	}

	void SetStream(int streamIdx, ArrayCRef<VertexFormatDesc> desc, const char* name, bool instance = false);
	ArrayCRef<VertexFormatDesc> Build();

	void EnableComponent(int streamIdx, const char* name) { SetComponentEnabled(streamIdx, name, true); }
	void DisableComponent(int streamIdx, const char* name) { SetComponentEnabled(streamIdx, name, false); }

	void SetComponentEnabled(int streamIdx, const char* name, bool enable);

protected:
	struct Stream
	{
		const VertexFormatDesc* srcFmt{ nullptr };
		const char*		name{ nullptr };
		int				srcFmtElems{ 0 };
		bool			instance{ false };
	};

	VertexFormatDesc	m_resultFormat[MAX_VERTEXSTREAM * VERTEXFORMAT_MAX_ELEMS_PER_STREAM];
	bool				m_enabledComponents[MAX_VERTEXSTREAM * VERTEXFORMAT_MAX_ELEMS_PER_STREAM];

	Stream				m_streams[MAX_VERTEXSTREAM];
};

// simple inline code

inline void CVertexFormatBuilder::SetStream(int streamIdx, ArrayCRef<VertexFormatDesc> desc, const char* name, bool instance)
{
	Stream& stream = m_streams[streamIdx];
	stream.name = name;
	stream.srcFmt = desc.ptr();
	stream.srcFmtElems = desc.numElem();
	stream.instance = instance;
}

inline ArrayCRef<VertexFormatDesc> CVertexFormatBuilder::Build()
{
	int formatDescCount = 0;
	for (int i = 0; i < MAX_VERTEXSTREAM; ++i)
	{
		const Stream& stream = m_streams[i];
		if (!stream.srcFmt || stream.srcFmtElems == 0)
			continue;

		for (int j = 0; j < stream.srcFmtElems; ++j)
		{
			VertexFormatDesc& destDesc = m_resultFormat[formatDescCount++];
			destDesc = stream.srcFmt[j];

			// assign the stream ID
			destDesc.streamId = i;

			const int compIdx = i * MAX_VERTEXSTREAM + j;
			if (!m_enabledComponents[compIdx])
				destDesc.attribType = VERTEXATTRIB_UNUSED;
			else if (stream.instance)
				destDesc.attribType |= VERTEXATTRIB_FLAG_INSTANCE;
		}
	}

	return ArrayCRef(m_resultFormat, formatDescCount);
}

inline void CVertexFormatBuilder::SetComponentEnabled(int streamIdx, const char* name, bool enable)
{
	Stream& stream = m_streams[streamIdx];
	int descIdx = -1;
	for (int i = 0; i < stream.srcFmtElems; i++)
	{
		if (!stream.srcFmt[i].name)
			continue;
		if (!stricmp(stream.srcFmt[i].name, name))
		{
			descIdx = i;
			break;
		}
	}

	ASSERT_MSG(descIdx != -1, "CVertexFormatBuilder::EnableComponent: %s not found in %s", name, stream.name);

	const int compIdx = streamIdx * MAX_VERTEXSTREAM + descIdx;
	m_enabledComponents[compIdx] = enable;
}
