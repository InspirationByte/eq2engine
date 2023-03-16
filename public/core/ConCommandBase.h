//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable - base class
//////////////////////////////////////////////////////////////////////////////////

#pragma once

static constexpr const char* cmdDefaultDescString = "No description";

enum ECommandBaseFlags
{
	CV_UNREGISTERED		= (1 << 0),	// Do not register this console command\cvar to list. So it can't be changed in console.
	CV_CHEAT			= (1 << 1), // Cheat. Must be checked from "__cheats" cvar
	CV_INITONLY			= (1 << 2), // Init-only console command such as "__cheats"
	CV_INVISIBLE		= (1 << 3), // Hidden console command. Does not shows in FastFind and others.
	CV_ARCHIVE			= (1 << 4), //Save on disk. Only ConVar have this flag
	CV_CLIENTCONTROLS	= (1 << 5), // Indicates that this command is client-controlled, and will disable on pause or when console is enabled

	CMDBASE_CONVAR		= (1 << 6), // Is ConVar
	CMDBASE_CONCOMMAND	= (1 << 7) // Is ConCommand
};

class ConCommandBase;
typedef void (*CMDBASE_VARIANTS_CALLBACK)(const ConCommandBase* base, Array<EqString>&, const char* query);

class ConCommandBase
{
	friend class ConVar;
	friend class ConCommand;
	friend class CConsoleCommands;

protected:
	template<typename CMD_TYPE, typename T>
	int CmdInit(CMD_TYPE* cv, T& val) { val(cv); return 0; }

public:
	struct Desc
	{
		Desc(const char* descriptionText) : descriptionText(descriptionText ? descriptionText : cmdDefaultDescString) {}
		void operator()(ConCommandBase* cmd) { cmd->m_szDesc = descriptionText; }

		const char* descriptionText;
	};

	struct Variants
	{
		Variants(CMDBASE_VARIANTS_CALLBACK cb) : cb(cb) {}
		void operator()(ConCommandBase* cmd) { cmd->m_fnVariantsList = cb; }

		CMDBASE_VARIANTS_CALLBACK cb;
	};

	ConCommandBase(char const* name, int flags);
	virtual ~ConCommandBase();

	// Names, descs, flags
	const char*		GetName()	const	{return m_szName;}
	const char*		GetDesc()	const	{return m_szDesc;}
	int				GetFlags()	const	{return m_nFlags;}

	bool			IsConVar( void )		const	{return (m_nFlags & CMDBASE_CONVAR) > 0;}
	bool			IsConCommand( void )	const	{return (m_nFlags & CMDBASE_CONCOMMAND) > 0;}
	bool			IsRegistered( void )	const	{return m_bIsRegistered;}

    static void		Register( ConCommandBase* pBase );
	static void		Unregister( ConCommandBase* pBase );

	bool			HasVariants() const;
	void			GetVariants(Array<EqString>& list, const char* query) const;

	void			SetVariantsCallback(CMDBASE_VARIANTS_CALLBACK fnVariants) {m_fnVariantsList = fnVariants;}

protected:

	const char*					m_szName{ nullptr };
	const char*					m_szDesc{ nullptr };
	CMDBASE_VARIANTS_CALLBACK	m_fnVariantsList{ nullptr };
	int							m_nFlags{ 0 };
	bool						m_bIsRegistered{ false };
};