//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Film director
//////////////////////////////////////////////////////////////////////////////////

#include "director.h"

#include "session_stuff.h"

#include "CameraAnimator.h"
#include "materialsystem/MeshBuilder.h"

#include "sys_host.h"
#include "FontCache.h"

#include "KeyBinding/InputCommandBinder.h"

#include "world.h"
#include "input.h"

#include "DrvSynHUD.h"

extern ConVar	g_pause;

void g_freecam_changed(ConVar* pVar,char const* pszOldValue);

ConVar			g_freecam("g_freecam", "0", g_freecam_changed, nullptr, 0 );
ConVar			g_freecam_speed("g_freecam_speed", "10", NULL, CV_ARCHIVE);

ConVar			g_director("director", "0");
ConVar			g_mouse_sens("g_mouse_sens", "1.0", "mouse sensitivity", CV_ARCHIVE);

ConVar			director_timeline_zoom("director_timeline_zoom", "1.0", 0.1, 10.0, "Timeline scale", CV_ARCHIVE);

int				g_nDirectorCameraType = CAM_MODE_OUTCAR;

freeCameraProps_t::freeCameraProps_t()
{
	fov = DIRECTOR_DEFAULT_CAMERA_FOV;
	position = vec3_zero;
	angles = vec3_zero;
	velocity = vec3_zero;
	zAxisMove = false;
}

freeCameraProps_t g_freeCamProps;

void g_freecam_changed(ConVar* pVar, char const* pszOldValue)
{
	// if we switching to free camera, we should reset Z axis rotation
	if(g_pCameraAnimator->GetRealMode() == CAM_MODE_INCAR && pVar->GetBool())
		g_freeCamProps.angles.z = 0.0f;
}

// refers to ECameraMode
static const char* s_cameraTypeString[] = {
	"Standard outside view",
	"Attached on car",
	"Inside car",
	"Tripod",
	"Tripod - follow",
	"Tripod - follow with zoom",
};

static const ColorRGB s_cameraColors[] = {
	ColorRGB(1.0f,0.25f,0.25f),		// standard view
	ColorRGB(0.0f,0.25f,0.65f),		// inside car
	ColorRGB(1.0f,0.25f,0.05f),		// on car
	ColorRGB(0.8f,0.8f,0.2f),		// tripod
	ColorRGB(0.5f,0.2f,0.7f),		// tripod follow
	ColorRGB(0.2f,0.7f,0.2f),		// tripod follow with zoom
};

bool g_director_ShiftKey = false;
bool g_director_CtrlKey = false;

const float DIRECTOR_FASTFORWARD_TIMESCALE = 4.0f;

extern ConVar sys_timescale;

void Director_Enable( bool enable )
{
	g_director.SetBool(enable);
}

bool Director_IsActive()
{
	return g_director.GetBool() && g_replayTracker->m_state == REPL_PLAYING;
}

void Director_Reset()
{
	//reset cameras
	g_nDirectorCameraType = 0;
}

void Director_MouseMove( int x, int y, float deltaX, float deltaY  )
{
	if(!g_freecam.GetBool())
		return;

	if(g_freeCamProps.zAxisMove)
	{
		g_freeCamProps.angles.z += deltaX * g_mouse_sens.GetFloat();
	}
	else
	{
		g_freeCamProps.angles.x += deltaY * g_mouse_sens.GetFloat();
		g_freeCamProps.angles.y += deltaX * g_mouse_sens.GetFloat();
	}
}

void Director_GetNewCameraProps(replayCamera_t* cam)
{
	cam->fov = g_freeCamProps.fov;
	cam->origin = g_freeCamProps.position;
	cam->type = g_nDirectorCameraType;

	if (Director_FreeCameraActive())
		cam->rotation = g_freeCamProps.angles;
	else
		cam->rotation = g_pCameraAnimator->GetAngles();	// only update given angles

	if (cam->type == CAM_MODE_ONCAR)
	{
		CGameObject* viewedObject = g_pGameSession->GetViewObject();

		Vector3D camDir;
		AngleVectors(cam->rotation, &camDir);

		Vector3D localPos = (!viewedObject->m_worldMatrix * Vector4D(cam->origin, 1.0f)).xyz();
		Vector3D localCamDir = (!viewedObject->m_worldMatrix.getRotationComponent() * camDir);

		// preserve Z rotation
		float zRotation = cam->rotation.z;

		cam->origin = localPos;
		cam->rotation = -VectorAngles(localCamDir);
		cam->rotation.z = -zRotation;
	}
}

