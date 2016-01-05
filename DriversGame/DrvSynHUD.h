//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: UI elements
//////////////////////////////////////////////////////////////////////////////////

#ifndef DRVSYNHUD_H
#define DRVSYNHUD_H

#include "math/DkMath.h"
#include "utils/eqwstring.h"
#include "car.h"

enum EHUDTargetFlags
{
	HUDTARGET_FLAG_SHOWONMAP	= (1 << 0),	// show this target on map
	HUDTARGET_FLAG_COP			= (1 << 1), // show cop mark and field of view on map
	HUDTARGET_FLAG_CAR_DAMAGE	= (1 << 2), // show car damage above the hood
	HUDTARGET_FLAG_PLAYER		= (1 << 3), // show player info if assigned to this car on top or on side of screen
};

struct hudDisplayObject_t
{
	int				handle;
	int				flags;
	CGameObject*	object;
	Vector3D		point;
};

//----------------------------------------------------------------------------------

class CDrvSynHUDManager
{
public:
								CDrvSynHUDManager();


	void						Init();
	void						Cleanup();

	// render the screen with maps and shit
	void						Render( float fDt, const IVector2D& screenSize); //, const Matrix4x4& projMatrix, const Matrix4x4& viewMatrix );

	// main object to display
	void						SetDisplayMainVehicle( CCar* car );

	// HUD map management
	int							AddTrackingObject( CGameObject* obj, int flags );
	void						RemoveTrackingObject( int handle );

	void						ShowScreenMessage( const char* token, float time );
	void						SetTimeDisplay(bool enabled, double time);

	void						Enable(bool enable)		{m_enable = enable;}
	bool						IsEnabled() const		{return m_enable;}

	void						ShowMap(bool enable)	{m_showMap = enable;}
	bool						IsMapShown() const		{return m_showMap;}

	void						FadeIn( bool useCurtains = false );
	void						FadeOut();

protected:

	bool						m_enable;
	bool						m_showMap;


	ITexture*					m_mapTexture;
	
	DkList<hudDisplayObject_t>	m_displayObjects;
	int							m_handleCounter;

	CCar*						m_mainVehicle;
	float						m_curTime;

	float						m_screenMessageTime;
	EqWString					m_screenMessageText;

	bool						m_timeDisplayEnable;
	double						m_timeDisplayValue;

	ILocToken*					m_damageTok;
	ILocToken*					m_felonyTok;
};


#ifndef __INTELLISENSE__

OOLUA_PROXY( CDrvSynHUDManager )
	OOLUA_TAGS(Abstract)

	OOLUA_MFUNC( AddTrackingObject )
	OOLUA_MFUNC( RemoveTrackingObject )
	OOLUA_MFUNC( ShowScreenMessage )
	OOLUA_MFUNC( SetTimeDisplay )

	OOLUA_MFUNC( Enable )
	OOLUA_MFUNC_CONST( IsEnabled )

	OOLUA_MFUNC( ShowMap )
	OOLUA_MFUNC_CONST( IsMapShown )

	OOLUA_MFUNC( FadeIn )
	OOLUA_MFUNC( FadeOut )

OOLUA_PROXY_END
#endif // __INTELLISENSE__

extern CDrvSynHUDManager* g_pGameHUD;

#endif // DRVSYNHUD_H