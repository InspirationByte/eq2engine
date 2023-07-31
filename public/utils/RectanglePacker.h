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
	float x, y;
	float width, height;

	void* userdata;
};

//-------------------------------------------------------------------------------------------------

inline int OriginalAreaComp(PackerRectangle *const &elem0, PackerRectangle *const &elem1)
{
	return (int)(elem1->width * elem1->height - elem0->width * elem0->height);
}

inline int AreaComp(PackerRectangle *const &elem0, PackerRectangle *const &elem1)
{
	int diff = elem1->width * elem1->height - elem0->width * elem0->height;

	if (diff)
		return diff;

	diff = elem1->width - elem0->width;

	if (diff)
		return diff;

	return elem1->height - elem0->height;
}

inline int WidthComp(PackerRectangle *const &elem0, PackerRectangle *const &elem1)
{
	int diff = elem1->width - elem0->width;

	if (diff)
		return diff;

	return elem1->height - elem0->height;
}

inline int HeightComp(PackerRectangle *const &elem0, PackerRectangle *const &elem1)
{
	int diff = elem1->height - elem0->height;

	if (diff)
		return diff;

	return elem1->width - elem0->width;
}

typedef int (*COMPRECTFUNC)(PackerRectangle *const &elem0, PackerRectangle *const &elem1);

//-------------------------------------------------------------------------------------------------

class CRectanglePacker
{
public:
								CRectanglePacker();
						virtual ~CRectanglePacker();

	// adds new rectangle
	int							AddRectangle(float width, float height, void* pUserData = NULL);

	// assigns coordinates
	bool						AssignCoords(float& width, float& height, COMPRECTFUNC compRectFunc = OriginalAreaComp);

	// returns rectangle
	void						GetRectangle(AARectangle& rect, void** userData, uint index) const;
	void*						GetRectangleUserData(uint index) const				{ return m_pRectangles[index]->userdata; }
	void						SetRectangleUserData(uint index, void* userData)	{ m_pRectangles[index]->userdata = userData; }

	int							GetRectangleCount() const {return m_pRectangles.numElem();}

	void						SetPackPadding(float padding) { m_padding = padding; }

	void						Cleanup();

protected:
	Array<PackerRectangle *>	m_pRectangles{ PP_SL };
	float						m_padding;
};