enum EDirectorActionType
{
	DIRECTOR_CAMERA_ADD = 0,
	DIRECTOR_CAMERA_RESET,
	DIRECTOR_CAMERA_REMOVE,
	DIRECTOR_CAMERA_WAYPOINT_SET_START,
	DIRECTOR_CAMERA_WAYPOINT_SET_END,

	DIRECTOR_PREVCAMERA,
	DIRECTOR_NEXTCAMERA,
};

void Director_Action(EDirectorActionType type)
{
	if (!Director_IsActive())
		return;

	// only execute director actions on pause
	if (!g_pause.GetBool())
		return;

	CGameObject* viewedObject = g_pGameSession->GetViewObject();

	int replayCamera = g_replayTracker->m_currentCamera;
	replayCamera_t* currentCamera = g_replayTracker->GetCurrentCamera();

	if (type == DIRECTOR_CAMERA_ADD)
	{
		replayCamera_t cam;
		cam.targetIdx = viewedObject->m_replayID;
		cam.startTick = g_replayTracker->m_tick;
		Director_GetNewCameraProps(&cam);

		// zero camera rotation pls
		if(cam.type == CAM_MODE_OUTCAR)
			cam.rotation = vec3_zero;

		int camIndex = g_replayTracker->AddCamera(cam);
		g_replayTracker->m_currentCamera = camIndex;

		// set camera after keypress
		g_freecam.SetBool(false);

		Msg("New camera at %d\n", cam.startTick);
	}
	else if (type == DIRECTOR_CAMERA_RESET)
	{
		if (currentCamera)
		{
			Msg("Reset camera at %d\n", currentCamera->startTick);

			currentCamera->targetIdx = viewedObject->m_replayID;
			Director_GetNewCameraProps(currentCamera);

			g_freecam.SetBool(false);
		}
	}
	else if (type == DIRECTOR_CAMERA_REMOVE)
	{
		if (replayCamera >= 0 && g_replayTracker->m_cameras.numElem())
		{
			g_replayTracker->m_cameras.removeIndex(replayCamera);
			g_replayTracker->m_currentCamera--;
		}
	}
	else if (type == DIRECTOR_CAMERA_WAYPOINT_SET_START || type == DIRECTOR_CAMERA_WAYPOINT_SET_END)
	{
		// TODO: add keyframe
	}
	else if (type == DIRECTOR_PREVCAMERA)
	{
		if (g_replayTracker->m_cameras.inRange(replayCamera - 1))
			g_replayTracker->m_currentCamera--;
	}
	else if (type == DIRECTOR_NEXTCAMERA)
	{
		if (g_replayTracker->m_cameras.inRange(replayCamera + 1))
			g_replayTracker->m_currentCamera++;
	}
}

DECLARE_CMD(director_pick_ray, "Director mode - picks object with ray", 0)
{
	if (!Director_IsActive())
		return;

	CCar* pickedCar = Director_GetCarOnCrosshair();

	if (pickedCar != nullptr)
	{
		g_pGameSession->SetViewObject(pickedCar);
	}
}

