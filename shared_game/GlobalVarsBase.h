//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Global vars base
//////////////////////////////////////////////////////////////////////////////////

#ifndef GLOBALVARS_H
#define GLOBALVARS_H

#ifdef _WIN32
#pragma once
#endif

class GlobalVarsBase
{
public:
	float frametime;
	float curtime;
	float realtime;

	float timescale;

	char worldname[64];
	
	// NOTENOTE: The real GlobalVars creating in the Game dll
};

extern GlobalVarsBase* gpGlobals;

#endif //GLOBALVARS_H