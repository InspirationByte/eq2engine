//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq UI manager
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQUI_MANAGER_H
#define EQUI_MANAGER_H

#include "equi_defs.h"

#include "materialsystem/IMaterialSystem.h"
#include "math/Rectangle.h"
#include "utils/DkList.h"

class IEqFont;

namespace equi
{

class IUIControl;
class Panel;

typedef IUIControl* (*EQUICONTROLFACTORYFN)();

struct ctrlFactory_t
{
	const char* name;
	EQUICONTROLFACTORYFN factory;
};

typedef bool (*EQUICOMMANDPROCESSOR)(DkList<EqString>& args);

struct commandProcFn_t
{
	const char* name;
	EQUICOMMANDPROCESSOR func;
};

class CUIManager
{
public:
						CUIManager();
						~CUIManager();

	void				Init();
	void				Shutdown();

	equi::Panel*		GetRootPanel() const;

	// the element loader
	void				RegisterFactory(const char* name, EQUICONTROLFACTORYFN factory);

	IUIControl*			CreateElement( const char* pszTypeName );

	void				AddPanel( equi::Panel* panel);
	void				DestroyPanel( equi::Panel* pPanel );
	equi::Panel*		FindPanel( const char* pszPanelName ) const;

	void				BringToTop( equi::Panel* panel );
	equi::Panel*		GetTopPanel() const;

	void				SetViewFrame(const IRectangle& rect);
	const IRectangle&	GetViewFrame() const;

	IVector2D			GetScreenSize() const;

	void				SetFocus( IUIControl* focusTo );
	IUIControl*			GetFocus() const;
	IUIControl*			GetMouseOver() const;

	bool				IsWindowsVisible() const;

	void				Render();

	bool				ProcessMouseEvents(float x, float y, int nMouseButtons, int flags);
	bool				ProcessKeyboardEvents(int nKeyButtons, int flags);

	void				DumpPanelsToConsole();

	IEqFont*			GetDefaultFont() const {return m_defaultFont;}

private:

	equi::Panel*		GetPanelByElement(IUIControl* control);

	equi::Panel*			m_rootPanel;

	IUIControl*				m_keyboardFocus;
	IUIControl*				m_mouseOver;

	IVector2D				m_mousePos;

	DkList<equi::Panel*>	m_panels;

	IRectangle				m_viewFrameRect;
	IMaterial*				m_material;

	IEqFont*				m_defaultFont;

	DkList<ctrlFactory_t>	m_controlFactory;
};

extern CUIManager* Manager;
};

#define EQUI_FACTORY(name) \
	s_equi_##name##_f

#define EQUI_REGISTER_CONTROL(name)				\
	extern equi::IUIControl* EQUI_FACTORY(name)();	\
	equi::Manager->RegisterFactory(#name, EQUI_FACTORY(name))

#ifdef _MSC_VER

#	define EQUI_CONTROL_DECLARATION(name)

#	define DECLARE_EQUI_CONTROL(name, classname) \
		equi::IUIControl* s_equi_##name##_f() {return new equi::classname();}

#	define EQUI_REGISTER_CONTROL2(name) EQUI_REGISTER_CONTROL(name)

#else	// let's not get doomed by GCC thingy
#	define EQUI_CONTROL_DECLARATION(name) \
		namespace equi {\
			equi::IUIControl* s_equi_##name##_f(); \
		}

#	define DECLARE_EQUI_CONTROL(name, classname) \
		namespace equi{\
			equi::IUIControl* s_equi_##name##_f() {return new equi::classname();} \
		}

#	define EQUI_REGISTER_CONTROL2(name) \
		equi::Manager->RegisterFactory(#name, equi::EQUI_FACTORY(name))

#endif // _MSC_VER

#endif // EQUI_MANAGER_H