void Director_KeyPress(int key, bool down)
{
	if(!Director_IsActive())
		return;

	if(key == KEY_SHIFT)
	{
		g_director_ShiftKey = down;
	}
	else if (key == KEY_CTRL)
	{
		g_director_CtrlKey = down;
	}
	else if(key == KEY_BACKSPACE)
	{
		sys_timescale.SetFloat( down ? DIRECTOR_FASTFORWARD_TIMESCALE : 1.0f );
	}
	else if (key == KEY_W)
	{
		if(down)
			g_freeCamProps.buttons |= IN_FORWARD;
		else
			g_freeCamProps.buttons &= ~IN_FORWARD;
	}
	else if (key == KEY_S)
	{
		if (down)
			g_freeCamProps.buttons |= IN_BACKWARD;
		else
			g_freeCamProps.buttons &= ~IN_BACKWARD;
	}
	else if (key == KEY_A)
	{
		if (down)
			g_freeCamProps.buttons |= IN_LEFT;
		else
			g_freeCamProps.buttons &= ~IN_LEFT;
	}
	else if (key == KEY_D)
	{
		if (down)
			g_freeCamProps.buttons |= IN_RIGHT;
		else
			g_freeCamProps.buttons &= ~IN_RIGHT;
	}

	// only execute director actions on pause
	if (!g_pause.GetBool())
		return;

	if (down)
	{
		//Msg("Director mode keypress: %d\n", key);

		int replayCamera = g_replayTracker->m_currentCamera;
		replayCamera_t* currentCamera = g_replayTracker->GetCurrentCamera();

		if (key >= KEY_1 && key <= KEY_6)
		{
			g_nDirectorCameraType = key - KEY_1;
		}
		else if (key == KEY_R)
		{
			g_State_Game->ReplayFastSeek(0);
		}
		else if (key == KEY_PGUP)
		{
			Director_Action(DIRECTOR_NEXTCAMERA);
		}
		else if (key == KEY_PGDN)
		{
			Director_Action(DIRECTOR_PREVCAMERA);
		}
		else if (key == KEY_ADD)
		{
			Director_Action(DIRECTOR_CAMERA_ADD);
		}
		else if (key == KEY_KP_ENTER)
		{
			Director_Action(DIRECTOR_CAMERA_RESET);
		}
		else if (key == KEY_DELETE)
		{
			Director_Action(DIRECTOR_CAMERA_REMOVE);
		}
		else
		{
			replayCamera_t* prevCamera = g_replayTracker->m_cameras.inRange(replayCamera - 1) ? &g_replayTracker->m_cameras[replayCamera - 1] : NULL;
			replayCamera_t* nextCamera = g_replayTracker->m_cameras.inRange(replayCamera + 1) ? &g_replayTracker->m_cameras[replayCamera + 1] : NULL;
			int totalTicks = g_replayTracker->m_numFrames;

			int highTick = nextCamera ? nextCamera->startTick : totalTicks;
			int lowTick = prevCamera ? prevCamera->startTick : 0;

			if (key == KEY_LEFT)
			{
				if (g_director_CtrlKey)
				{
					g_State_Game->ReplayFastSeek( g_replayTracker->m_tick - 100);
					return;
				}
				else if (currentCamera)
				{
					currentCamera->startTick -= g_director_ShiftKey ? 10 : 1;

					if (currentCamera->startTick < lowTick)
						currentCamera->startTick = lowTick;
				}

			}
			else if (key == KEY_RIGHT)
			{
				if (g_director_CtrlKey)
				{
					g_State_Game->ReplayFastSeek(g_replayTracker->m_tick + 100);
					return;
				}
				else if (currentCamera)
				{
					currentCamera->startTick += g_director_ShiftKey ? 10 : 1;

					if (currentCamera->startTick > highTick)
						currentCamera->startTick = highTick;
				}
			}
		}
	}
}

CCar* Director_GetCarOnCrosshair(bool queryImportantOnly /*= true*/)
{
	Vector3D start = g_freeCamProps.position;
	Vector3D dir;
	AngleVectors(g_freeCamProps.angles, &dir);

	Vector3D end = start + dir*1000.0f;

	CollisionData_t coll;
	g_pPhysics->TestLine(start, end, coll, OBJECTCONTENTS_VEHICLE);

	if(coll.hitobject != NULL && (coll.hitobject->m_flags & BODY_ISCAR))
	{
		CCar* pickedCar = (CCar*)coll.hitobject->GetUserData();

		bool isPursuer = false;
		if(pickedCar->ObjType() == GO_CAR_AI)
		{
			CAITrafficCar* potentialPursuer = (CAITrafficCar*)pickedCar;
			isPursuer = potentialPursuer->IsPursuer();
		}

		if(queryImportantOnly)
		{
			if(!(isPursuer || (pickedCar->GetScriptID() != SCRIPT_ID_NOTSCRIPTED)))
				return NULL;
		}

		return pickedCar;
	}

	return NULL;
}

bool Director_FreeCameraActive()
{
	return g_freecam.GetBool();
}

