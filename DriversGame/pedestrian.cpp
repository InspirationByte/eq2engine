//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Pedestrian
//////////////////////////////////////////////////////////////////////////////////

#include "pedestrian.h"
#include "CameraAnimator.h"
#include "input.h"
#include "world.h"

#include "Shiny.h"

//-----------------------------------------------------------------------
// Jack tribute
//-----------------------------------------------------------------------
#define JACK_PED_MODEL "models/characters/jack.egf"

ConVar g_jack("g_jack", "0", nullptr, CV_CHEAT);

extern CPFXAtlasGroup* g_jackPed;
extern CPFXAtlasGroup* g_vehicleEffects;

spritePedSegment_t g_spritePedSegments[13] = {
	{7, 8, 0.07f, 0.08f, 1}, // L upperarm
	{8, 9, 0.05f, 0.07f, 2}, // L forearm
	{9, 9, 0.05f, 0.1f, 2}, // L hand

	{11, 12, 0.07f, 0.08f, 1}, // R upperarm
	{12, 13, 0.05f, 0.07f, 2}, // R forearm
	{13, 13, 0.05f, 0.1f, 2}, // R hand

	{14, 15, 0.12f, 0.12f, 3}, // L thigh
	{15, 16, 0.12f, 0.05f, 3}, // L leg
	{16, 17, 0.07f, 0.08f, 0}, // L foot

	{18, 19, 0.12f, 0.12f, 3}, // R thigh
	{19, 20, 0.12f, 0.05f, 3}, // R leg
	{20, 21, 0.07f, 0.08f, 0}, // R foot

	{4, 0, 0.21f, 0.0f, 0},	// chest
};

// for Jack rendering
void DrawJackParticle(const Vector3D& from, const Vector3D& to, float width, TexAtlasEntry_t* atlEntry)
{
	PFXVertex_t* verts;
	if (g_jackPed->AllocateGeom(4, 4, &verts, NULL, true) < 0)
		return;

	Vector3D viewDir = from - g_pGameWorld->m_view.GetOrigin();
	Vector3D lineDir = normalize(from - to);

	Vector3D ccross = fastNormalize(cross(lineDir, viewDir));

	Vector3D temp;

	VectorMA(to, width, ccross, temp);

	ColorRGBA ambient = ColorRGBA(g_pGameWorld->m_envConfig.ambientColor + g_pGameWorld->m_envConfig.sunColor + g_pGameWorld->m_envConfig.moonBrightness*0.15f, 1);

	verts[0].point = temp;
	verts[0].texcoord = atlEntry->rect.GetLeftBottom();
	verts[0].color = ambient;

	VectorMA(to, -width, ccross, temp);

	verts[1].point = temp;
	verts[1].texcoord = atlEntry->rect.GetRightBottom();
	verts[1].color = ambient;

	VectorMA(from, width, ccross, temp);

	verts[2].point = temp;
	verts[2].texcoord = atlEntry->rect.GetLeftTop();
	verts[2].color = ambient;

	VectorMA(from, -width, ccross, temp);

	verts[3].point = temp;
	verts[3].texcoord = atlEntry->rect.GetRightTop();
	verts[3].color = ambient;
}

//-----------------------------------------------------------------------

CPedestrian::CPedestrian() : CAnimatingEGF(), CControllableGameObject(), m_thinker(this), m_physObj(nullptr)
{
	m_pedState = 0;

	m_thinkTime = 0;
	m_hasAI = false;
	m_jack = true;

	m_pedSteerAngle = 0.0f;

	m_drawFlags |= GO_DRAW_FLAG_SHADOW;
}

CPedestrian::CPedestrian(pedestrianConfig_t* config) : CPedestrian()
{
	SetModel(config->model.c_str());
	m_hasAI = config->hasAI;

	// we're not forcing to spawn them all as Jack, but instead adding a chance
	m_jack = g_jack.GetBool() && RandomInt(0, 100) > 99;
}

CPedestrian::~CPedestrian()
{

}

void CPedestrian::Precache()
{
	if (m_jack)
		PrecacheStudioModel(JACK_PED_MODEL);
}

