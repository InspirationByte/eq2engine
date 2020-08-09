//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: UI elements
//////////////////////////////////////////////////////////////////////////////////

#ifndef DRVSYNHUD_H
#define DRVSYNHUD_H

#include "utils/eqwstring.h"
#include "car.h"
#include "materialsystem/MeshBuilder.h"

#include "EqUI_DrvSynTimer.h"

enum EHUDDisplayObjectFlags
{
	HUD_DOBJ_IS_TARGET	= (1 << 0),	// show this target on map
	HUD_DOBJ_CAR_DAMAGE	= (1 << 1), // show car damage above the hood

	// to be added later
	//HUDTARGET_FLAG_PLAYERINFO	= (1 << 2), // show player info if assigned to this car on top or on side of screen
};

enum EScreenAlertType
{
	HUD_ALERT_NORMAL = 0,
	HUD_ALERT_SUCCESS,
	HUD_ALERT_DANGER,
};

struct hudDisplayObject_t
{
	int				flags;		// flags or type
	CGameObject*	object;		// bound object
	Vector3D		point;		// static position if object is undefined

	float			flashValue; // flashing

	float			flashVelocity;
};

typedef std::map<int, hudDisplayObject_t>::iterator hudDisplayObjIterator_t;

//----------------------------------------------------------------------------------

class CDrvSynHUDManager
{
public:
								CDrvSynHUDManager();


	void						Init();
	void						Cleanup();

	void						InvalidateObjects();

	void						SetHudScheme(const char* name);

	// render the screen with maps and shit
	void						Render( float fDt, const IVector2D& screenSize, bool shownHud);
	void						Render3D( float fDt );

	// main object to display
	void						SetDisplayMainVehicle( CCar* car );

	// HUD map management
	int							AddTrackingObject( CGameObject* obj, int flags );
	int							AddMapTargetPoint( const Vector3D& position );
	void						RemoveTrackingObject( int handle );

	void						ShowMessage( const char* token, float time );
	void						ShowLastMessage();

	void						ShowAlert( const char* token, float time, int type); // EScreenAlertType

	void						SetTimeDisplay(bool enabled, double time);

	void						Enable(bool enable)		{m_enable = enable;}
	bool						IsEnabled() const		{return m_enable;}

	void						EnableInReplay(bool enable) { m_enableInReplay = enable; }

	void						ShowMap(bool enable)	{m_showMap = enable;}
	bool						IsMapShown() const		{return m_showMap;}

	void						ShowMotionBlur(bool enable) { m_showMotionBlur = enable; }
	bool						IsMotionBlurShown() const { return m_showMotionBlur; }

	void						FadeIn( bool useCurtains, float time );
	void						FadeOut( bool useCurtains, float time );

	equi::IUIControl*			FindChildElement(const char* name) const;
	equi::IUIControl*			GetRootElement() const;

	void						DrawWorldIntoMap(const CViewParams& params, float fDt);

protected:

	void						DrawDamageBar(CMeshBuilder& meshBuilder, Rectangle_t& rect, float percentage, float alpha = 1.0f);

	void						DoDebugDisplay();

	void						RenderBlur(const IVector2D& screenSize);

	std::map<int, hudDisplayObject_t>	m_displayObjects;

	EqWString							m_screenMessageText;
	EqWString							m_screenAlertText;

	float								m_screenMessageTime;

	ITexture*							m_mapTexture;
	ITexture*							m_mapRenderTarget;

	ITexture*							m_framebufferTex;
	ITexture*							m_blurAccumTex;
	IMaterial*							m_blurAccumMat;
	IMaterial*							m_blurMat;

	ILocToken*							m_felonyTok;

	EqString							m_schemeName;
	equi::IUIControl*					m_hudLayout;

	equi::IUIControl*					m_hudDamageBar;
	equi::IUIControl*					m_hudFelonyBar;
	equi::IUIControl*					m_hudMap;
	equi::DrvSynTimerElement*			m_hudTimer;

	IEqModel*							m_hudModels;
	CGameObject							m_hudObjectDummy;

	CCar*								m_mainVehicle;
	float								m_lightsTime;

	int									m_handleCounter;

	float								m_radarBlank;
	float								m_screenAlertTime;
	float								m_screenAlertInTime;

	EScreenAlertType					m_screenAlertType;

	float								m_fadeValue;
	float								m_fadeTarget;		// both time and target value

	bool								m_fadeCurtains;

	bool								m_enable;
	bool								m_enableInReplay;

	bool								m_showMap;
	bool								m_showMotionBlur;
};

#ifndef __INTELLISENSE__

OOLUA_PROXY( CDrvSynHUDManager )
	OOLUA_TAGS(Abstract)

	OOLUA_MFUNC( SetHudScheme )

	OOLUA_MFUNC( AddTrackingObject )
	OOLUA_MFUNC( AddMapTargetPoint )
	OOLUA_MFUNC( RemoveTrackingObject )

	OOLUA_MFUNC( ShowMessage )
	OOLUA_MFUNC( ShowAlert )

	OOLUA_MFUNC( SetTimeDisplay )

	OOLUA_MFUNC( Enable )
	OOLUA_MFUNC( EnableInReplay )
	OOLUA_MFUNC_CONST( IsEnabled )

	OOLUA_MFUNC( ShowMap )
	OOLUA_MFUNC_CONST( IsMapShown )

	OOLUA_MFUNC_CONST( FindChildElement )

	OOLUA_MFUNC( FadeIn )
	OOLUA_MFUNC( FadeOut )

OOLUA_PROXY_END
#endif // __INTELLISENSE__

extern CDrvSynHUDManager* g_pGameHUD;

#endif // DRVSYNHUD_H