void Director_UpdateFreeCamera(float fDt)
{
	Vector3D f, r;
	AngleVectors(g_freeCamProps.angles, &f, &r);

	Vector3D camMoveVec(0.0f);

	if(!Director_IsActive())
		g_freeCamProps.buttons = g_nClientButtons;

	if(g_freeCamProps.buttons & IN_FORWARD)
		camMoveVec += f;
	else if(g_freeCamProps.buttons & IN_BACKWARD)
		camMoveVec -= f;

	if(g_freeCamProps.buttons & IN_LEFT)
		camMoveVec -= r;
	else if(g_freeCamProps.buttons & IN_RIGHT)
		camMoveVec += r;

	float joyFreecamMultiplier = (g_nClientButtons & IN_FASTSTEER) ? 2.0f : 1.0f;

	camMoveVec += r * g_joyFreecamMove.x;
	camMoveVec += f * g_joyFreecamMove.y;

	g_freeCamProps.angles.x += g_joyFreecamLook.x * joyFreecamMultiplier;
	g_freeCamProps.angles.y += g_joyFreecamLook.y * joyFreecamMultiplier;

	g_freeCamProps.velocity += camMoveVec * 200.0f * fDt;

	float camSpeed = length(g_freeCamProps.velocity);

	// limit camera speed
	if(camSpeed > g_freecam_speed.GetFloat())
	{
		float speedDiffScale = g_freecam_speed.GetFloat() / camSpeed;
		g_freeCamProps.velocity *= speedDiffScale;
	}

	btSphereShape collShape(0.5f);

	// update camera collision
	if(camSpeed > 1.0f)
	{
		g_freeCamProps.velocity -= normalize(g_freeCamProps.velocity) * 90.0f * fDt;

		eqPhysCollisionFilter filter;
		filter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
		filter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_STATICOBJECTS;

		int cycle = 0;

		CollisionData_t coll;
		while(g_pPhysics->TestConvexSweep(&collShape, Quaternion(0,0,0,0), g_freeCamProps.position, g_freeCamProps.position+g_freeCamProps.velocity, coll, 0xFFFFFFFF, &filter))
		{
			if(coll.fract == 0.0f)
			{
				float nDot = dot(coll.normal, g_freeCamProps.velocity);
				g_freeCamProps.velocity -= coll.normal*nDot;
			}

			filter.AddObject( coll.hitobject );

			cycle++;
			if(cycle > MAX_COLLISION_FILTER_OBJECTS)
				break;
		}
	}
	else
	{
		g_freeCamProps.velocity = vec3_zero;
	}

	/*
	g_pPhysics->m_physics.SetDebugRaycast(true);

	// test code, must be removed after fixing raycast broadphase
	CollisionData_t coll;
	g_pPhysics->TestConvexSweep(&collShape, Quaternion(0,0,0,0), g_freeCamProps.position, g_freeCamProps.position+f*2000.0f, coll, 0xFFFFFFFF);

	debugoverlay->Box3D(coll.position - 0.5f, coll.position + 0.5f, ColorRGBA(0,1,0,0.25f), 0.1f);
	debugoverlay->Line3D(coll.position, coll.position + coll.normal, ColorRGBA(0,0,1,0.25f), ColorRGBA(0,0,1,0.25f) );

	g_pPhysics->m_physics.SetDebugRaycast(false);
	*/

	g_freeCamProps.position += g_freeCamProps.velocity * fDt;
}

