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

#include "system.h"
#include "FontCache.h"

#include "KeyBinding/InputCommandBinder.h"

extern ConVar	g_pause;

ConVar			g_freecam("g_freecam", "0");
ConVar			g_freecam_speed("g_freecam_speed", "10", NULL, CV_ARCHIVE);
ConVar			g_director("director", "0");
ConVar			g_mouse_sens("g_mouse_sens", "1.0", "mouse sensitivity", CV_ARCHIVE);

ConVar			director_timeline_zoom("director_timeline_zoom", "1.0", 0.1, 10.0, "Timeline scale", CV_ARCHIVE);

int				g_nDirectorCameraType	= CAM_MODE_TRIPOD_ZOOM;

freeCameraProps_t::freeCameraProps_t()
{
	fov = DIRECTOR_DEFAULT_CAMERA_FOV;
	position = vec3_zero;
	angles = vec3_zero;
	velocity = vec3_zero;
	zAxisMove = false;
}

freeCameraProps_t g_freeCamProps;

static const wchar_t* cameraTypeStrings[] = {
	L"Outside car",
	L"In car",
	L"Tripod",
	L"Tripod (fixed zoom)",
	L"Static",
};

static const ColorRGB cameraColors[] = {
	ColorRGB(1.0f,0.25f,0.25f),
	ColorRGB(0.0f,0.25f,0.65f),
	ColorRGB(0.2f,0.7f,0.2f),
	ColorRGB(0.5f,0.2f,0.7f),
	ColorRGB(0.8f,0.8f,0.2f),
};

bool g_director_ShiftKey = false;
const float DIRECTOR_FASTFORWARD_TIMESCALE = 4.0f;

extern ConVar sys_timescale;

void Director_Enable( bool enable )
{
	g_director.SetBool(enable);
}

