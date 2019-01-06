//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Build Options dialog
//////////////////////////////////////////////////////////////////////////////////

#include "BuildOptionsDialog.h"

#define ACTION_ID_CANCEL	2300
#define ACTION_ID_SAVE		2301
#define ACTION_ID_BUILD		2302


BEGIN_EVENT_TABLE(CBuildOptionsDialog, wxDialog)
	EVT_BUTTON(-1, CBuildOptionsDialog::OnButtonClick)
END_EVENT_TABLE()

CBuildOptionsDialog::CBuildOptionsDialog() : wxDialog(g_editormainframe, -1, "Build options", wxPoint(-1,-1) ,wxSize(530,335), wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX)
{
	Iconize( false );

	wxStaticBox *pMainBox = new wxStaticBox(this, -1, "Main", wxPoint(5,5), wxSize(256,128));
	wxStaticBox *pSubdivBox = new wxStaticBox(this, -1, "Subdivision", wxPoint(265,5), wxSize(256,128));
	wxStaticBox *pLightmapBox = new wxStaticBox(this, -1, "Lightmaps", wxPoint(5,135), wxSize(256,128));
	wxStaticBox *pDetailsBox = new wxStaticBox(this, -1, "Details", wxPoint(265,135), wxSize(256,128));

	new wxButton(this, ACTION_ID_CANCEL, DKLOC("TOKEN_CANCEL", L"Cancel"), wxPoint(5, 280), wxSize(75, 25));
	new wxButton(this, ACTION_ID_SAVE, DKLOC("TOKEN_SAVEBUILDOPT", L"Save"), wxPoint(365, 280), wxSize(75, 25));
	new wxButton(this, ACTION_ID_BUILD, DKLOC("TOKEN_BUILD", L"Build"), wxPoint(445, 280), wxSize(75, 25));

	m_pDisableTreeCheck	= new wxCheckBox(pMainBox, -1, DKLOC("TOKEN_DISABLETREE", L"Disable tree"), wxPoint(10,25));
	m_pOnlyEntsCheck	= new wxCheckBox(pMainBox, -1, DKLOC("TOKEN_ONLYENTS", L"Only entities"), wxPoint(10,55));

	wxString sectorDefaultSizes[] = 
	{
		"8192",
		"4096",
		"2048",
		"1024",
		"512",
	};

	new wxStaticText(pSubdivBox,-1, DKLOC("TOKEN_SECTORSIZE", L"Sector size (Units)"), wxPoint(10,25));
	m_pSectorSizeSpin	= new wxComboBox(pSubdivBox, -1, sectorDefaultSizes[3], wxPoint(165,25), wxSize(85,20),5,sectorDefaultSizes);

	new wxStaticText(pSubdivBox,-1, DKLOC("TOKEN_MAXTREEDEPTH", L"Max. tree depth"), wxPoint(10,55));
	m_pTreeMaxDepthSpin = new wxSpinCtrl(pSubdivBox, -1, "8", wxPoint(165,55), wxSize(85,20));

	m_pPhysicsOnlyCompile = new wxCheckBox(pSubdivBox, -1, DKLOC("TOKEN_ONLYPHYSICS", L"Physics only"), wxPoint(10,80));

	m_pBrushCSGCheck	= new wxCheckBox(pSubdivBox, -1, DKLOC("TOKEN_BRUSHCSG", L"Brush CSG"), wxPoint(10,105));
	m_pBrushCSGCheck->SetValue(true);

	m_pCompileLightmaps = new wxCheckBox(pLightmapBox, -1, DKLOC("TOKEN_COMPILELIGHTMAPS", L"Compile lightmaps"), wxPoint(10,25));
	new wxStaticText(pLightmapBox,-1, DKLOC("TOKEN_LIGHTMAPSIZE", L"Lightmap size"), wxPoint(10,55));
	m_pLightmapSize = new wxComboBox(pLightmapBox, -1, sectorDefaultSizes[3], wxPoint(165,55), wxSize(85,20),5,sectorDefaultSizes);
	new wxStaticText(pLightmapBox,-1, DKLOC("TOKEN_LUXELSPERMETER", L"Luxels per meter"), wxPoint(10,80));
	m_pLuxelsPerMeter = new wxTextCtrl(pLightmapBox, -1, "10.0",wxPoint(165,80),wxSize(85,20));
}

void CBuildOptionsDialog::OnButtonClick(wxCommandEvent& event)
{
	if(event.GetId() == ACTION_ID_CANCEL || event.GetId() == wxID_CANCEL)
	{
		EndModal(wxID_CANCEL);
	}
	else if(event.GetId() == ACTION_ID_SAVE)
	{
		// save settings
		SaveMapSettings();
	}
	else if(event.GetId() == ACTION_ID_BUILD)
	{
		EndModal(wxID_OK);
	}
}

