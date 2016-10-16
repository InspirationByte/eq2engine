//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI control base
//////////////////////////////////////////////////////////////////////////////////

#ifndef IEQUICONTROL_H
#define IEQUICONTROL_H

#include "math/DkMath.h"
#include "math/Rectangle.h"
#include "utils/DkLinkedList.h"
#include "utils/eqstring.h"

class IEqUIControl
{
public:

	IEqUIControl();
	virtual ~IEqUIControl();

	virtual int				GetType() const = 0;

	// name and type
	const char*				GetName() const {return m_name.c_str();}
	void					SetName(const char* pszName) {m_name = pszName;}

	// visibility
	virtual void			Show() {m_visible = true;}
	virtual void			Hide() {m_visible = false;}

	virtual void			SetVisible(bool bVisible) {m_visible = bVisible;}
	virtual bool			IsVisible() const;

	// activation
	virtual void			Enable(bool value) {m_enabled = value;}
	virtual bool			IsEnabled() const;

	// position and size
	void					SetSize(const IVector2D& size);
	void					SetPosition(const IVector2D& pos);

	const IVector2D&		GetSize() const;
	const IVector2D&		GetPosition() const;

	// clipping rectangle, size position
	void					SetRectangle(const IRectangle& rect);
	virtual IRectangle		GetRectangle() const;

	// child controls
	void					AddChild(IEqUIControl* pControl);
	void					RemoveChild(IEqUIControl* pControl, bool destroy = true);
	IEqUIControl*			FindChild( const char* pszName );
	void					ClearChilds( bool destroy = true );

	IEqUIControl*			GetParent() const { return m_parent; }

	// events
	virtual bool			ProcessMouseEvents(float x, float y, int nMouseButtons, int flags) = 0;
	virtual bool			ProcessKeyboardEvents(int nKeyButtons, int flags) = 0;

	// rendering
	virtual void			Render() = 0;

protected:
	IEqUIControl*			m_parent;

	DkLinkedList<IEqUIControl*>	m_childs;		// child panels

	IVector2D				m_position;
	IVector2D				m_size;

	bool					m_visible;
	bool					m_enabled;
	EqString				m_name;
};

#endif // IEQUICONTROL_H