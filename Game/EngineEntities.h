//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Camera for Engine
//////////////////////////////////////////////////////////////////////////////////

#ifndef ENGINEENTITIES_H
#define ENGINEENTITIES_H

class CWorldInfo : public BaseEntity
{
	// always do this type conversion
	typedef BaseEntity BaseClass;

public:
	CWorldInfo();

	ColorRGB	GetAmbient();
	float		GetLevelRenderableDist();

	const char*	GetMenuName();

	void		Activate();
private:
	ColorRGB	m_AmbientColor;
	float		m_fZFar;
	EqString	m_szMenuName;

	DECLARE_DATAMAP();
};

#endif // ENGINEENTITIES_H