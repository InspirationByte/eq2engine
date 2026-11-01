//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq UI manager
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "equi_defs.h"
#include "IEqUI_Control.h"

class IEqFont;
class IMaterial;
class IGPURenderPassRecorder;
class ITexture;

namespace equi
{
class Panel;

using ControlFactoryFn = IUIControl* (*)();

class CUIManager
{
public:
	~CUIManager();
	CUIManager() = default;

	void				Init();
	void				Shutdown();

	Panel*				GetRootPanel() const;

	// the element loader
	void				RegisterFactory(const char* name, ControlFactoryFn factory);

	template<typename T>
	T*					Create();

	IUIControl*			CreateElement( const char* pszTypeName ) const;

	void				AddPanel( Panel* panel);
	void				DestroyPanel( Panel* pPanel );
	Panel*				FindPanel( const char* pszPanelName ) const;

	void				BringToTop( Panel* panel );
	Panel*				GetTopPanel() const;

	void				SetViewFrame(const IAARectangle& rect);
	const IAARectangle&	GetViewFrame() const;

	IVector2D			GetScreenSize() const;

	void				SetFocus( IUIControl* focusTo );
	IUIControl*			GetFocus() const;
	IUIControl*			GetMouseOver() const;

	bool				IsWindowsVisible() const;

	// renders to the backbuffer when target is not set
	void				Render(ITexture* colorTarget);

	bool				ProcessMouseEvents(float x, float y, int nMouseButtons, int flags);
	bool				ProcessKeyboardEvents(int nKeyButtons, int flags);

	void				DumpPanelsToConsole() const;

	IEqFont*			GetDefaultFont() const {return m_defaultFont;}

private:

	struct Factory
	{
		const char*			name;
		ControlFactoryFn	factory;
	};

	Panel*				GetPanelByElement(IUIControl* control) const;

	Array<Factory>		m_controlFactory{ PP_SL };
	IEqFont*			m_defaultFont{ nullptr };

	Panel*				m_rootPanel{ nullptr };
	Array<Panel*>		m_panels{ PP_SL };

	IUIControl*			m_keyboardFocus{ nullptr };
	IUIControl*			m_mouseOver{ nullptr };

	IAARectangle		m_viewFrameRect{ 0, 0, 640, 480 };
	IVector2D			m_mousePos{ 0 };
};

template<typename T>
inline T* CUIManager::Create()
{
	IUIControl* control = CreateElement(T::Classname());
	ASSERT_MSG(control, "Failed to create %s, check if that was registered using EQUI_REGISTER_CONTROL", T::Classname());
	return control->As<T>();
}

extern CStaticAutoPtr<CUIManager> Manager;
};

#define EQUI_FACTORY(name) s_equi_##name##_f

#define EQUI_REGISTER_CONTROL(name)				\
	extern equi::IUIControl* EQUI_FACTORY(name)();	\
	equi::Manager->RegisterFactory(#name, EQUI_FACTORY(name))

#ifdef _MSC_VER

#	define EQUI_CONTROL_DECLARATION(name)

#	define DECLARE_EQUI_CONTROL(name, classname) \
		equi::IUIControl* s_equi_##name##_f() {return PPNew equi::classname();}

#	define EQUI_REGISTER_CONTROL2(name) EQUI_REGISTER_CONTROL(name)

#else	// let's not get doomed by GCC thingy
#	define EQUI_CONTROL_DECLARATION(name) \
		namespace equi {\
			equi::IUIControl* EQUI_FACTORY(name)(); \
		}

#	define DECLARE_EQUI_CONTROL(name, classname) \
		namespace equi{\
			equi::IUIControl* EQUI_FACTORY(name)() {return PPNew equi::classname();} \
		}

#	define EQUI_REGISTER_CONTROL2(name) \
		equi::Manager->RegisterFactory(#name, equi::EQUI_FACTORY(name))

#endif // _MSC_VER