void CPedestrian::SetModelPtr(IEqModel* modelPtr)
{
	DestroyAnimating();

	BaseClass::SetModelPtr(modelPtr);

	InitAnimating(m_pModel);
}

void CPedestrian::OnRemove()
{
	DestroyAnimating();

	g_pPhysics->RemoveObject(m_physObj);
	m_physObj = nullptr;

	BaseClass::OnRemove();
}

void CPedestrian::OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy)
{
	BaseClass::OnCarCollisionEvent(pair, hitBy);
}

const float PEDESTRIAN_PHYSICS_RADIUS = 0.85f;

void CPedestrian::Spawn()
{
	CEqRigidBody* body = new CEqRigidBody();
	body->Initialize(PEDESTRIAN_PHYSICS_RADIUS);

	body->SetCollideMask(COLLIDEMASK_PEDESTRIAN);
	body->SetContents(OBJECTCONTENTS_PEDESTRIAN);

	body->SetPosition(m_vecOrigin);

	body->m_flags |= BODY_DISABLE_DAMPING | COLLOBJ_DISABLE_RESPONSE | BODY_FROZEN;

	body->SetMass(85.0f);
	body->SetFriction(0.0f);
	body->SetRestitution(0.0f);
	body->SetAngularFactor(vec3_zero);
	body->m_erp = 0.15f;
	body->SetGravity(18.0f);
	
	body->SetUserData(this);

	m_physObj = new CPhysicsHFObject(body, this);
	g_pPhysics->AddObject(m_physObj);

	if(m_hasAI)
		m_thinker.FSMSetState(AI_State(&CPedestrianAI::SearchDaWay));

	if (m_jack)
		SetModel(JACK_PED_MODEL);

	BaseClass::Spawn();
}

void CPedestrian::ConfigureCamera(cameraConfig_t& conf, eqPhysCollisionFilter& filter) const
{
	conf.dist = 3.8f;
	conf.height = 1.0f;
	conf.distInCar = 0.0f;
	conf.widthInCar = 0.0f;
	conf.heightInCar = 0.72f;
	conf.fov = 60.0f;

	filter.AddObject(m_physObj->GetBody());
}

void CPedestrian::Draw(int nRenderFlags)
{
	CEqRigidBody* physBody = m_physObj->GetBody();

	physBody->UpdateBoundingBoxTransform();
	m_bbox = physBody->m_aabb_transformed;

	RecalcBoneTransforms();

	if (m_jack)
	{
		if (!g_jackPed)
		{
			g_jackPed = new CPFXAtlasGroup();
			g_jackPed->Init("models/characters/jack", false, 4096);
			g_pPFXRenderer->AddRenderGroup(g_jackPed, g_vehicleEffects);
		}

		for (int i = 0; i < 13; i++)
		{
			spritePedSegment_t& seg = g_spritePedSegments[i];

			Vector3D fromPos = (m_worldMatrix*Vector4D(m_boneTransforms[seg.fromJoint].rows[3].xyz(), 1.0f)).xyz();
			Vector3D toPos = (m_worldMatrix*Vector4D(m_boneTransforms[seg.toJoint].rows[3].xyz(), 1.0f)).xyz();
			Vector3D toDir = transpose(m_worldMatrix.getRotationComponent() * m_boneTransforms[seg.fromJoint].getRotationComponent()).rows[1];

			//debugoverlay->Line3D(fromPos, toPos, ColorRGBA(1,1,1,1), ColorRGBA(1, 1, 1, 1), 0.0f);
			//debugoverlay->Line3D(toPos, toPos+toDir, ColorRGBA(0, 0, 1, 1), ColorRGBA(0, 0, 1, 1), 0.0f);

			DrawJackParticle(fromPos, toPos + toDir * seg.addLength, seg.width, g_jackPed->GetEntry(seg.atlasIdx));
		}
	}

	DrawEGF(nRenderFlags, m_boneTransforms);

	m_shadowDecal.dirty = true;
}