void CBuildOptionsDialog::SaveMapSettings()
{
	EqString level_name = g_pLevel->GetLevelName();
	EqString leveldir(EqString("worlds/")+level_name);

	// make a file	
	EqString configFile(leveldir + "/buildconfig.txt");

	KeyValues save_kv;

	kvkeybase_t* pSection = save_kv.GetRootSection()->AddKeyBase("BuildConfig");

	pSection->AddKeyBase("disabletree", varargs("%i", IsTreeBuildingDisabled()));
	pSection->AddKeyBase("enablecsg", varargs("%i", IsBrushCSGEnabled()));
	pSection->AddKeyBase("onlyents", varargs("%i", IsOnlyEntitiesMode()));
	pSection->AddKeyBase("physicsonly", varargs("%i", IsPhysicsOnly()));

	pSection->AddKeyBase("treemaxdepth", varargs("%i", GetTreeMaxDepth()));
	pSection->AddKeyBase("sectorboxsize", varargs("%i", GetSectorBoxSize()));

	pSection->AddKeyBase("lightmaps", varargs("%i", !IsLightmapsDisabled()));
	pSection->AddKeyBase("lightmapsize", varargs("%i", GetLightmapSize()));
	pSection->AddKeyBase("luxelspermeter", varargs("%g", GetLightmapLuxelsPerMeter()));

	save_kv.SaveToFile(configFile.GetData(), SP_MOD);
}

void CBuildOptionsDialog::LoadMapSettings()
{
	EqString level_name = g_pLevel->GetLevelName();
	EqString leveldir(EqString("worlds/")+level_name);

	// make a file	
	EqString configFile(leveldir + "/buildconfig.txt");

	KeyValues load_kv;
	if(load_kv.LoadFromFile(configFile.GetData()))
	{
		kvkeybase_t* pSection = load_kv.GetRootSection()->FindKeyBase("BuildConfig", KV_FLAG_SECTION);
		if(pSection)
		{
			m_pPhysicsOnlyCompile->SetValue(KV_GetValueBool(pSection->FindKeyBase("physicsonly")));
			m_pDisableTreeCheck->SetValue(KV_GetValueBool(pSection->FindKeyBase("disabletree")));
			m_pBrushCSGCheck->SetValue(KV_GetValueBool(pSection->FindKeyBase("enablecsg")));
			m_pSectorSizeSpin->SetValue(KV_GetValueString(pSection->FindKeyBase("sectorboxsize")));
			m_pTreeMaxDepthSpin->SetValue(KV_GetValueInt(pSection->FindKeyBase("treemaxdepth")));
			m_pOnlyEntsCheck->SetValue(KV_GetValueBool(pSection->FindKeyBase("onlyents")));
			m_pCompileLightmaps->SetValue(KV_GetValueBool(pSection->FindKeyBase("lightmaps")));
			m_pLuxelsPerMeter->SetValue(KV_GetValueString(pSection->FindKeyBase("luxelspermeter")));
			m_pLightmapSize->SetValue(KV_GetValueString(pSection->FindKeyBase("lightmapsize")));

		}
	}
}

int	CBuildOptionsDialog::GetTreeMaxDepth()
{
	return m_pTreeMaxDepthSpin->GetValue();
}

int	CBuildOptionsDialog::GetSectorBoxSize()
{
	return atoi(m_pSectorSizeSpin->GetValue().c_str());
}

bool CBuildOptionsDialog::IsTreeBuildingDisabled()
{
	return m_pDisableTreeCheck->GetValue();
}

bool CBuildOptionsDialog::IsBrushCSGEnabled()
{
	return m_pBrushCSGCheck->GetValue();
}

bool CBuildOptionsDialog::IsOnlyEntitiesMode()
{
	return m_pOnlyEntsCheck->GetValue();
}

bool CBuildOptionsDialog::IsPhysicsOnly()
{
	return m_pPhysicsOnlyCompile->GetValue();
}

bool CBuildOptionsDialog::IsLightmapsDisabled()
{
	return !m_pCompileLightmaps->GetValue();
}

float CBuildOptionsDialog::GetLightmapLuxelsPerMeter()
{
	return atof(m_pLuxelsPerMeter->GetValue().c_str());
}

int	CBuildOptionsDialog::GetLightmapSize()
{
	return atoi(m_pLightmapSize->GetValue().c_str());
}