//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Generic image list renderer
//////////////////////////////////////////////////////////////////////////////////

#ifndef GENERICIMAGELISTRENDERER_H
#define GENERICIMAGELISTRENDERER_H

#include "materialsystem/IMaterialSystem.h"

class IEqFont;

template <typename T>
class CGenericImageListRenderer
{
public:
	CGenericImageListRenderer();
	virtual ~CGenericImageListRenderer();

	void				RedrawItems( const IRectangle& screenRect, float scrollPerc, float previewSize );

	virtual Rectangle_t	ItemGetImageCoordinates( T& item ) = 0;
	virtual ITexture*	ItemGetImage( T& item ) = 0;
	virtual void		ItemPostRender( int id, T& item, const IRectangle& rect ) {}

protected:
	

	DkList<T>	m_filteredList;

	IEqFont*	m_debugFont;
	int			m_selection;
	int			m_mouseOver;
	IVector2D	m_mousePos;
};


template <typename T>
inline CGenericImageListRenderer<T>::CGenericImageListRenderer() : m_debugFont(nullptr), m_selection(-1), m_mouseOver(-1), m_mousePos(0)
{
}

template <typename T>
inline CGenericImageListRenderer<T>::~CGenericImageListRenderer()
{
}

template <typename T>
inline void CGenericImageListRenderer<T>::RedrawItems( const IRectangle& rect, float scrollPerc, float previewSize )
{
	if(!m_debugFont) m_debugFont = g_fontCache->GetFont("debug", 0);

	m_mouseOver = -1;

	BlendStateParam_t blendParams;
	blendParams.alphaTest = false;
	blendParams.alphaTestRef = 1.0f;
	blendParams.blendEnable = false;
	blendParams.srcFactor = BLENDFACTOR_ONE;
	blendParams.dstFactor = BLENDFACTOR_ZERO;
	blendParams.mask = COLORMASK_ALL;
	blendParams.blendFunc = BLENDFUNC_ADD;

	IVector2D size = rect.GetSize();

	materials->Setup2D(size.x,size.y);

	int nLine = 0;
	int nItem = 0;

	IRectangle screenRect = rect;
	screenRect.Fix();

	int numItems = 0;

	for(int i = 0; i < m_filteredList.numElem(); i++)
	{
		float x_offset = 16 + nItem*(previewSize+16);

		if(x_offset + previewSize > size.x)
		{
			numItems = i;
			break;
		}

		numItems++;
		nItem++;
	}

	if(numItems > 0)
	{
		nItem = 0;

		for(int i = 0; i < m_filteredList.numElem(); i++)
		{
			T& elem = m_filteredList[i];

			if(nItem >= numItems)
			{
				nItem = 0;
				nLine++;
			}

			float x_offset = 16 + nItem*(previewSize+16);
			float y_offset = 8 + nLine*(previewSize+48);

			y_offset -= scrollPerc*(previewSize+48);

			IRectangle check_rect(x_offset, y_offset, x_offset + previewSize, y_offset + previewSize);
			check_rect.Fix();

			if( check_rect.vleftTop.y > screenRect.vrightBottom.y )
				break;

			if( !screenRect.IsIntersectsRectangle(check_rect) )
			{
				nItem++;
				continue;
			}

			float x_scale = 1.0f;
			float y_scale = 1.0f;

			ITexture* pTex = ItemGetImage(elem);
			float texture_aspect = pTex->GetWidth() / pTex->GetHeight();

			if(pTex->GetWidth() > pTex->GetHeight())
				y_scale /= texture_aspect;

			Rectangle_t name_rect(x_offset, y_offset+previewSize, x_offset + previewSize,y_offset + previewSize + 400);

			Vertex2D_t	verts[] = {MAKETEXQUAD(x_offset, y_offset, x_offset + previewSize*x_scale,y_offset + previewSize*y_scale, 0)};
			Vertex2D	rectVerts[] = {MAKETEXRECT(x_offset, y_offset, x_offset + previewSize,y_offset + previewSize, 0)};

			Rectangle_t texCoords = ItemGetImageCoordinates( elem );

			verts[0].texCoord = texCoords.GetLeftTop();
			verts[1].texCoord = texCoords.GetLeftBottom();
			verts[2].texCoord = texCoords.GetRightTop();
			verts[3].texCoord = texCoords.GetRightBottom();

			// mouseover rectangle
			if( check_rect.IsInRectangle( m_mousePos ) )
			{
				m_mouseOver = i;

				materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts,4, pTex, ColorRGBA(1,0.5f,0.5f,1), &blendParams);
				materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, rectVerts, elementsOf(rectVerts), NULL, ColorRGBA(1,0,0,1));
			}
			else
				materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts,4, pTex, color4_white, &blendParams);

			// draw selection rectangle
			if(m_selection == i)
				materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, rectVerts, elementsOf(rectVerts), NULL, ColorRGBA(0,1,0,1));

			// post render item
			ItemPostRender(i, elem, check_rect);

			nItem++;
		}
	}
}

#endif // GENERICIMAGELISTRENDERER_H