const float ACCEL_RATIO = 12.0f;
const float DECEL_RATIO = 25.0f;

const float MAX_WALK_VELOCITY = 1.35f;
const float MAX_RUN_VELOCITY = 9.0f;

const float PEDESTRIAN_THINK_TIME = 0.0f;

void CPedestrian::OnPhysicsFrame(float fDt)
{
	PROFILE_FUNC();

	CEqRigidBody* physBody = m_physObj->GetBody();

	Vector3D velocity = physBody->GetLinearVelocity();

	Vector3D preferredMove(0.0f);

	Vector3D forwardVec;
	AngleVectors(GetAngles(), &forwardVec);

	// do pedestrian thinker
	if (m_hasAI)
	{
		PROFILE_BEGIN(DoStatesAndEvents);
		int res = m_thinker.DoStatesAndEvents(fDt);
		PROFILE_END();
	}

	int controlButtons = GetControlButtons();

	Activity bestMoveActivity = (controlButtons & IN_BURNOUT) ? ACT_RUN : ACT_WALK;
	float bestMaxSpeed = (bestMoveActivity == ACT_RUN) ? MAX_RUN_VELOCITY : MAX_WALK_VELOCITY;


	if (fDt > 0.0f)
	{
		const float bestMaxSpeedSqr = bestMaxSpeed * bestMaxSpeed;

		if (lengthSqr(velocity) < bestMaxSpeedSqr)
		{
			if (controlButtons & IN_ACCELERATE)
				preferredMove += forwardVec * ACCEL_RATIO;
			else if (controlButtons & IN_BRAKE)
				preferredMove -= forwardVec * ACCEL_RATIO;
		}

		{
			preferredMove.x -= (velocity.x - preferredMove.x) * DECEL_RATIO;
			preferredMove.z -= (velocity.z - preferredMove.z) * DECEL_RATIO;
		}

		physBody->ApplyLinearForce(preferredMove * physBody->GetMass());
	}

	if (controlButtons)
		physBody->TryWake(false);

	Activity idleAct = ACT_IDLE;
	int pedState = m_pedState;

	if (pedState == 1)
	{
		idleAct = ACT_IDLE_WPN;
	}
	else if (pedState == 2)
	{
		idleAct = ACT_IDLE_CROUCH;
	}

	Activity currentAct = GetCurrentActivity();

	if (currentAct != idleAct)
		SetPlaybackSpeedScale(length(velocity) / bestMaxSpeed);
	else
		SetPlaybackSpeedScale(1.0f);

	if (length(velocity.xz()) > 0.5f)
	{
		if (currentAct != bestMoveActivity)
			SetActivity(bestMoveActivity);
	}
	else if (currentAct != idleAct)
		SetActivity(idleAct);
}

void CPedestrian::Simulate(float fDt)
{
	int controlButtons = GetControlButtons();

	if (controlButtons & IN_TURNLEFT)
		m_pedSteerAngle.y += 120.0f * fDt;
	if (controlButtons & IN_TURNRIGHT)
		m_pedSteerAngle.y -= 120.0f * fDt;

	if (controlButtons & IN_BURNOUT)
		m_pedState = 1;
	else if (controlButtons & IN_EXTENDTURN)
		m_pedState = 2;
	else if (controlButtons & IN_HANDBRAKE)
		m_pedState = 0;

	AdvanceFrame(fDt);
	DebugRender(m_worldMatrix);

	BaseClass::Simulate(fDt);
}

void CPedestrian::UpdateTransform()
{
	Vector3D offset(vec3_up*PEDESTRIAN_PHYSICS_RADIUS);

	// refresh it's matrix
	m_worldMatrix = translate(m_vecOrigin - offset)*rotateY4(DEG2RAD(m_pedSteerAngle.y));
}

void CPedestrian::SetOrigin(const Vector3D& origin)
{
	if (m_physObj)
		m_physObj->GetBody()->SetPosition(origin);

	m_vecOrigin = origin;
}

void CPedestrian::SetAngles(const Vector3D& angles)
{
	m_pedSteerAngle = angles;
}

