//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Rectangle packer, for atlases
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif // _MSC_VER

struct PackerRectangle
{
	float x = 0;
	float y = 0;
	float width = 0;
	float height = 0;
};

//-------------------------------------------------------------------------------------------------

inline int OriginalAreaComp(const PackerRectangle& elem0, const PackerRectangle& elem1)
{
	return (int)(elem1.width * elem1.height - elem0.width * elem0.height);
}

inline int AreaComp(const PackerRectangle& elem0, const PackerRectangle& elem1)
{
	int diff = elem1.width * elem1.height - elem0.width * elem0.height;

	if (diff)
		return diff;

	diff = elem1.width - elem0.width;

	if (diff)
		return diff;

	return elem1.height - elem0.height;
}

inline int WidthComp(const PackerRectangle& elem0, const PackerRectangle& elem1)
{
	const int diff = elem1.width - elem0.width;

	if (diff)
		return diff;

	return elem1.height - elem0.height;
}

inline int HeightComp(const PackerRectangle& elem0, const PackerRectangle& elem1)
{
	const int diff = elem1.height - elem0.height;

	if (diff)
		return diff;

	return elem1.width - elem0.width;
}

typedef int (*COMPRECTFUNC)(const PackerRectangle& elem0, const PackerRectangle& elem1);

//-------------------------------------------------------------------------------------------------

class CRectanglePacker
{
public:
	CRectanglePacker();
	virtual ~CRectanglePacker();

	// adds new rectangle
	int						AddRectangle(float width, float height, void* pUserData = NULL);

	// assigns coordinates
	bool					AssignCoords(float& width, float& height, COMPRECTFUNC compRectFunc = OriginalAreaComp);

	// returns rectangle
	void					GetRectangle(AARectangle& rect, void** userData, int index) const;
	void*					GetRectangleUserData(int index) const			{ return m_rectUserData[index]; }
	void					SetRectangleUserData(int index, void* userData)	{ m_rectUserData[index] = userData; }

	int						GetRectangleCount() const { return m_rectList.numElem(); }

	void					SetPackPadding(float padding) { m_padding = padding; }

	void					Cleanup();

protected:
	Array<PackerRectangle>	m_rectList{ PP_SL };
	Array<void*>			m_rectUserData{ PP_SL };
	float					m_padding{ 0.0f };
};
