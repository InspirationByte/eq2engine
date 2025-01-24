//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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

class IGPURenderPassRecorder;
struct KVSection;
struct FontStyleParam;

#define UICMD_ARGV(index)		event.args[index]
#define UICMD_ARGC				event.args.numElem()

// a helper macro for baseclass defintion
#define EQUI_CLASS( className, baseClassName )					\
	using ThisClass = className;								\
	using BaseClass = baseClassName;							\
	virtual const char*	GetClassname() const override { return ThisClass::Classname(); }		\
	static const char* Classname() { return #className; }		\

namespace equi
{
static constexpr int MAX_CONTROL_DEPTH = 48;

struct EvtHandler;
class IUIControl;
using EvtCallback = EqFunction<int(IUIControl*, const EvtHandler&, void*)>;

struct EvtHandler
{
	EvtCallback		callback;
	Array<EqString>	args{ PP_SL };
	EqString		name;
	int				uid{ 0 };
};

// TODO: rewrite to make it more compact
using TransformStack = FixedArray<Matrix4x4, MAX_CONTROL_DEPTH>;

struct RenderContextAbstract
{
	RenderContextAbstract(IGPURenderPassRecorder* recorder, TransformStack& transformStack)
		: rendPassRecorder(recorder)
		, transformStack(transformStack)
	{
	}
	
	TransformStack&			transformStack;
	IGPURenderPassRecorder* rendPassRecorder{ nullptr };
};

struct ControlRenderContext : public RenderContextAbstract
{
	ControlRenderContext(IGPURenderPassRecorder* recorder)
		: RenderContextAbstract(recorder, transformStackStorage)
	{
		transformStack.append(identity4);
	}

	TransformStack transformStackStorage;
};

//-------------------------------------------------------------
// EqUI control interface
// use equi::DynamicCast to convert type
//-------------------------------------------------------------
class IUIControl
{
	friend class CUIManager;
public:
	IUIControl();
	virtual ~IUIControl();

	virtual void				InitFromKeyValues(const KVSection* sec, bool keepElements = false);

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

	virtual void				SetClipsChilds(bool bEnable)		{ m_clipChilds = bEnable; }
	virtual bool				IsClipsChilds() const				{return m_clipChilds;}

	virtual void				SetClipTransform(bool bEnable)		{ m_clipTransform = bEnable; }
	virtual bool				HasClipTransform() const			{return m_clipTransform;}

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
	void						SetRectangle(const IAARectangle& rect);
	virtual IAARectangle		GetRectangle() const;

	// sets new transformation. Set all zeros to reset
	void						SetTransform(const Vector2D& translate, const Vector2D& scale, float rotate);

	// drawn rectangle
	virtual IAARectangle		GetClientRectangle() const;

	// returns the scaling of element
	Vector2D					CalcScaling() const;

	const Vector2D&				GetSizeDiff() const;
	const Vector2D&				GetSizeDiffPerc() const;

	// child controls
	void						AddChild(IUIControl* pControl);
	void						RemoveChild(IUIControl* pControl, bool destroy = true);
	IUIControl*					FindChild( const char* pszName );
	IUIControl*					FindChildRecursive( const char* pszName );
	IUIControl*					Get( const char* pathToElem);
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

	virtual	void				GetCalcFontStyle(FontStyleParam& style) const;

	// PURE VIRTUAL
	virtual const char*			GetClassname() const = 0;

	// rendering
	virtual void				Render(int depth, RenderContextAbstract& context);

	// Events
	int							AddEventHandler(const char* pszName, EvtCallback&& cb);
	void						RemoveEventHandler(int handlerId);
	void						RemoveEventHandlers(const char* name);

	int							RaiseEvent(const char* name, void* userData);
	int							RaiseEventUid(int uid, void* userData);

	virtual IAARectangle		GetClientScissorRectangle(int depth, const RenderContextAbstract& context) const;
	static IAARectangle			TransformScissorRectangle(const IAARectangle& rect, const Matrix4x4& transform);
	static IAARectangle			ClipScissorRectangle(const IAARectangle& rect, const IAARectangle& parentRect);

protected:
	struct Transform
	{
		float		rotation{ 0.0f };
		Vector2D	translation{ 0.0f };
		Vector2D	scale{ 1.0f };
	};

	struct FontProps
	{
		IEqFont*	font{ nullptr };
		Vector2D	fontScale{ 1 };
		ColorRGBA	textColor{ 1.0f };
		float		textWeight{ 0.0f };
		int			textAlignment{ TEXT_ALIGN_LEFT | TEXT_ALIGN_TOP };

		ColorRGBA	shadowColor{ 0.0f, 0.0f, 0.0f, 0.7f };
		float		shadowOffset{ 1.0f };
		float		shadowWeight{ 0.01f };

		bool		monoSpace{ false };
	};

	using FontCollection = Map<uint, FontProps>;

	const FontProps*			FindFont(const char* name, const char* requestedBy) const;

	// rendering
	virtual void				RenderChilds(int depth, RenderContextAbstract& context);

	void						InitFonts(const KVSection* sec);
	void						InitChildItems(const KVSection* sec, bool keepElements = false);

	void						ResetSizeDiffs();
	virtual void				DrawSelf(const IAARectangle& rect, IGPURenderPassRecorder* rendPassRecorder) = 0;

	static int					CommandCb(IUIControl* control, const EvtHandler& event, void* userData);

	virtual IUIControl*			HitTest(const IVector2D& point) const;

	// events
	virtual bool				ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags) { return true; }
	virtual bool				ProcessKeyboardEvents(int nKeyButtons, int flags) { return true; }

	IUIControl*					m_parent{ nullptr };
	List<IUIControl*>			m_childs{ PP_SL };		// child panels
	Array<EvtHandler>			m_eventCallbacks{ PP_SL };
	FontCollection				m_fontCollection{ PP_SL };
	FontProps					m_font;

	Transform					m_transform;

	IVector2D					m_position{ 0 };
	IVector2D					m_size{ 64 };
	IVector2D					m_sizeReal{ 64 };
	// for anchors
	Vector2D					m_sizeDiff{ 0.0f };
	Vector2D					m_sizeDiffPerc{ 1.0f };

	EqString					m_name;
	EqWString					m_label;

	int							m_alignment { UI_ALIGN_LEFT | UI_ALIGN_TOP };
	int							m_anchors { 0 };
	int							m_scaling { UI_SCALING_NONE };

	bool						m_visible{ true };
	bool						m_selfVisible{ true };
	bool						m_enabled{ true };
	bool						m_clipChilds{ true };
	bool						m_clipTransform{ true };
};

template <class T> 
T* DynamicCast(IUIControl* control)
{
	if(control == nullptr)
		return nullptr;

	if(!CString::Compare(T::Classname(), control->GetClassname())) 
		return (T*)control;

	return nullptr;
}

};