void CPedestrian::SetVelocity(const Vector3D& vel)
{
	if (m_physObj)
		m_physObj->GetBody()->SetLinearVelocity(vel);
}

const Vector3D& CPedestrian::GetVelocity() const
{
	if (m_physObj)
		return m_physObj->GetBody()->GetLinearVelocity();

	return vec3_zero;
}

const Vector3D& CPedestrian::GetAngles() const
{
	return m_pedSteerAngle;
}

void CPedestrian::HandleAnimatingEvent(AnimationEvent nEvent, char* options)
{

}

//--------------------------------------------------------------
// PEDESTRIAN THINKER FSM

bool CPedestrianAI::GetNextPath(int dir)
{
	// get the road tile

	CLevelRegion* reg;
	levroadcell_t* cell = g_pGameWorld->m_level.Road_GetGlobalTile(m_host->GetOrigin(), &reg);

	if (cell && cell->type == ROADTYPE_PAVEMENT)
	{
		IVector2D curTile;
		g_pGameWorld->m_level.LocalToGlobalPoint(IVector2D(cell->posX, cell->posY), reg, curTile);

		int tileOfsX[] = ROADNEIGHBOUR_OFFS_X(curTile.x);
		int tileOfsY[] = ROADNEIGHBOUR_OFFS_Y(curTile.y);

		// try to walk in usual dae way
		{
			levroadcell_t* nCell = g_pGameWorld->m_level.Road_GetGlobalTileAt(IVector2D(tileOfsX[dir], tileOfsY[dir]), &reg);

			if (nCell && nCell->type == ROADTYPE_PAVEMENT)
			{
				IVector2D nTile;
				g_pGameWorld->m_level.LocalToGlobalPoint(IVector2D(nCell->posX, nCell->posY), reg, nTile);

				if (nCell != m_prevRoadCell && nCell != m_nextRoadCell)
				{
					m_prevRoadCell = m_nextRoadCell;
					m_nextRoadCell = nCell;
					m_nextRoadTile = nTile;
					m_prevDir = m_curDir;
					m_curDir = dir;
					return true;
				}
			}
		}
	}
	else
	{
		m_nextRoadTile = 0;
	}

	return false;
}

int	CPedestrianAI::SearchDaWay(float fDt, EStateTransition transition)
{
	if (transition == EStateTransition::STATE_TRANSITION_PROLOG)
	{
		return 0;
	}
	else if (transition == EStateTransition::STATE_TRANSITION_EPILOG)
	{
		return 0;
	}

	int randomDir = RandomInt(0, 3);

	if (GetNextPath(randomDir))
		AI_SetState(&CPedestrianAI::DoWalk);

	return 0;
}

const float AI_PEDESTRIAN_CAR_AFRAID_MAX_RADIUS = 9.0f;
const float AI_PEDESTRIAN_CAR_AFRAID_MIN_RADIUS = 2.0f;
const float AI_PEDESTRIAN_CAR_AFRAID_STRAIGHT_RADIUS = 2.5f;
const float AI_PEDESTRIAN_CAR_AFRAID_VELOCITY = 1.0f;

