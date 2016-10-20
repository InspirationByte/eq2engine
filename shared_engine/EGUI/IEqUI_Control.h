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
#include "utils/DkList.h"
#include "utils/DkLinkedList.h"
#include "utils/eqstring.h"
#include "utils/eqwstring.h"

#include "ppmem.h"

struct kvkeybase_t;
class IEqFont;

#define UICMD_ARGV(index)		args.ptr()[index]
#define UICMD_ARGC				args.numElem()

namespace equi
{

struct eqUIEventCmd_t
{
	DkList<EqString> args;
};

#ifdef DECLARE_CLASS
#undef DECLARE_CLASS
#endif

// a helper macro for baseclass defintion
#define EQUI_CLASS( className, baseClassName )	\
	typedef className ThisClass;				\
	typedef baseClassName BaseClass;			\
	const char*	GetClassname() const { return #className; }
	

class IUIControl
{
public:
	PPMEM_MANAGED_OBJECT();

	IUIControl();
	virtual ~IUIControl();

	virtual void				InitFromKeyValues( kvkeybase_t* sec = NULL );

	// name and type
	const char*					GetName() const						{return m_name.c_str();}
	void						SetName(const char* pszName)		{m_name = pszName;}


	// name and type
	const wchar_t*				GetLabel() const					{return m_label.c_str();}
	void						SetLabel(const wchar_t* pszLabel)	{m_label = pszLabel;}

	// visibility
	virtual void				Show()								{m_visible = true;}
	virtual void				Hide()								{m_visible = false;}

	virtual void				SetVisible(bool bVisible)			{m_visible = bVisible;}
	virtual bool				IsVisible() const;

	// activation
	virtual void				Enable(bool value)					{m_enabled = value;}
	virtual bool				IsEnabled() const;

	// position and size
	void						SetSize(const IVector2D& size);
	void						SetPosition(const IVector2D& pos);

	const IVector2D&			GetSize() const;
	const IVector2D&			GetPosition() const;

	// clipping rectangle, size position
	void						SetRectangle(const IRectangle& rect);
	virtual IRectangle			GetRectangle() const;

	virtual IRectangle			GetClientRectangle() const;

	// child controls
	void						AddChild(IUIControl* pControl);
	void						RemoveChild(IUIControl* pControl, bool destroy = true);
	IUIControl*					FindChild( const char* pszName );
	void						ClearChilds( bool destroy = true );

	IUIControl*					GetParent() const { return m_parent; }

	virtual IEqFont*			GetFont() const;

	// rendering
	virtual void				Render();

	virtual IUIControl*			HitTest(const IVector2D& point);

	// events
	virtual bool				ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags);
	virtual bool				ProcessKeyboardEvents(int nKeyButtons, int flags);

	bool						ProcessCommand(DkList<EqString>& args);

	virtual void				DrawSelf(const IRectangle& rect) = 0;

	// PURE VIRTUAL
	virtual const char*			GetClassname() const = 0;

protected:
	IUIControl*					m_parent;

	DkLinkedList<IUIControl*>	m_childs;		// child panels

	eqUIEventCmd_t				m_commandEvent;

	IVector2D					m_position;
	IVector2D					m_size;

	bool						m_visible;
	bool						m_enabled;

	EqString					m_name;
	EqWString					m_label;

	IEqFont*					m_font;
};
};

#endif // IEQUICONTROL_H