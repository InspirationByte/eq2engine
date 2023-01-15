//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI control base
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "equi_defs.h"
#include "font/IFont.h"

#ifdef GetParent
#undef GetParent
#endif //GetParent

struct KVSection;
struct eqFontStyleParam_t;

#define UICMD_ARGV(index)		event.args.ptr()[index]
#define UICMD_ARGC				event.args.numElem()

// a helper macro for baseclass defintion
#define EQUI_CLASS( className, baseClassName )					\
	typedef className ThisClass;								\
	typedef baseClassName BaseClass;							\
	const char*	GetClassname() const { return ThisClass::Classname(); }		\
	static const char* Classname() { return #className; }		\

namespace equi
{

struct ui_event;
class IUIControl;
typedef int (*uiEventCallback_t)(IUIControl* control, ui_event& event, void* userData);

struct ui_event
{
	ui_event()
	{}

	ui_event(const char* pszName, uiEventCallback_t cb)
		: uid(0), name(pszName), callback(cb)
	{}

	int					uid;
	EqString			name;
	uiEventCallback_t	callback { nullptr };
	Array<EqString>		args{ PP_SL };
};

struct ui_transform
{
	float					rotation { 0.0f };
	Vector2D				translation { 0.0f };
	Vector2D				scale { 1.0f };
};

struct ui_fontprops
{
	IEqFont*				font { nullptr };
	Vector2D				fontScale { 1 };
	ColorRGBA				textColor { 1.0f };
	float					textWeight { 0.0f };
	int						textAlignment { TEXT_ALIGN_LEFT | TEXT_ALIGN_TOP };

	ColorRGBA				shadowColor { 0.0f, 0.0f, 0.0f, 0.7f };
	float					shadowOffset { 1.0f };
	float					shadowWeight { 0.01f };

	bool					monoSpace { false };
};

//-------------------------------------------------------------
// EqUI control interface
// use equi::DynamicCast to convert type
//-------------------------------------------------------------
class IUIControl
{
	friend class CUIManager;
	//friend class IEqUIEventHandler;

public:
	IUIControl();
	virtual ~IUIControl();

	virtual void				InitFromKeyValues( KVSection* sec, bool noClear = false );

	// name and type
	const char*					GetName() const						{return m_name.ToCString();}
	void						SetName(const char* pszName)		{m_name = pszName;}

	// label (UTF-8)
	const char*					GetLabel() const;
	void						SetLabel(const char* pszLabel);

	const wchar_t*				GetLabelText() const;
	void						SetLabelText(const wchar_t* pszLabel);

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

	void						SetAchors(int anchor)				{ m_anchors = anchor; }
	int							GetAchors() const					{ return m_anchors; }

	void						SetAlignment(int alignmentFlags)	{ m_alignment = alignmentFlags; }
	int							GetAlignment() const				{ return m_alignment; }

	void						SetScaling(int scalingMode)			{ m_scaling = scalingMode; }
	int							GetScaling() const					{ return m_scaling; }

	// real rectangle, size position
	void						SetRectangle(const IRectangle& rect);
	virtual IRectangle			GetRectangle() const;

	// sets new transformation. Set all zeros to reset
	void						SetTransform(const Vector2D& translate, const Vector2D& scale, float rotate);

	// drawn rectangle
	virtual IRectangle			GetClientRectangle() const;

	// for text only
	virtual IRectangle			GetClientScissorRectangle() const { return GetClientRectangle(); }

	// returns the scaling of element
	Vector2D					CalcScaling() const;

	const Vector2D&				GetSizeDiff() const;
	const Vector2D&				GetSizeDiffPerc() const;

	// child controls
	void						AddChild(IUIControl* pControl);
	void						RemoveChild(IUIControl* pControl, bool destroy = true);
	IUIControl*					FindChild( const char* pszName );
	IUIControl*					FindChildRecursive( const char* pszName );
	void						ClearChilds( bool destroy = true );

	IUIControl*					GetParent() const						{ return m_parent; }

	virtual IEqFont*			GetFont() const;

	virtual void				SetFontScale(const Vector2D& scale)		{ m_font.fontScale = scale; }
	virtual const Vector2D&		GetFontScale() const					{ return m_font.fontScale; }

	// text properties
	const ColorRGBA&			GetTextColor() const					{ return m_font.textColor; }
	void						SetTextColor(const ColorRGBA& color)	{ m_font.textColor = color; }

	void						SetTextAlignment(int alignmentFlags)	{ m_font.textAlignment = alignmentFlags; }
	int							GetTextAlignment() const				{ return m_font.textAlignment; }

	virtual	void				GetCalcFontStyle(eqFontStyleParam_t& style) const;

	// PURE VIRTUAL
	virtual const char*			GetClassname() const = 0;

	// rendering
	virtual void				Render();

	// Events
	int							AddEventHandler(const char* pszName, uiEventCallback_t cb);
	void						RemoveEventHandler(int handlerId);
	void						RemoveEventHandlers(const char* name);

	int							RaiseEvent(const char* name, void* userData);
	int							RaiseEventUid(int uid, void* userData);

protected:
	
	void						ResetSizeDiffs();
	virtual void				DrawSelf(const IRectangle& rect, bool scissorOn) = 0;

	static int					CommandCb(IUIControl* control, ui_event& event, void* userData);

	virtual IUIControl*			HitTest(const IVector2D& point);

	// events
	virtual bool				ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags);
	virtual bool				ProcessKeyboardEvents(int nKeyButtons, int flags);

	IUIControl*					m_parent{ nullptr };

	List<IUIControl*>			m_childs{ PP_SL };		// child panels

	Array<ui_event>				m_eventCallbacks{ PP_SL };

	IVector2D					m_position { 0 };
	IVector2D					m_size { 64 };

	// for anchors
	Vector2D					m_sizeDiff { 0.0f };
	Vector2D					m_sizeDiffPerc { 1.0f };

	bool						m_visible { true };
	bool						m_enabled { true };

	bool						m_selfVisible { true };

	int							m_alignment { UI_ALIGN_LEFT | UI_ALIGN_TOP };
	int							m_anchors { 0 };
	int							m_scaling { UI_SCALING_NONE };

	EqString					m_name;
	EqWString					m_label;

	ui_fontprops				m_font;
	ui_transform				m_transform;
};

template <class T> 
T* DynamicCast(IUIControl* control)
{
	if(control == nullptr)
		return nullptr;

	if(!stricmp(T::Classname(), control->GetClassname())) 
		return (T*)control;

	return nullptr;
}

};