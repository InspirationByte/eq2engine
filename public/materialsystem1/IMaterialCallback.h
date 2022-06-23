//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Material system render parameters
//////////////////////////////////////////////////////////////////////////////////

#pragma once
class IMaterial;


class IMaterialRenderParamCallbacks
{
public:
	~IMaterialRenderParamCallbacks() {}

	// called before material gets bound. Return pMaterial if you don't want to override it, return different material if you want to override.
	virtual IMaterial* OnPreBindMaterial(IMaterial* pMaterial) = 0;

	// for direct control of states in application
	virtual void		OnPreApplyMaterial(IMaterial* pMaterial) = 0;

	// parameters used by shaders, in user shaders
	virtual void*		GetRenderUserParams() { return nullptr; }
	virtual int			GetRenderUserParamsTypeSignature() { return 0; }
};