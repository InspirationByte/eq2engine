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

	void				SetFocus( IUIControl* focusTo );
	IUIControl*			GetFocus() const;
	IUIControl*			GetMouseOver() const;

	bool				IsPanelsVisible() const;

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

#ifdef _MSC_VER
#define DECLARE_EQUI_CONTROL(name, classname) \
		equi::IUIControl* s_equi_##name##_f() {return new equi::classname();}
#else
#define DECLARE_EQUI_CONTROL(name, classname) \
	namespace equi{\
		equi::IUIControl* s_equi_##name##_f() {return new equi::classname();} \
		}
#endif // _MSC_VER

#define EQUI_FACTORY(name) \
	s_equi_##name##_f

#define EQUI_REGISTER_CONTROL(name)				\
	extern equi::IUIControl* EQUI_FACTORY(name)();	\
	equi::Manager->RegisterFactory(#name, EQUI_FACTORY(name))

#endif // EQUI_MANAGER_H
