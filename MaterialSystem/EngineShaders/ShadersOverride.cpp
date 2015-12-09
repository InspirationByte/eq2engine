//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Shader override functions
//////////////////////////////////////////////////////////////////////////////////

#include "materialsystem/IMaterialSystem.h"

const char* OverrideShader_Base()
{
	switch(materials->GetLightingModel())
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
	switch(materials->GetLightingModel())
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

const char* OverrideShader_BaseParticle()
{
	/*
	switch(materials->GetLightingModel())
	{
		case MATERIAL_LIGHT_DEFERRED:
			return "BaseParticleDS";
	}
	*/
	return "BaseParticle";
}

const char* OverrideShader_Error()
{
	switch(materials->GetLightingModel())
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

void InitShaderOverrides()
{
	materials->RegisterShaderOverrideFunction("Base", OverrideShader_Base);
	materials->RegisterShaderOverrideFunction("BaseSkinned", OverrideShader_BaseSkinned);
	materials->RegisterShaderOverrideFunction("BaseParticle", OverrideShader_BaseParticle);
	materials->RegisterShaderOverrideFunction("Error", OverrideShader_Error);
}