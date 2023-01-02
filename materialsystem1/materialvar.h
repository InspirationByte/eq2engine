//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Material Variable
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "materialsystem1/IMaterialVar.h"

class CMatVar
{
	friend class CMaterial;

public:
					CMatVar() = default;
					~CMatVar();

	void			Init(const char* pszName,const char* pszValue);

	const char*		GetName() const;

	void			SetString(const char* szValue);
private:
	EqString		m_name;
	int				m_nameHash{ 0 };

	MatVarData		m_data;
};