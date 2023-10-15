//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Shader override functions
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"

const char* OverrideShader_Base()
{
	switch(g_matSystem->GetConfiguration().lightingModel)
	{
		case MATERIAL_LIGHT_UNLIT:
			return "BaseUnlit";
		case MATERIAL_LIGHT_FORWARD:
			return "BaseSingle";
		case MATERIAL_LIGHT_DEFERRED:
			return "BaseSingle";
	}

	return "BaseUnlit";
}

const char* OverrideShader_BaseSkinned()
{
	switch(g_matSystem->GetConfiguration().lightingModel)
	{
		case MATERIAL_LIGHT_UNLIT:
			return "BaseSkinned";
		case MATERIAL_LIGHT_FORWARD:
			return "BaseSkinned";
		case MATERIAL_LIGHT_DEFERRED:
			return "BaseSkinned";
	}

	return "BaseUnlit";
}

const char* OverrideShader_Error()
{
	switch(g_matSystem->GetConfiguration().lightingModel)
	{
		case MATERIAL_LIGHT_UNLIT:
			return "Error";
		case MATERIAL_LIGHT_FORWARD:
			return "BaseUnlit";
		case MATERIAL_LIGHT_DEFERRED:
			return "BaseSingle";
	}

	return "Error";
}


const char* OverrideShader_Sky()
{
	return "Skybox";
}

void InitShaderOverrides()
{
	g_matSystem->RegisterShaderOverrideFunction("Base", OverrideShader_Base);
	g_matSystem->RegisterShaderOverrideFunction("BaseSkinned", OverrideShader_BaseSkinned);
	g_matSystem->RegisterShaderOverrideFunction("Error", OverrideShader_Error);
	g_matSystem->RegisterShaderOverrideFunction("Sky", OverrideShader_Sky);
}