//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: a mesh builder designed for dynamic meshes (material system)
//////////////////////////////////////////////////////////////////////////////////

#ifndef VERTEXFORMATUTIL_H
#define VERTEXFORMATUTIL_H

/*

USAGE:

#include "materialsystem/IMaterialSystem.h"
#include "VertexFormatBuilder.h"			// always include after materialsystem

...

CVertexFormatBuilder vfmt;
vfmt.SetStream(0, g_EGFHwVertexFormat, elementsOf(g_EGFHwVertexFormat), "EGFVertex");
vfmt.SetStream(2, g_MatrixInstanceFormat, elementsOf(g_MatrixInstanceFormat), "Instance");

VertexFormatDesc_t* genFmt = nullptr;
int genFmtElems = vfmt.Build(&genFmt);

g_pShaderAPI->CreateVertexFormat(genFmt, genFmtElems);

*/

const int VERTEXFORMAT_MAX_ELEMS_PER_STREAM = 64;

class CVertexFormatBuilder
{
public:
	CVertexFormatBuilder()
	{
		memset(m_streams, 0, sizeof(m_streams));
		memset(m_enabledComponents, 0xFFFFFFFF, sizeof(m_enabledComponents));
	}

	void SetStream(int streamIdx, VertexFormatDesc_t* fmtDesc, int elems, const char* debugName = nullptr);
	int Build(VertexFormatDesc_t** outPointer);

	void EnableComponent(int streamIdx, const char* name) { SetComponentEnabled(streamIdx, name, true); }
	void DisableComponent(int streamIdx, const char* name) { SetComponentEnabled(streamIdx, name, false); }

	void SetComponentEnabled(int streamIdx, const char* name, bool enable);

protected:
	struct stream_t
	{
		VertexFormatDesc_t* srcFmt;
		int srcFmtElems;
		const char* debugName;
	};

	VertexFormatDesc_t	m_resultFormat[MAX_VERTEXSTREAM * VERTEXFORMAT_MAX_ELEMS_PER_STREAM];
	bool				m_enabledComponents[MAX_VERTEXSTREAM * VERTEXFORMAT_MAX_ELEMS_PER_STREAM];

	stream_t			m_streams[MAX_VERTEXSTREAM];
};

// simple inline code

inline void CVertexFormatBuilder::SetStream(int streamIdx, VertexFormatDesc_t* fmtDesc, int elems, const char* debugName)
{
	stream_t& stream = m_streams[streamIdx];
	stream.debugName = debugName;
	stream.srcFmt = fmtDesc;
	stream.srcFmtElems = elems;
}

inline int CVertexFormatBuilder::Build(VertexFormatDesc_t** outPointer)
{
	ASSERT(outPointer);

	int formatDescCount = 0;
	for (int i = 0; i < MAX_VERTEXSTREAM; i++)
	{
		const stream_t& stream = m_streams[i];
		if (!stream.srcFmt || stream.srcFmtElems == 0)
			continue;

		for (int j = 0; j < stream.srcFmtElems; j++)
		{
			VertexFormatDesc_t& srcDesc = stream.srcFmt[j];
			VertexFormatDesc_t& destDesc = m_resultFormat[formatDescCount++];
			memcpy(&destDesc, &srcDesc, sizeof(VertexFormatDesc_t));
			
			// assign the stream ID
			destDesc.streamId = i;

			int compIdx = i * MAX_VERTEXSTREAM + j;

			// disable attribute
			if (!m_enabledComponents[compIdx])
				destDesc.attribType = VERTEXATTRIB_UNUSED;
		}
	}

	*outPointer = m_resultFormat;

	return formatDescCount;
}

inline void CVertexFormatBuilder::SetComponentEnabled(int streamIdx, const char* name, bool enable)
{
	stream_t& stream = m_streams[streamIdx];
	int descIdx = -1;

	for (int i = 0; i < stream.srcFmtElems; i++)
	{
		if (!stricmp(stream.srcFmt[i].name, name))
		{
			descIdx = i;
			break;
		}
	}

	ASSERT_MSG(descIdx != -1, "EnableComponent: not found");

	int compIdx = streamIdx * MAX_VERTEXSTREAM + descIdx;
	m_enabledComponents[compIdx] = enable;
}

#endif // VERTEXFORMATUTIL_H