void Director_Draw( float fDt )
{
	if(!Director_IsActive())
		return;

	const IVector2D& screenSize = g_pHost->GetWindowSize();
	materials->Setup2D(screenSize.x,screenSize.y);

	g_pGameHUD->GetRootElement()->SetSize(screenSize);

	Vector2D scaling = g_pGameHUD->GetRootElement()->GetSizeDiffPerc();
	const float aspectCorrection = scaling.y / scaling.x;	// Width based aspect
	scaling *= Vector2D(aspectCorrection, 1.0f);
	
	const Vector2D textSize = scaling * 10.0f;

	replayCamera_t* currentCamera = g_replayTracker->GetCurrentCamera();
	int replayCamera = g_replayTracker->m_currentCamera;
	int currentTick = g_replayTracker->m_tick;
	int totalTicks = g_replayTracker->m_numFrames;

	int totalCameras = g_replayTracker->m_cameras.numElem();

	Rectangle_t timelineRect(0.0f,screenSize.y-40.0f*scaling.y, (float)screenSize.x, screenSize.y-20.0f*scaling.y);
	float timelineCenterPos = timelineRect.GetCenter().x;
	float timelineHalfThickness = timelineRect.GetSize().y*0.5f;

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	g_pShaderAPI->SetTexture(0,0,0);
	materials->SetRasterizerStates(CULL_NONE);
	materials->SetDepthStates(false,false);
	materials->SetBlendingStates(blending);
	materials->BindMaterial(materials->GetDefaultMaterial());

	const float pixelsPerTick = 1.0f / 4.0f * director_timeline_zoom.GetFloat();
	const float currentTickOffset = currentTick*pixelsPerTick;
	const float lastTickOffset = totalTicks*pixelsPerTick;

	CGameObject* viewedObject = g_pGameSession->GetViewObject();
	CCar* playerCar = g_pGameSession->GetPlayerCar();
	CCar* leadCar = g_pGameSession->GetLeadCar();

	// if car is no longer valid, resetthe viewed car
	if(!g_pGameWorld->IsValidObject(viewedObject))
	{
		g_pGameSession->SetViewObject(nullptr);
		viewedObject = g_pGameSession->GetViewObject();
	}

	if (!g_pGameWorld->IsValidObject(playerCar))
		playerCar = nullptr;

	if (!g_pGameWorld->IsValidObject(leadCar))
		leadCar = nullptr;

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		float ticksOffset = lastTickOffset-currentTickOffset;

		// draw timeline transparent part
		Rectangle_t drawnTimeline(
			timelineRect.GetCenter().x - currentTickOffset, 
			timelineRect.GetCenter().y - timelineHalfThickness * 0.8f,
			timelineRect.GetCenter().x + ticksOffset, 
			timelineRect.GetCenter().y + timelineHalfThickness * 0.8f
		);

		drawnTimeline.vleftTop.x = clamp(drawnTimeline.vleftTop.x,0.0f,timelineRect.vrightBottom.x);
		drawnTimeline.vrightBottom.x = clamp(drawnTimeline.vrightBottom.x,0.0f,timelineRect.vrightBottom.x);

		meshBuilder.Color4f(1,1,1, 0.25f);
		meshBuilder.Quad2(drawnTimeline.GetLeftTop(), drawnTimeline.GetRightTop(), drawnTimeline.GetLeftBottom(), drawnTimeline.GetRightBottom());

		// draw cameras on the timeline
		for (int i = 0; i < totalCameras; i++)
		{
			replayCamera_t* camera = &g_replayTracker->m_cameras[i];

			float cameraTickPos = (camera->startTick - currentTick) * pixelsPerTick;

			replayCamera_t* nextCamera = i + 1 < g_replayTracker->m_cameras.numElem() ? &g_replayTracker->m_cameras[i + 1] : NULL;
			float nextTickPos = ((nextCamera ? nextCamera->startTick : totalTicks) - currentTick) * pixelsPerTick;

			// draw colored rectangle
			Rectangle_t cameraColorRect(
				timelineRect.GetCenter().x + cameraTickPos, 
				timelineRect.GetCenter().y - timelineHalfThickness * 0.5f,
				timelineRect.GetCenter().x + nextTickPos, 
				timelineRect.GetCenter().y + timelineHalfThickness * 0.5f
			);

			ColorRGB camRectColor(s_cameraColors[camera->type]);

			float tickScale = 0.8f;

			if (currentCamera == camera)
			{
				tickScale = 2.0f;

				// draw start tick position
				Rectangle_t currentTickRect(
					timelineRect.GetCenter() - Vector2D(2, timelineHalfThickness * 2.0f) + Vector2D(cameraTickPos, 0),
					timelineRect.GetCenter() + Vector2D(2, -timelineHalfThickness) + Vector2D(cameraTickPos, 0));

				meshBuilder.Color4f(1.0f, 0.0f, 0.0f, 0.8f);
				meshBuilder.Triangle2(currentTickRect.GetLeftTop(), currentTickRect.GetCenter() + Vector2D(16.0f,0), currentTickRect.GetLeftBottom());

				if (!g_freecam.GetBool())
				{
					Rectangle_t cameraFixedRect = cameraColorRect;
					cameraFixedRect.vrightBottom.y += 5;
					meshBuilder.Color4f(0.25f, 0.0f, 1.0f, 0.5f);
					meshBuilder.Quad2(cameraFixedRect.GetLeftTop(), cameraFixedRect.GetRightTop(), cameraFixedRect.GetLeftBottom(), cameraFixedRect.GetRightBottom());
				}

				if (g_pause.GetBool())
					camRectColor *= fabs(sinf((float)g_pHost->GetCurTime()*2.0f));
			}

			meshBuilder.Color4fv(ColorRGBA(camRectColor, 0.7f));
			meshBuilder.Quad2(cameraColorRect.GetLeftTop(), cameraColorRect.GetRightTop(), cameraColorRect.GetLeftBottom(), cameraColorRect.GetRightBottom());

			// draw start tick position
			Rectangle_t currentTickRect(
				timelineRect.GetCenter() - Vector2D(2, timelineHalfThickness * tickScale) + Vector2D(cameraTickPos,0),
				timelineRect.GetCenter() + Vector2D(2, timelineHalfThickness * 0.8f) + Vector2D(cameraTickPos,0));

			meshBuilder.Color4f(0.9f,0.9f,0.9f,0.8f);
			meshBuilder.Quad2(currentTickRect.GetLeftTop(), currentTickRect.GetRightTop(), currentTickRect.GetLeftBottom(), currentTickRect.GetRightBottom());
		}

		// current tick
		Rectangle_t currentTickRect(
			timelineRect.GetCenter() - Vector2D(2, timelineHalfThickness), 
			timelineRect.GetCenter() + Vector2D(2, timelineHalfThickness));

		meshBuilder.Color4f(0,0,0,1.0f);
		meshBuilder.Quad2(currentTickRect.GetLeftTop(), currentTickRect.GetRightTop(), currentTickRect.GetLeftBottom(), currentTickRect.GetRightBottom());

		// end tick
		Rectangle_t lastTickRect(
			timelineRect.GetCenter() - Vector2D(2, timelineHalfThickness * 0.8f) + Vector2D(ticksOffset,0), 
			timelineRect.GetCenter() + Vector2D(2, timelineHalfThickness * 0.8f) + Vector2D(ticksOffset,0));

		meshBuilder.Color4f(1,0.05f,0,1.0f);
		meshBuilder.Quad2(lastTickRect.GetLeftTop(), lastTickRect.GetRightTop(), lastTickRect.GetLeftBottom(), lastTickRect.GetRightBottom());

		if (g_freecam.GetBool())
		{
			// draw current viewed car marker
			if (viewedObject)
			{
				Vector3D screenPos;
				PointToScreen_Z(viewedObject->GetOrigin() + Vector3D(0, 1.0f, 0), screenPos, g_pGameWorld->m_viewprojection, Vector2D((float)screenSize.x, (float)screenSize.y));

				if (screenPos.z > 0.0f)
				{
					meshBuilder.Color4f(0.0f, 0.7f, 0.0f, 0.5f);
					meshBuilder.Triangle2(screenPos.xy() + Vector2D(-20, -20), screenPos.xy(), screenPos.xy() + Vector2D(20, -20));
				}
			}

			// draw lead car marker
			if (leadCar)
			{
				Vector3D screenPos;
				PointToScreen_Z(leadCar->GetOrigin() + Vector3D(0, 1.0f, 0), screenPos, g_pGameWorld->m_viewprojection, Vector2D((float)screenSize.x, (float)screenSize.y));

				if (screenPos.z > 0.0f)
				{
					meshBuilder.Color4f(0.9f, 0.7f, 0.0f, 0.5f);
					meshBuilder.Triangle2(screenPos.xy() + Vector2D(-20, -20), screenPos.xy(), screenPos.xy() + Vector2D(20, -20));
				}
			}

			// draw player car marker
			if (playerCar)
			{
				Vector3D screenPos;
				PointToScreen_Z(playerCar->GetOrigin() + Vector3D(0, 1.0f, 0), screenPos, g_pGameWorld->m_viewprojection, Vector2D((float)screenSize.x, (float)screenSize.y));

				if (screenPos.z > 0.0f)
				{
					meshBuilder.Color4f(0.0f, 0.2f, 0.5f, 0.5f);
					meshBuilder.Triangle2(screenPos.xy() + Vector2D(-20, -20), screenPos.xy(), screenPos.xy() + Vector2D(20, -20));
				}
			}

			// draw crosshair
			Vector2D halfScreen = Vector2D(screenSize)*0.5f;

			Vector2D crosshair[] =
			{
				Vector2D(halfScreen + Vector2D(0,-3)),
				Vector2D(halfScreen + Vector2D(3,3)),
				Vector2D(halfScreen + Vector2D(-3,3))
			};

			meshBuilder.Color4f(1, 1, 1, 0.45);
			meshBuilder.Triangle2(crosshair[0], crosshair[1], crosshair[2]);
		}
	meshBuilder.End();

	eqFontStyleParam_t params;
	params.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_USE_TAGS;
	params.textColor = color4_white;
	params.scale = textSize;

	static IEqFont* font = g_fontCache->GetFont("Roboto", 30);

	Vector2D frameInfoTextPos(screenSize.x / 2, screenSize.y - (screenSize.y / 6));

	// Print the hotkey information
	{
		static ConCommandBase* cmd_togglevar = (ConCommandBase*)g_sysConsole->FindBase("togglevar");

		EqString play_pause_bind("UNBOUND");
		UTIL_GetBindingKeyString(play_pause_bind, g_inputCommandBinder->FindBindingByCommand(cmd_togglevar, g_pause.GetName()));

		EqString freecam_bind("UNBOUND");
		UTIL_GetBindingKeyString(freecam_bind, g_inputCommandBinder->FindBindingByCommand(cmd_togglevar, g_freecam.GetName()));

		EqString director_pick_ray_bind("UNBOUND");
		UTIL_GetBindingKeyString(director_pick_ray_bind, g_inputCommandBinder->FindBindingByCommand(&cmd_director_pick_ray));

		const char* cameraPropsStr = nullptr;

		if (g_freecam.GetBool())
		{
			cameraPropsStr = varargs(
				"Replay camera: &#FFFF00;%s&;\n\n"

				"Type: &#FFFF00;1 - 6&;\n"
				"&#FFFF00;1&; %s Standard outside view&;\n"
				"&#FFFF00;2&; %s Attached on car&;\n"
				"&#FFFF00;3&; %s Inside car&;\n"
				"&#FFFF00;4&; %s Tripod&;\n"
				"&#FFFF00;5&; %s Tripod - follow&;\n"
				"&#FFFF00;6&; %s Tripod - follow with zoom&;\n\n"

				"Zoom: &#FFFF00;MOUSE WHEEL&; (%.2f deg.)\n"
				"Tilt: &#FFFF00;RMB and MOUSE MOVE&; (%.2f deg.)\n\n"

				"Insert camera: &#FFFF00;NUMPAD ADD&;\n"
				"Replace camera: &#FFFF00;NUMPAD ENTER&;\n"
				"Delete camera: &#FFFF00;DELETE&;\n\n"

				"Move: &#FFFF00;WSAD&;\n\n"

				"Set vehicle: &#FFFF00;%s&;\n",

				freecam_bind.c_str(),

				g_nDirectorCameraType == 0 ? "&#FF0000;" : "",
				g_nDirectorCameraType == 1 ? "&#FF0000;" : "",
				g_nDirectorCameraType == 2 ? "&#FF0000;" : "",
				g_nDirectorCameraType == 3 ? "&#FF0000;" : "",
				g_nDirectorCameraType == 4 ? "&#FF0000;" : "",
				g_nDirectorCameraType == 5 ? "&#FF0000;" : "",

				g_freeCamProps.fov,
				g_freeCamProps.angles.z,

				director_pick_ray_bind.c_str()
				);
		}
		else
		{
			int cameraType = currentCamera ? currentCamera->type : g_nDirectorCameraType;
			float fov = currentCamera ? (float)currentCamera->fov : g_freeCamProps.fov;
			float tilt = currentCamera ? (float)currentCamera->rotation.z : g_freeCamProps.angles.z;

			cameraPropsStr = varargs(
				"Free camera: &#FFFF00;%s&;\n\n"

				"Type: &#FFFF00;1 - 6&;\n"
				"&#FFFF00;1&; %s%s Standard outside view&;&;\n"
				"&#FFFF00;2&; %s%s Attached on car&;&;\n"
				"&#FFFF00;3&; %s%s Inside car&;&;\n"
				"&#FFFF00;4&; %s%s Tripod&;&;\n"
				"&#FFFF00;5&; %s%s Tripod - follow&;&;\n"
				"&#FFFF00;6&; %s%s Tripod - follow with zoom&;&;\n\n"

				"Zoom: &#FFFF00;MOUSE WHEEL&; (%.2f deg.)\n"
				"Tilt: &#FFFF00;RMB and MOUSE MOVE&; (%.2f deg.)\n\n"

				"Insert camera: &#FFFF00;NUMPAD ADD&;\n"
				"Replace camera: &#FFFF00;NUMPAD ENTER&;\n"
				"Delete camera: &#FFFF00;DELETE&;\n\n",

				freecam_bind.c_str(),

				cameraType == 0 ? "&#FFFF00;>" : "",
				g_nDirectorCameraType == 0 ? "&#FF0000;" : "",

				cameraType == 1 ? "&#FFFF00;>" : "",
				g_nDirectorCameraType == 1 ? "&#FF0000;" : "",

				cameraType == 2 ? "&#FFFF00;>" : "",
				g_nDirectorCameraType == 2 ? "&#FF0000;" : "",

				cameraType == 3 ? "&#FFFF00;>" : "",
				g_nDirectorCameraType == 3 ? "&#FF0000;" : "",

				cameraType == 4 ? "&#FFFF00;>" : "",
				g_nDirectorCameraType == 4 ? "&#FF0000;" : "",

				cameraType == 5 ? "&#FFFF00;>" : "",
				g_nDirectorCameraType == 5 ? "&#FF0000;" : "",

				fov,
				tilt);
		}

		const char* controlsText = varargs(
			"Play: &#FFFF00;%s&;\n"
			"Rewind: &#FFFF00;R&;\n"
			"Goto FRAME: &#FFFF00;CTRL+ARROWS&; or &#FFFF00;fastseek <frame>&; (in console)\n",

			play_pause_bind.c_str()
		);

		const char* shortText = varargs(
			"Pause: &#FFFF00;%s&;\n"
			"Fast Forward: &#FFFF00;BACKSPACE&;\n",
			play_pause_bind.c_str());

		Vector2D playbackControlsTextPos(screenSize.x - 25, 45);
		Vector2D cameraPropsTextPos(25, 45);

		if (g_pause.GetBool())
		{
			params.align = TEXT_ALIGN_RIGHT;
			font->RenderText(controlsText, playbackControlsTextPos, params);

			params.align = TEXT_ALIGN_LEFT;
			font->RenderText(cameraPropsStr, cameraPropsTextPos, params);

			Vector2D cameraTextPos(25, frameInfoTextPos.y);

			if (currentCamera)
			{
				params.align = TEXT_ALIGN_LEFT;

				char* cameraStr = varargs(
					"CAMERA START: &#FFFF00;ARROWS&; (FRAME %d)\n"
					"NEXT CAMERA: &#FFFF00;PAGE UP&;\n"
					"PREV CAMERA: &#FFFF00;PAGE DN&;\n\n",
					currentCamera ? currentCamera->startTick : 0);

				font->RenderText(cameraStr, cameraTextPos, params);
			}
			
		}
		else
		{
			params.align = TEXT_ALIGN_RIGHT;
			font->RenderText(shortText, playbackControlsTextPos, params);

			params.align = TEXT_ALIGN_LEFT;
			font->RenderText(cameraPropsStr, cameraPropsTextPos, params);
		}
	}

	params.align = TEXT_ALIGN_HCENTER;

	if(g_freecam.GetBool())
	{
		CCar* carOnCrosshair = Director_GetCarOnCrosshair();

		if(carOnCrosshair && carOnCrosshair != viewedObject)
		{
			Vector3D screenPos;
			PointToScreen_Z(carOnCrosshair->GetOrigin() + Vector3D(0,1.0f,0), screenPos, g_pGameWorld->m_viewprojection, Vector2D((float)screenSize.x,(float)screenSize.y));

			font->RenderText(L"Click to set as current", screenPos.xy(), params);
		}
	}

	char* framesStr = varargs(
		"CAMERA: &#FFFF00;%d&; / &#FFFF00;%d&;\n"
		"FRAME: &#FFFF00;%d / %d&;\n",
		replayCamera + 1, totalCameras,
		currentTick, totalTicks);

	
	font->RenderText(framesStr, frameInfoTextPos, params);
}
