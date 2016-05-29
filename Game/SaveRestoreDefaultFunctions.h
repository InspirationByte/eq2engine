//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Save game save/restore default functions
//////////////////////////////////////////////////////////////////////////////////

#ifndef SAVERESTORE_H
#define SAVERESTORE_H

#include "DebugInterface.h"
#include "VirtualStream.h"

// INTEGER

int Save_Int(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Write(pObjectPtr, 1, sizeof(int));
}

int Restore_Int(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Read(pObjectPtr, 1, sizeof(int));
}

// FLOAT

int Save_Float(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Write(pObjectPtr, 1, sizeof(float));
}

int Restore_Float(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Read(pObjectPtr, 1, sizeof(float));
}

// BOOL

int Save_Bool(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Write(pObjectPtr, 1, sizeof(bool));
}

int Restore_Bool(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Read(pObjectPtr, 1, sizeof(bool));
}

// SHORT

int Save_Short(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Write(pObjectPtr, 1, sizeof(short));
}

int Restore_Short(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Read(pObjectPtr, 1, sizeof(short));
}


// BYTE

int Save_Byte(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Write(pObjectPtr, 1, sizeof(ubyte));
}

int Restore_Byte(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Read(pObjectPtr, 1, sizeof(ubyte));
}

// STRING

int Save_String(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	EqString* strPtr = (EqString*)pObjectPtr;

	int len = 0;

	int strLen = strPtr->Length()+1; // save with null

	len += pStream->Write(&strLen, 1, sizeof(int));
	len += pStream->Write((char*)strPtr->GetData(), 1, strLen);

	return len;
}

int Restore_String(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	EqString* strPtr = (EqString*)pObjectPtr;

	int len = 0;

	int strLen = 0;
	len += pStream->Read(&strLen, 1, sizeof(int));

	char *pszText = (char*)stackalloc(strLen);

	len += pStream->Read(pszText, 1, strLen);

	*strPtr = pszText;

	return len;
}

// Vector2D

int Save_Vector2D(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Write(pObjectPtr, 1, sizeof(Vector2D));
}

int Restore_Vector2D(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Read(pObjectPtr, 1, sizeof(Vector2D));
}

// Vector3D

int Save_Vector3D(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Write(pObjectPtr, 1, sizeof(Vector3D));
}

int Restore_Vector3D(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Read(pObjectPtr, 1, sizeof(Vector3D));
}

// Vector4D

int Save_Vector4D(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Write(pObjectPtr, 1, sizeof(Vector4D));
}

int Restore_Vector4D(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Read(pObjectPtr, 1, sizeof(Vector4D));
}

// Matrix2x2

int Save_Matrix2x2(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Write(pObjectPtr, 1, sizeof(Matrix2x2));
}

int Restore_Matrix2x2(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Read(pObjectPtr, 1, sizeof(Matrix2x2));
}

// Matrix3x3

int Save_Matrix3x3(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Write(pObjectPtr, 1, sizeof(Matrix3x3));
}

int Restore_Matrix3x3(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Read(pObjectPtr, 1, sizeof(Matrix3x3));
}

// Matrix4x4

int Save_Matrix4x4(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Write(pObjectPtr, 1, sizeof(Matrix4x4));
}

int Restore_Matrix4x4(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return pStream->Read(pObjectPtr, 1, sizeof(Matrix4x4));
}

// Entity Ptr to index

int Save_EntityPtr(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	int ent_index = -1;

	BaseEntity** pPtr = (BaseEntity**)pObjectPtr;
	if(*pPtr)
	{
		ent_index = (*pPtr)->EntIndex();
	}

	return pStream->Write(&ent_index, 1, sizeof(int));
}

int Restore_EntityPtr(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	int ent_index = -1;

	int nBytes = pStream->Read(&ent_index, 1, sizeof(int));

	// set entity index temporary for further restoration
	int* eptr = (int*)pObjectPtr;
	*eptr = ent_index;

	return nBytes;
}

// EMPTY

int Save_Empty(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return 0;
}

int Restore_Empty(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	return 0;
}

#endif // SAVERESTORE_H