void CPedestrianAI::DetectEscape()
{
	Vector3D pedPos = m_host->GetOrigin();

	DkList<CGameObject*> nearestCars;
	g_pGameWorld->QueryObjects(nearestCars, AI_PEDESTRIAN_CAR_AFRAID_MAX_RADIUS, pedPos, [](CGameObject* x) {
		int objType = x->ObjType();
		return (objType == GO_CAR || objType == GO_CAR_AI);
	});

	int numCars = nearestCars.numElem();
	for (int i = 0; i < numCars; i++)
	{
		CControllableGameObject* nearCar = (CControllableGameObject*)nearestCars[i];

		Vector3D carPos = nearCar->GetOrigin();
		Vector3D carVel = nearCar->GetVelocity();
		Vector3D carHeadingPos = carPos + carVel;

		float projResult = lineProjection(carPos, carHeadingPos, pedPos);

		float velocity = length(carVel);
		float dist = length(carPos - pedPos);

		bool hasSirenOrHorn = nearCar->GetControlButtons() & IN_HORN;
		bool tooNearToCar = (dist < AI_PEDESTRIAN_CAR_AFRAID_MIN_RADIUS);

		if (projResult > 0.0f && projResult < 1.0f && (velocity > AI_PEDESTRIAN_CAR_AFRAID_VELOCITY) || 
			hasSirenOrHorn || 
			tooNearToCar)
		{
			if (tooNearToCar)
			{
				m_escapeFromPos = pedPos;
				m_escapeDir = fastNormalize(pedPos - carPos);

				AI_SetState(&CPedestrianAI::DoEscape);
			}
			else
			{
				Vector3D projPos = lerp(carPos, carHeadingPos, projResult);

				if (hasSirenOrHorn || length(projPos - pedPos) < AI_PEDESTRIAN_CAR_AFRAID_STRAIGHT_RADIUS)
				{
					m_escapeFromPos = pedPos;
					m_escapeDir = fastNormalize(pedPos - projPos);

					AI_SetState(&CPedestrianAI::DoEscape);

					return;
				}
			}
		}
	}
}

int	CPedestrianAI::DoEscape(float fDt, EStateTransition transition)
{
	if (transition == EStateTransition::STATE_TRANSITION_PROLOG)
	{
		return 0;
	}
	else if (transition == EStateTransition::STATE_TRANSITION_EPILOG)
	{
		return 0;
	}

	DetectEscape();

	Vector3D pedPos = m_host->GetOrigin();
	Vector3D pedAngles = m_host->GetAngles();

	if (length(pedPos - m_escapeFromPos) > AI_PEDESTRIAN_CAR_AFRAID_MIN_RADIUS)
	{
		AI_SetState(&CPedestrianAI::DoWalk);
		return 0;
	}

	int controlButtons = 0;

	Vector3D dirAngles = VectorAngles(m_escapeDir);
	m_host->SetAngles(Vector3D(0.0f, dirAngles.y, 0.0f));
	/*
	float angleDiff = AngleDiff(pedAngles.y, dirAngles.y);

	if (fabs(angleDiff) > 1.0f)
	{
		if (angleDiff > 0)
			controlButtons |= IN_TURNLEFT;
		else
			controlButtons |= IN_TURNRIGHT;
	}
	*/
	controlButtons |= IN_ACCELERATE | IN_BURNOUT;

	m_host->SetControlButtons(controlButtons);

	return 0;
}

int	CPedestrianAI::DoWalk(float fDt, EStateTransition transition)
{
	if (transition == EStateTransition::STATE_TRANSITION_PROLOG)
	{
		return 0;
	}
	else if (transition == EStateTransition::STATE_TRANSITION_EPILOG)
	{
		return 0;
	}
	
	Vector3D pedPos = m_host->GetOrigin();
	Vector3D pedAngles = m_host->GetAngles();

	DetectEscape();

	CLevelRegion* reg;
	levroadcell_t* cell = g_pGameWorld->m_level.Road_GetGlobalTile(pedPos, &reg);

	if (!m_nextRoadCell || m_nextRoadCell == cell)
	{
		if (!GetNextPath(m_curDir))
		{
			AI_SetState(&CPedestrianAI::SearchDaWay);
			return 0;
		}
	}

	
	int controlButtons = 0;

	Vector3D nextCellPos = g_pGameWorld->m_level.GlobalTilePointToPosition(m_nextRoadTile);

	Vector3D dirToCell = normalize(nextCellPos - pedPos);
	Vector3D dirAngles = VectorAngles(dirToCell);

	float angleDiff = AngleDiff(pedAngles.y, dirAngles.y);

	if (fabs(angleDiff) > 1.0f)
	{
		if (angleDiff > 0)
			controlButtons |= IN_TURNLEFT;
		else
			controlButtons |= IN_TURNRIGHT;
	}
	else
		controlButtons |= IN_ACCELERATE;// | IN_BURNOUT;


	m_host->SetControlButtons(controlButtons);


	return 0;
}