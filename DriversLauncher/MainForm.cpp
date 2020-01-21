#include "MainForm.h"

namespace NATIVE
{
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
}
#include <vcclr.h>

using namespace NATIVE;
using namespace System;
using namespace Security::Permissions;
using namespace Microsoft::Win32;

#define REGISTRY_PATH "SOFTWARE\\InspirationByte\\DriverSyndicate\\"

typedef void (*FUNC)(char* str, int len, void* extra);

void ForEachSeparated(char* str, char separator, FUNC fn, void* extra)
{
	char c = str[0];

	char* iterator = str;

	char* pFirst = str;
	char* pLast = NULL;

	while (c != 0)
	{
		c = *iterator;

		if (c == separator || c == 0)
		{
			pLast = iterator;

			int char_count = pLast - pFirst;

			if (char_count > 0)
				(*fn)(pFirst, char_count, extra);

			pFirst = iterator + 1;
		}

		iterator++;
	}
}

static bool r_fullscreen = false;
static char r_mode[32] = { 0 };
static char g_language[32] = { 0 };

void _GetParameters(char* str, int len, void* extra)
{
	*(str + len) = 0;

	char* foundStr = nullptr;

	if (foundStr = strstr(str, "r_fullscreen"))
	{
		char value[32];
		memset(value, 0, sizeof(value));

		int valueStart = strchr(foundStr, ' ') - foundStr;

		strncpy(value, foundStr+valueStart, len-valueStart);

		r_fullscreen = atoi(value);
	}
	else if (foundStr = strstr(str, "r_mode"))
	{
		memset(r_mode, 0, sizeof(r_mode));

		int valueStart = strchr(foundStr, ' ') - foundStr;

		strncpy(r_mode, foundStr + valueStart, len-valueStart);
	}
}

void LoadConfigFile()
{
	FILE* file = fopen("GameData/cfg/user.cfg", "rb");

	if (file)
	{
		fseek(file, 0, SEEK_END);
		long fileSize = ::ftell(file);
		fseek(file, 0, SEEK_SET);

		char* fileBuff = new char[fileSize];
		fread(fileBuff, 1, fileSize, file);

		fclose(file);

		// parse line by line
		ForEachSeparated(fileBuff, '\n', _GetParameters, nullptr);
	}
}

public ref class LanguageItem {
public:
	LanguageItem(String^ text, String^ value)
	{
		Text = text;
		Lang = value;
	}
	property String^ Text;
	property String^ Lang;

	String^ ToString() override {
		return Lang;
	}
};

System::Void DriversLauncher::MainForm::MainForm_Load(System::Object^  sender, System::EventArgs^  e)
{
	LoadConfigFile();

	String^ language;

	// load language
	{
		RegistryKey^ currentUserKey = Registry::CurrentUser;
		RegistryKey^ drvSynKey = currentUserKey->OpenSubKey(REGISTRY_PATH);

		if (drvSynKey)
		{
			language = drvSynKey->GetValue("Language")->ToString();
			drvSynKey->Close();
		}
			
	}


	DEVMODEW vDevMode;
	int i = 0;
	while (EnumDisplaySettingsW(nullptr, i, &vDevMode))
	{
		if (vDevMode.dmPelsWidth > 640 && vDevMode.dmPelsHeight > 480)
		{
			String^ resolution = (vDevMode.dmPelsWidth + "x" + vDevMode.dmPelsHeight);

			if (m_resolutionList->Items->IndexOf(resolution) == -1)
				m_resolutionList->Items->Add(resolution);
		}

		i++;
	}

	if (strlen(r_mode)) // use from config
		m_resolutionList->SelectedText = (String(r_mode)).Trim();
	else // or possible highest one
		m_resolutionList->SelectedIndex = m_resolutionList->Items->Count - 1;

	m_fullscreen->Checked = r_fullscreen;

	m_langSel->Items->Add(gcnew LanguageItem(L"English", "English"));
	m_langSel->Items->Add(gcnew LanguageItem(L"Français", "French"));
	m_langSel->Items->Add(gcnew LanguageItem(L"Русский", "Russian"));
	m_langSel->Items->Add(gcnew LanguageItem(L"Українська", "Ukrainian"));

	if (String::IsNullOrEmpty(language))
		m_langSel->SelectedIndex = 0;
	else
		m_langSel->SelectedIndex = Int32::Parse(language);
}

System::Void DriversLauncher::MainForm::playBtn_Click(System::Object^  sender, System::EventArgs^  e)
{
	LanguageItem^ selectedLang = (LanguageItem^)m_langSel->SelectedItem;

	// store language
	{
		RegistryKey^ currentUserKey = Registry::CurrentUser;
		RegistryKey^ drvSynKey = currentUserKey->OpenSubKey(gcnew String(REGISTRY_PATH), true);

		if (!drvSynKey)
			drvSynKey = currentUserKey->CreateSubKey(gcnew String(REGISTRY_PATH));

		drvSynKey->SetValue("Language", m_langSel->SelectedIndex);

		drvSynKey->Close();
	}

	String^ argumentsStr = " +seti r_fullscreen \"" + (m_fullscreen->Checked ? 1 : 0) + "\" +seti r_mode \"" + m_resolutionList->Text + "\" -language " + selectedLang->Lang;

	PROCESS_INFORMATION ProcessInfo; //This is what we get as an [out] parameter

	STARTUPINFO StartupInfo; //This is an [in] parameter
	ZeroMemory(&StartupInfo, sizeof(StartupInfo));
	StartupInfo.cb = sizeof StartupInfo; //Only compulsory field

	pin_ptr<const wchar_t> wch = PtrToStringChars(argumentsStr);

	BOOL bSuccess = CreateProcess(
		L".\\DrvSyn.exe",
		(wchar_t*)wch,
		NULL, NULL, FALSE, 0,
		NULL, NULL, &StartupInfo, &ProcessInfo);

	if (bSuccess == TRUE)
	{
		Application::Exit();
	}
	else
		MessageBox::Show(this, L"Failed to start DrvSyn.exe");
}