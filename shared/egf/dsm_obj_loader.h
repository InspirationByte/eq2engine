//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Shared Model loader, obj support
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IFileStream;
using IFileStreamPtr = CRefPtr<IFileStream>;

namespace SharedModel
{
struct DSModel;

// Loads OBJ model, as DSM
bool LoadOBJ(DSModel& model, const char* filename, int searchPatch = -1);
bool SaveOBJ(const DSModel& model, const char* filename, int searchPatch = -1);

bool LoadOBJ(DSModel& model, IFileStreamPtr pFile);
bool SaveOBJ(const DSModel& model, IFileStreamPtr pFile);

} // namespace
