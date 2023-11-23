//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Mesh instance pool
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "materialsystem1/renderers/ShaderAPI_defs.h"

class IGPUBuffer;
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;

struct MeshInstanceBuffer
{
	IGPUBufferPtr	instBuffer;
	ushort			firstInst{ 0 };
	ushort			instCount{ 0 };
};

struct MeshInstanceRef
{
	ushort			index{ 0xffff };
};

template<typename T>
class MeshInstancePool
{
public:
	void					SetBufferFrequency(int instancesPerBuffer);
	MeshInstanceRef*		AllocInstance();

	void					GetInstanceBufferAndOffset(int instanceIdx, IGPUBufferPtr& buffer, int& offset);

private:
	Array<T>				m_instancePool{ PP_SL };
	Array<MeshInstanceRef>	m_instanceRefs{ PP_SL };
	Array<IGPUBufferPtr>	m_buffers{ PP_SL };
	int						m_instFrequency{ 256 };
};

template<typename T>
void MeshInstancePool<T>::SetBufferFrequency(int instancesPerBuffer)
{
	m_instFrequency = instancesPerBuffer;
}

template<typename T>
MeshInstanceRef* MeshInstancePool<T>::AllocInstance()
{
	return m_instancePool.append(std::move(inst));
}

template<typename T>
void MeshInstancePool<T>::GetInstanceBufferAndOffset(int instanceIdx, IGPUBufferPtr& buffer, int& offset)
{
	buffer = m_buffers[instanceIdx / m_instFrequency];
	offset = (instanceIdx % m_instFrequency) * sizeof(T);
}