//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI panel
//////////////////////////////////////////////////////////////////////////////////

#include "IEqUI_Control.h"

#include "EqUI_Manager.h"
#include "EqUI_Panel.h"

#include "IFileSystem.h"
#include "FontCache.h"

#include "IConCommandFactory.h"

namespace equi
{

IUIControl::IUIControl()
	: m_visible(false), m_selfVisible(true), m_enabled(true), m_parent(NULL), m_font(NULL)
{

}

IUIControl::~IUIControl()
{
	ClearChilds(true);
}

void IUIControl::InitFromKeyValues( kvkeybase_t* sec )
{
	if(!stricmp(sec->GetName(), "child"))
		SetName(KV_GetValueString(sec, 1, ""));
	else
		SetName(KV_GetValueString(sec, 0, ""));

	m_label = KV_GetValueString(sec->FindKeyBase("label"), 0, "Control");
	m_position = KV_GetIVector2D(sec->FindKeyBase("position"), 0, 25);
	m_size = KV_GetIVector2D(sec->FindKeyBase("size"), 0, 128);

	m_visible = KV_GetValueBool(sec->FindKeyBase("visible"), 0, true);
	m_selfVisible = KV_GetValueBool(sec->FindKeyBase("selfvisible"), 0, true);

	kvkeybase_t* font = sec->FindKeyBase("font");

	if(font)
	{
		// TODO: styles
		m_font = g_fontCache->GetFont(KV_GetValueString(font), KV_GetValueInt(font, 1, 20), 0, false);
	}

	kvkeybase_t* command = sec->FindKeyBase("command");

	if(command)
	{
		for(int i = 0; i < command->values.numElem(); i++)
		{
			m_commandEvent.args.append( KV_GetValueString(command, i) );
		}
	}

	for(int i = 0; i < sec->keys.numElem(); i++)
	{
		kvkeybase_t* csec = sec->keys[i];
		if(!csec->IsSection())
			continue;

		if(!stricmp(csec->GetName(), "child"))
		{
			const char* controlClass = KV_GetValueString(csec, 0, "");

			IUIControl* control = equi::Manager->CreateElement( controlClass );

			if(!control)
				continue;

			control->InitFromKeyValues(csec);
			AddChild( control );
		}
	}
}

void IUIControl::SetSize(const IVector2D &size)
{
	m_size = size;
}

void IUIControl::SetPosition(const IVector2D &pos)
{
	m_position = pos;
}

void IUIControl::SetRectangle(const IRectangle& rect)
{
	m_position = rect.vleftTop;
	m_size = rect.vrightBottom - m_position;
}

bool IUIControl::IsVisible() const
{
	if(m_parent)
		return m_parent->IsVisible() && m_visible;

	return m_visible;
}

bool IUIControl::IsEnabled() const
{
	if(m_parent)
		return m_parent->IsEnabled() && m_enabled;

	return m_enabled;
}

const IVector2D& IUIControl::GetSize() const
{
	return m_size;
}

const IVector2D& IUIControl::GetPosition() const
{
	return m_position;
}

// clipping rectangle, size position
IRectangle IUIControl::GetRectangle() const
{
	return IRectangle(m_position, m_position + m_size);
}

IRectangle IUIControl::GetClientRectangle() const
{
	IRectangle thisRect(m_position, m_position + m_size);

	if(m_parent)
	{
		IRectangle parentRect = m_parent->GetClientRectangle();

		thisRect.vleftTop += parentRect.vleftTop;
		thisRect.vrightBottom += parentRect.vleftTop;
	}

	return thisRect;
}

IEqFont* IUIControl::GetFont() const
{
	if(m_font == NULL)
	{
		if( m_parent )
			return m_parent->GetFont();
		else
			return equi::Manager->GetDefaultFont();
	}

	return m_font;
}

// rendering function
void IUIControl::Render()
{
	if(!m_visible)
		return;

	RasterizerStateParams_t rasterState;
	rasterState.fillMode = FILL_SOLID;
	rasterState.cullMode = CULL_NONE;
	rasterState.scissor = true;

	IRectangle clientRectRender = GetClientRectangle();

	// force rasterizer state
	// other states are pretty useless
	materials->SetRasterizerStates(rasterState);
	materials->SetShaderParameterOverriden(SHADERPARAM_RASTERSETUP, true);

	if( m_parent && m_selfVisible )
	{
		// paint control itself
		DrawSelf( clientRectRender );
	}

	// render from last
	if(m_childs.goToLast())
	{
		do
		{
			// set scissor rect before childs are rendered
			g_pShaderAPI->SetScissorRectangle( clientRectRender );

			m_childs.getCurrent()->Render();
		}
		while(m_childs.goToPrev());
	}
}

IUIControl* IUIControl::HitTest(const IVector2D& point)
{
	if(!m_visible)
		return NULL;

	IUIControl* bestControl = this;

	IRectangle clientRect = GetClientRectangle();

	if(!clientRect.IsInRectangle(point))
		return NULL;

	if(m_childs.goToFirst())
	{
		do
		{
			IUIControl* hit = m_childs.getCurrent()->HitTest(point);

			if(hit)
			{
				bestControl = hit;
				break;
			}
		}
		while(m_childs.goToNext());
	}

	return bestControl;
}

// returns child control
IUIControl* IUIControl::FindChild(const char* pszName)
{
	DkLinkedListIterator<IUIControl*> iter(m_childs);

	if(iter.goToFirst())
	{
		do
		{
			if(!strcmp(iter.getCurrent()->GetName(), pszName))
				return iter.getCurrent();
		}
		while(iter.goToNext());
	}

	return NULL;
}

void IUIControl::ClearChilds(bool destroy)
{
	if(m_childs.goToFirst())
	{
		do
		{
			m_childs.getCurrent()->m_parent = NULL;

			if(destroy)
				delete m_childs.getCurrent();

			m_childs.setCurrent(NULL);
		}
		while(m_childs.goToNext());
	}

	m_childs.clear();
}

void IUIControl::AddChild(IUIControl* pControl)
{
	m_childs.addFirst(pControl);
	pControl->m_parent = this;
}

void IUIControl::RemoveChild(IUIControl* pControl, bool destroy)
{
	if(m_childs.goToFirst())
	{
		do
		{
			if(m_childs.getCurrent() == pControl)
			{
				pControl->m_parent = NULL;

				if(destroy)
					delete pControl;

				m_childs.removeCurrent();
				return;
			}
		}
		while(m_childs.goToNext());
	}
}

bool IUIControl::ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags)
{

	///Msg("ProcessMouseEvents on %s\n", m_name.c_str());
	return true;
}

bool IUIControl::ProcessKeyboardEvents(int nKeyButtons, int flags)
{
	//Msg("ProcessKeyboardEvents on %s\n", m_name.c_str());
	return true;
}

bool IUIControl::ProcessCommand(DkList<EqString>& args)
{
	if(UICMD_ARGC == 0)
		return true;

	if(!stricmp("hideparent", UICMD_ARGV(0).c_str()))
	{
		if(m_parent)
			m_parent->Hide();
	}

	if(!stricmp("engine", UICMD_ARGV(0).c_str()))
	{
		// execute console commands
		g_sysConsole->SetCommandBuffer(UICMD_ARGV(1).c_str());
		g_sysConsole->ExecuteCommandBuffer();
		g_sysConsole->ClearCommandBuffer();
	}

	if(!stricmp("showpanel", UICMD_ARGV(0).c_str()))
	{
		// show panel
		equi::Panel* panel = equi::Manager->FindPanel(UICMD_ARGV(1).c_str());
		panel->Show();
	}

	if(!stricmp("hidepanel", UICMD_ARGV(0).c_str()))
	{
		// hide panel
		equi::Panel* panel = equi::Manager->FindPanel(UICMD_ARGV(1).c_str());
		panel->Hide();
	}

	return true;
}

};
