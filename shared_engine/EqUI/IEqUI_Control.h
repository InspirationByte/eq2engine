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

#ifdef GetParent
#undef GetParent
#endif //GetParent

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
#define EQUI_CLASS( className, baseClassName )					\
	typedef className ThisClass;								\
	typedef baseClassName BaseClass;							\
	const char*	GetClassname() const { return ThisClass::Classname(); }		\
	static const char* Classname() { return #className; }		\

class IUIControl
{
	friend class CUIManager;

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

	virtual void				SetVisible(bool bVisible)			{bVisible ? Show() : Hide();}
	virtual bool				IsVisible() const;

	virtual void				SetSelfVisible(bool bVisible)		{m_selfVisible = bVisible;}
	virtual bool				IsSelfVisible() const				{return m_selfVisible;}

	// activation
	virtual void				Enable(bool value)					{m_enabled = value;}
	virtual bool				IsEnabled() const;

	// position and size
	void						SetSize(const IVector2D& size);
	void						SetPosition(const IVector2D& pos);

	const IVector2D&			GetSize() const;
	const IVector2D&			GetPosition() const;

	void						SetAchors(int anchor)			{ m_anchors = anchor; }
	int							GetAchors() const				{ return m_anchors; }

	// real rectangle, size position
	void						SetRectangle(const IRectangle& rect);
	virtual IRectangle			GetRectangle() const;

	// drawn rectangle
	virtual IRectangle			GetClientRectangle() const;

	// child controls
	void						AddChild(IUIControl* pControl);
	void						RemoveChild(IUIControl* pControl, bool destroy = true);
	IUIControl*					FindChild( const char* pszName );
	void						ClearChilds( bool destroy = true );

	IUIControl*					GetParent() const { return m_parent; }

	virtual IEqFont*			GetFont() const;

	// PURE VIRTUAL
	virtual const char*			GetClassname() const = 0;

protected:
	Vector2D					CalcScaling() const;
	void						ResetSizeDiffs();

	// rendering
	virtual void				Render();
	virtual void				DrawSelf(const IRectangle& rect) = 0;

	bool						ProcessCommand(DkList<EqString>& args);

	virtual IUIControl*			HitTest(const IVector2D& point);

	// events
	virtual bool				ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags);
	virtual bool				ProcessKeyboardEvents(int nKeyButtons, int flags);

	IUIControl*					m_parent;

	DkLinkedList<IUIControl*>	m_childs;		// child panels

	eqUIEventCmd_t				m_commandEvent;

	IVector2D					m_position;
	IVector2D					m_size;

	// for anchors
	IVector2D					m_sizeDiff;	
	Vector2D					m_sizeDiffPerc;

	bool						m_visible;
	bool						m_enabled;

	bool						m_selfVisible;

	int							m_alignment;
	int							m_anchors;
	int							m_scaling;

	EqString					m_name;
	EqWString					m_label;

	IEqFont*					m_font;
};

template <class T> 
T* DynamicCast(IUIControl* control)
{
	if(control == nullptr)
		return nullptr;

	if(!stricmp(T::Classname(), control->GetClassname())) 
	{
		return (T*)control;
	}
	return nullptr;
}

};

#endif // IEQUICONTROL_H