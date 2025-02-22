//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Shared Model loader, obj support
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IVirtualStream;
using IVirtualStreamPtr = CRefPtr<IVirtualStream>;

namespace SharedModel
{
struct DSModel;

// Loads OBJ model, as DSM
bool LoadOBJ(DSModel& model, const char* filename, int searchPatch = -1);
bool SaveOBJ(const DSModel& model, const char* filename, int searchPatch = -1);

bool LoadOBJ(DSModel& model, IVirtualStreamPtr pFile);
bool SaveOBJ(const DSModel& model, IVirtualStreamPtr pFile);

} // namespace