bool Director_IsActive()
{
	return g_director.GetBool() && g_replayData->m_state == REPL_PLAYING;
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

void Director_KeyPress(int key, bool down)
{
	if(!Director_IsActive())
		return;

	CCar* viewedCar = g_pGameSession->GetViewCar();

	if(key == KEY_SHIFT)
	{
		g_director_ShiftKey = down;
	}
	else if(key == KEY_BACKSPACE)
	{
		//sys_timescale.GetFloat();
		sys_timescale.SetFloat( down ? DIRECTOR_FASTFORWARD_TIMESCALE : 1.0f );
	}

	if(down)
	{
		//Msg("Director mode keypress: %d\n", key);

		int replayCamera = g_replayData->m_currentCamera;
		replaycamera_t* currentCamera = g_replayData->GetCurrentCamera();
		replaycamera_t* prevCamera = g_replayData->m_cameras.inRange(replayCamera-1) ? &g_replayData->m_cameras[replayCamera-1] : NULL;
		replaycamera_t* nextCamera = g_replayData->m_cameras.inRange(replayCamera+1) ? &g_replayData->m_cameras[replayCamera+1] : NULL;
		int totalTicks = g_replayData->m_numFrames;

		int highTick = nextCamera ? nextCamera->startTick : totalTicks;
		int lowTick = prevCamera ? prevCamera->startTick : 0;

		if(key == KEY_ADD)
		{
			replaycamera_t cam;

			cam.fov = g_freeCamProps.fov;
			cam.origin = g_freeCamProps.position;
			cam.rotation = g_freeCamProps.angles;
			cam.startTick = g_replayData->m_tick;
			cam.targetIdx = viewedCar->m_replayID;
			cam.type = g_nDirectorCameraType;

			int camIndex = g_replayData->AddCamera(cam);
			g_replayData->m_currentCamera = camIndex;

			// set camera after keypress
			g_freecam.SetBool(false);

			Msg("Add camera at tick %d\n", cam.startTick);
		}
		else if(key == KEY_KP_ENTER)
		{
			if(currentCamera && g_pause.GetBool())
			{
				Msg("Set camera\n");
				currentCamera->fov = g_freeCamProps.fov;
				currentCamera->origin = g_freeCamProps.position;
				currentCamera->rotation = g_freeCamProps.angles;
				currentCamera->targetIdx = viewedCar->m_replayID;
				currentCamera->type = g_nDirectorCameraType;

				g_freecam.SetBool(false);
			}
		}
		else if(key == KEY_DELETE)
		{
			if(replayCamera >= 0 && g_replayData->m_cameras.numElem())
			{
				g_replayData->m_cameras.removeIndex(replayCamera);
				g_replayData->m_currentCamera--;
			}
		}
		else if(key == KEY_SPACE)
		{
			//Msg("Add camera keyframe\n");
		}
		else if(key >= KEY_1 && key <= KEY_5)
		{
			g_nDirectorCameraType = key - KEY_1;
		}
		else if(key == KEY_PGUP)
		{
			if(g_replayData->m_cameras.inRange(replayCamera+1) && g_pause.GetBool())
				g_replayData->m_currentCamera++;
		}
		else if(key == KEY_PGDN)
		{
			if(g_replayData->m_cameras.inRange(replayCamera-1) && g_pause.GetBool())
				g_replayData->m_currentCamera--;
		}
		else if(key == KEY_LEFT)
		{
			if(currentCamera && g_pause.GetBool())
			{
				currentCamera->startTick -= g_director_ShiftKey ? 10 : 1;
				
				if(currentCamera->startTick < lowTick)
					currentCamera->startTick = lowTick;
			}
				
		}
		else if(key == KEY_RIGHT)
		{
			if(currentCamera && g_pause.GetBool())
			{
				currentCamera->startTick += g_director_ShiftKey ? 10 : 1;
				
				if(currentCamera->startTick > highTick)
					currentCamera->startTick = highTick;
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

	if(g_nClientButtons & IN_FORWARD)
		camMoveVec += f;
	else if(g_nClientButtons & IN_BACKWARD)
		camMoveVec -= f;

	if(g_nClientButtons & IN_LEFT)
		camMoveVec -= r;
	else if(g_nClientButtons & IN_RIGHT)
		camMoveVec += r;

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

DECLARE_CMD(director_pick_ray, "Director mode - picks object with ray", 0)
{
	if(!Director_IsActive())
		return;

	CCar* pickedCar = Director_GetCarOnCrosshair();

	if(pickedCar != nullptr)
	{
		g_pGameSession->SetViewCar( pickedCar );
	}
}

void Director_Draw( float fDt )
{
	if(!Director_IsActive())
		return;

	const IVector2D& screenSize = g_pHost->GetWindowSize();

	materials->Setup2D(screenSize.x,screenSize.y);

	wchar_t* controlsText = varargs_w(
		L"PLAY = &#FFFF00;O&;\n"
		L"TOGGLE FREE CAMERA = &#FFFF00;F&;\n\n"

		L"NEXT CAMERA = &#FFFF00;PAGE UP&;\n"
		L"PREV CAMERA = &#FFFF00;PAGE DOWN&;\n\n"

		L"INSERT NEW CAMERA = &#FFFF00;KP_PLUS&;\n"
		L"UPDATE CAMERA = &#FFFF00;KP_ENTER&;\n"
		L"DELETE CAMERA = &#FFFF00;DEL&;\n"

		L"MOVE CAMERA START = &#FFFF00;LEFT ARROW&; and &#FFFF00;RIGHT ARROW&;\n\n"

		//L"SET CAMERA KEYFRAME = &#FFFF00;SPACE&;\n\n"

		L"CAMERA TYPE = &#FFFF00;1-5&; (Current is &#FFFF00;'%s'&;)\n"
		L"CAMERA ZOOM = &#FFFF00;MOUSE WHEEL&; (%.2f deg.)\n"
		L"TARGET VEHICLE = &#FFFF00;LEFT MOUSE CLICK ON OBJECT&;\n"
		
		L"SEEK = &#FFFF00;fastseek <frame>&; (in console)\n", cameraTypeStrings[g_nDirectorCameraType], g_freeCamProps.fov);

	wchar_t* shortText =	L"PAUSE = &#FFFF00;O&;\n"
							L"TOGGLE FREE CAMERA = &#FFFF00;F&;\n"
							L"FAST FORWARD 4x = &#FFFF00;BACKSPACE&;\n";

	eqFontStyleParam_t params;
	params.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_USE_TAGS;
	params.textColor = color4_white;

	Vector2D directorTextPos(15, screenSize.y/3);

	if(g_pause.GetBool())
		g_pHost->GetDefaultFont()->RenderText(controlsText, directorTextPos, params);
	else
		g_pHost->GetDefaultFont()->RenderText(shortText, directorTextPos, params);

	replaycamera_t* currentCamera = g_replayData->GetCurrentCamera();
	int replayCamera = g_replayData->m_currentCamera;
	int currentTick = g_replayData->m_tick;
	int totalTicks = g_replayData->m_numFrames;

	int totalCameras = g_replayData->m_cameras.numElem();

	wchar_t* framesStr = varargs_w(L"FRAME: &#FFFF00;%d / %d&;\nCAMERA: &#FFFF00;%d&; (frame %d) / &#FFFF00;%d&;", currentTick, totalTicks, replayCamera+1, currentCamera ? currentCamera->startTick : 0, totalCameras);

	Rectangle_t timelineRect(0,screenSize.y-100, screenSize.x, screenSize.y-70);
	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	g_pShaderAPI->SetTexture(0,0,0);
	materials->SetRasterizerStates(CULL_BACK);
	materials->SetDepthStates(false,false);
	materials->SetBlendingStates(blending);
	materials->BindMaterial(materials->GetDefaultMaterial());

	const float pixelsPerTick = 1.0f / 4.0f * director_timeline_zoom.GetFloat();
	const float currentTickOffset = currentTick*pixelsPerTick;
	const float lastTickOffset = totalTicks*pixelsPerTick;

	float timelineCenterPos = timelineRect.GetCenter().x;

	CCar* viewedCar = g_pGameSession->GetViewCar();

	// if car is no longer valid, resetthe viewed car
	if(!g_pGameWorld->IsValidObject(viewedCar))
	{
		g_pGameSession->SetViewCar(NULL);
		viewedCar = g_pGameSession->GetViewCar();
	}

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		float ticksOffset = lastTickOffset-currentTickOffset;

		Rectangle_t drawnTimeline(timelineRect.GetCenter().x - currentTickOffset, screenSize.y-100.0f, timelineRect.GetCenter().x + ticksOffset , screenSize.y-70.0f);
		drawnTimeline.vleftTop.x = clamp(drawnTimeline.vleftTop.x,0.0f,timelineRect.vrightBottom.x);
		drawnTimeline.vrightBottom.x = clamp(drawnTimeline.vrightBottom.x,0.0f,timelineRect.vrightBottom.x);

		meshBuilder.Color4f(1,1,1, 0.25f);
		meshBuilder.Quad2(drawnTimeline.GetLeftTop(), drawnTimeline.GetRightTop(), drawnTimeline.GetLeftBottom(), drawnTimeline.GetRightBottom());

		for(int i = 0; i < totalCameras; i++)
		{
			replaycamera_t* camera = &g_replayData->m_cameras[i];

			float cameraTickPos = (camera->startTick-currentTick) * pixelsPerTick;

			replaycamera_t* nextCamera = i+1 <g_replayData->m_cameras.numElem() ? &g_replayData->m_cameras[i+1] : NULL;
			float nextTickPos = ((nextCamera ? nextCamera->startTick : totalTicks)-currentTick) * pixelsPerTick;

			// draw colored rectangle
			Rectangle_t cameraColorRect(timelineRect.GetCenter().x + cameraTickPos, screenSize.y-95.0f, timelineRect.GetCenter().x + nextTickPos, screenSize.y-75.0f);

			ColorRGB camRectColor(cameraColors[camera->type]);

			if(currentCamera == camera && g_pause.GetBool())
			{
				camRectColor *= fabs(sinf((float)g_pHost->GetCurTime()*2.0f));

				// draw start tick position
				Rectangle_t currentTickRect(timelineRect.GetCenter() - Vector2D(2, 25) + Vector2D(cameraTickPos,0), timelineRect.GetCenter() + Vector2D(2, 0) + Vector2D(cameraTickPos,0));
				meshBuilder.Color4f(1.0f,0.0f,0.0f,0.8f);
				meshBuilder.Quad2(currentTickRect.GetLeftTop(), currentTickRect.GetRightTop(), currentTickRect.GetLeftBottom(), currentTickRect.GetRightBottom());
			}

			meshBuilder.Color4fv(ColorRGBA(camRectColor, 0.7f));
			meshBuilder.Quad2(cameraColorRect.GetLeftTop(), cameraColorRect.GetRightTop(), cameraColorRect.GetLeftBottom(), cameraColorRect.GetRightBottom());

			// draw start tick position
			Rectangle_t currentTickRect(timelineRect.GetCenter() - Vector2D(2, 15) + Vector2D(cameraTickPos,0), timelineRect.GetCenter() + Vector2D(2, 15) + Vector2D(cameraTickPos,0));
			meshBuilder.Color4f(0.9f,0.9f,0.9f,0.8f);
			meshBuilder.Quad2(currentTickRect.GetLeftTop(), currentTickRect.GetRightTop(), currentTickRect.GetLeftBottom(), currentTickRect.GetRightBottom());
		}

		if(g_freecam.GetBool())
		{
			if(viewedCar)
			{
				Vector3D screenPos;
				PointToScreen_Z(viewedCar->GetOrigin() + Vector3D(0,1.0f,0), screenPos, g_pGameWorld->m_viewprojection, Vector2D((float)screenSize.x,(float)screenSize.y));

				meshBuilder.Color4f( 0.0f, 0.7f, 0.0f, 0.5f );

				if(screenPos.z > 0.0f)
					meshBuilder.Triangle2(screenPos.xy(), screenPos.xy()+Vector2D(-20, -20), screenPos.xy()+Vector2D(20, -20));
			}
		}

		// current tick
		Rectangle_t currentTickRect(timelineRect.GetCenter() - Vector2D(2, 20), timelineRect.GetCenter() + Vector2D(2, 20));
		meshBuilder.Color4f(0,0,0,1.0f);
		meshBuilder.Quad2(currentTickRect.GetLeftTop(), currentTickRect.GetRightTop(), currentTickRect.GetLeftBottom(), currentTickRect.GetRightBottom());

		// end tick
		Rectangle_t lastTickRect(timelineRect.GetCenter() - Vector2D(2, 20) + Vector2D(ticksOffset,0), timelineRect.GetCenter() + Vector2D(2, 20) + Vector2D(ticksOffset,0));
		meshBuilder.Color4f(1,0.05f,0,1.0f);
		meshBuilder.Quad2(lastTickRect.GetLeftTop(), lastTickRect.GetRightTop(), lastTickRect.GetLeftBottom(), lastTickRect.GetRightBottom());

	meshBuilder.End();

	params.align = TEXT_ALIGN_HCENTER;

	if(g_freecam.GetBool())
	{
		CCar* carOnCrosshair = Director_GetCarOnCrosshair();

		if(carOnCrosshair && carOnCrosshair != viewedCar)
		{
			Vector3D screenPos;
			PointToScreen_Z(carOnCrosshair->GetOrigin() + Vector3D(0,1.0f,0), screenPos, g_pGameWorld->m_viewprojection, Vector2D((float)screenSize.x,(float)screenSize.y));

			g_pHost->GetDefaultFont()->RenderText(L"Click to set as current", screenPos.xy(), params);
		}
	}

	Vector2D frameInfoTextPos(screenSize.x/2, screenSize.y - (screenSize.y/6));
	g_pHost->GetDefaultFont()->RenderText(framesStr, frameInfoTextPos, params);

	if(g_freecam.GetBool())
	{
		Vector2D halfScreen = Vector2D(screenSize)*0.5f;

		Vertex2D_t tmprect[] =
		{
			Vertex2D_t(halfScreen+Vector2D(0,-3), vec2_zero),
			Vertex2D_t(halfScreen+Vector2D(3,3), vec2_zero),
			Vertex2D_t(halfScreen+Vector2D(-3,3), vec2_zero)
		};

		// Draw crosshair
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLES, tmprect, elementsOf(tmprect), NULL, ColorRGBA(1,1,1,0.45));
	}
}