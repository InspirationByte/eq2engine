//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech engine system class
//////////////////////////////////////////////////////////////////////////////////

#ifndef IENGINE_H
#define IENGINE_H

class IEngineSceneRenderer;
class IPhysics;
class IMaterialSystem;

#include "math/Vector.h"
#include "math/Matrix.h"

// Keys
#include "in_keys_ident.h"

// Fonts
#include "IFont.h"

#define SCRIPT_DEFAULT_PATH "scripts/"

#define IENGINEHOST_INTERFACE_VERSION "IEngineHost_001"

class IEngineHost
{
public:
	enum
	{
		QUIT_NOTQUITTING = 0,
		QUIT_TODESKTOP,
		QUIT_RESTART
	};

	virtual							~IEngineHost( void ) { }

	virtual int						Frame( void ) = 0;

	virtual double					GetFrameTime( void ) = 0;
	virtual double					GetCurTime( void ) = 0;

	virtual void					BeginScene() = 0;
	virtual void					EndScene() = 0;

	virtual void					ProcessKeyChar( ubyte chr ) = 0;	// TODO: utf8 support?
	virtual void					TrapKey_Event( int key, bool down ) = 0;
	virtual void					TrapMouse_Event( float x, float y, int buttons, bool down ) = 0;
	virtual void					TrapMouseMove_Event( int x, int y, float deltaX, float deltaY ) = 0;
	virtual void					TrapMouseWheel_Event(int x, int y, int scroll) = 0;

	virtual void					SetWindowSize(int width,int height) = 0;
	virtual void					SetCursorPosition(const IVector2D &pos) = 0;
	virtual IVector2D				GetCursorPosition() = 0;

	virtual void					SetCursorShow(bool bShow, bool console = false) = 0;

	virtual IVector2D				GetWindowSize() = 0;

	virtual void					StartTrapMode( void ) = 0;
	virtual bool					IsTrapping( void ) = 0;
	virtual bool					CheckDoneTrapping( int& buttons, int& key ) = 0;

	virtual int						GetQuitting( void ) = 0;
	virtual void					SetQuitting( int quittype ) = 0;

	virtual EQWNDHANDLE				GetWindowHandle() = 0;

	virtual IEqFont*				GetDefaultFont() = 0;
	virtual IEqFont*				LoadFont(const char* pszFontName) = 0;

	virtual IShaderAPI*				GetRenderer( void ) = 0;
	virtual IPhysics*				GetPhysics( void ) = 0;
	virtual	IMaterialSystem*		GetMaterialSystem( void ) = 0;

	virtual void					EnterResourceLoading() = 0;
	virtual void					EndResourceLoading() = 0;
	virtual int						GetResourceLoadingTimes() = 0;

};

extern IEngineHost *g_pEngineHost;

#endif //IENGINE_H
