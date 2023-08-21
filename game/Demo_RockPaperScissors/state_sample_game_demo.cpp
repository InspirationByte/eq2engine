#include "core/core_common.h"
#include "core/ConVar.h"
#include "math/Random.h"
#include "math/Utility.h"
#include "utils/KeyValues.h"
#include "utils/TextureAtlas.h"
#include "utils/SpiralIterator.h"

#include "state_sample_game_demo.h"

#include "sys/sys_host.h"

#include "audio/eqSoundEmitterSystem.h"
#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

#include "render/EqParticles.h"
#include "render/IDebugOverlay.h"
#include "input/in_keys_ident.h"

#include "movie/MoviePlayer.h"

static constexpr const float s_spriteSize = 50.0f;
static constexpr const float s_radiusSqr = s_spriteSize * s_spriteSize;
static constexpr const float s_moveSpeed = 80000.0f;

static const char* s_soundNames[] = {
	"effect.rock",
	"effect.paper",
	"effect.scissors",
};

static int CheckWhoDefeats(const RPSType& a, const RPSType& b)
{
	if (a == b)
		return 0;

	// GPT3 helped to optimize commented code below!
	const int diff = (static_cast<int>(a) - static_cast<int>(b) + 3) % 3;
	if (diff == 1)
		return 1; // a wins
	else
		return 2; // b wins

	/*
	// rock break scissors
	if (a.type == RPSType::ROCK && b.type == RPSType::SCISSORS)
		return 1;
	else if (b.type == RPSType::ROCK && a.type == RPSType::SCISSORS)
		return 2;

	// paper covers rock
	if (a.type == RPSType::PAPER && b.type == RPSType::ROCK)
		return 1;
	else if (b.type == RPSType::PAPER && a.type == RPSType::ROCK)
		return 2;

	// scissors cut paper
	if (a.type == RPSType::SCISSORS && b.type == RPSType::PAPER)
		return 1;
	else if (b.type == RPSType::SCISSORS && a.type == RPSType::PAPER)
		return 2;

	return 0; // WTF?
	*/
}

template<typename T>
struct ECSComponent
{
	// TODO:	Thread safety
	//			More efficient component lookup
	//			Non-static storage

	static T* Get(Map<int, int>::Iterator& iterator)
	{
		return &pool[*iterator];
	}

	static T* Get(int entity)
	{
		auto it = entityComponent.find(entity);
		if (it.atEnd())
			return nullptr;
		return &pool[*it];
	}

	static T* GetOrCreate(int entity)
	{
		auto it = entityComponent.find(entity);
		if (it.atEnd())
		{
			if (freeSlots.numElem())
			{
				const int poolIdx = freeSlots.popBack();
				it = entityComponent.insert(entity, poolIdx);
				pool[poolIdx] = T();
			}
			else
			{
				it = entityComponent.insert(entity, pool.numElem());
				pool.append();
			}
		}
		return &pool[*it];
	}

	template<typename ...Params>
	static T* GetOrCreate(int entity, Params&&... args)
	{
		auto it = entityComponent.find(entity);
		if (it.atEnd())
		{
			if (freeSlots.numElem())
			{
				const int poolIdx = freeSlots.popBack();
				it = entityComponent.insert(entity, poolIdx);
				pool[poolIdx] = T(std::forward<Params>(args)...);
			}
			else
			{
				it = entityComponent.insert(entity, pool.numElem());
				pool.append(T(std::forward<Params>(args)...));
			}
		}
		return &pool[*it];
	}

	void Remove(int entity)
	{
		auto it = entityComponent.find(entity);
		if (it.atEnd())
			return;
		freeSlots.append(it.value());
		entityComponent.remove(it);
	}

	// FIXME: this could be slow, use different data structures to search instead?
	inline static Map<int, int>	entityComponent{ PP_SL };
	inline static Array<T>		pool{ PP_SL };
	inline static Array<int>	freeSlots{ PP_SL };
};

template<typename T>
struct ECSSystem
{
	using Sys = T;
	static void Process(T& system);
};

template<typename T>
struct ECSMessage
{
	int fromEntity{ -1 };

	inline static Array<T>	messages{ PP_SL };
};

//---------------------------------------------
/*
struct EffectPosition
{
	Vector3D point{ vec3_zero };
};

struct EffectVelocity
{
	EffectVelocity() = default;
	EffectVelocity(const Vector3D& initialVelocity) : velocity(initialVelocity) {}
	Vector3D velocity{ vec3_zero };
};

struct ParticlesMovement
{
	// Components used by the system goes here
	using Position = ECSComponent<EffectPosition>;
	using Velocity = ECSComponent<EffectVelocity>;
};

void ECSSystem<ParticlesMovement>::Process(int entity)
{
	auto pos = Sys::Position::Get(entity);
	auto vel = Sys::Velocity::GetOrCreate(entity, vec3_zero);
	pos->point += vel->velocity;
}
*/

struct Position
{
	Position() = default;
	Position(Vector2D& initPos) : pos(initPos){}

	Vector2D pos{ vec2_zero };
};

struct State
{
	State() = default;
	State(RPSType initType) : type(initType) {}

	RPSType type{ RPSType::ROCK };
};

struct Movement
{
	// alias for components used by system
	using Position = ECSComponent<Position>;
	using State = ECSComponent<State>;

	// system arguments and feedback
	float deltaTime{ 0.0f };
};

struct StateUpdate
{
	struct DefeatMessage : public ECSMessage<DefeatMessage> 
	{
		int defeatedBy{ -1 };
	};

	// alias for components used by system
	using Position = ECSComponent<Position>;
	using State = ECSComponent<State>;

	// system arguments and feedback
	int counts[static_cast<int>(RPSType::COUNT)]{ 0 };
};

struct Draw
{
	// alias for components used by system
	using Position = ECSComponent<Position>;
	using State = ECSComponent<State>;

	// system arguments and feedback
	const AtlasEntry**	atlEntries{ nullptr };
	CParticleBatch*		pfxGroup{ nullptr };
};

void ECSSystem<Movement>::Process(Movement& sysState)
{
	const float deltaTime = sysState.deltaTime;

	for (auto it = Sys::Position::entityComponent.begin(); !it.atEnd(); ++it)
	{
		Position& curPos = *Sys::Position::Get(it);
		State& state = *Sys::State::Get(*it);

		Vector2D moveVector(0.0f);
		float criticalMass = 1.0f;

		for (auto otherIt = Sys::Position::entityComponent.begin(); !otherIt.atEnd(); ++otherIt)
		{
			if (*it == *otherIt)
				continue;

			Position& otherPos = *Sys::Position::Get(*otherIt);
			State& otherState = *Sys::State::Get(*otherIt);

			const int whoDefeats = CheckWhoDefeats(state.type, otherState.type);
			const float distSqr = distanceSqr(curPos.pos, otherPos.pos);

			if (whoDefeats == 1)
				moveVector += normalize(otherPos.pos - curPos.pos) / distSqr;
			else if (whoDefeats == 2)
				moveVector -= normalize(otherPos.pos - curPos.pos) / distSqr * 0.25f;
			else
				criticalMass += rsqrtf(distSqr);

			if (distSqr < s_radiusSqr)
				StateUpdate::DefeatMessage::messages.append({ *it, *otherIt });
		}

		curPos.pos += moveVector * s_moveSpeed * criticalMass * deltaTime;
	}
}

void ECSSystem<StateUpdate>::Process(StateUpdate& sysState)
{
	// NOTE: concept
	// use more generalized solution that does not rely on array cleanup
	for (auto& message : StateUpdate::DefeatMessage::messages)
	{
		State& state = *Sys::State::Get(message.fromEntity);
		State& otherState = *Sys::State::Get(message.defeatedBy);
		const int whoDefeats = CheckWhoDefeats(state.type, otherState.type);

		// turn loser into winner type and play winner sound
		if (whoDefeats == 1) // a beaten b
		{
			Position& curPos = *Sys::Position::Get(message.fromEntity);
			otherState.type = state.type;

			EmitParams ep(s_soundNames[static_cast<int>(state.type)]);
			ep.origin.x = curPos.pos.x * 0.01f;
			g_sounds->EmitSound(&ep);
		}
		else if (whoDefeats == 2) // b beaten a
		{
			Position& otherPos = *Sys::Position::Get(message.defeatedBy);
			state.type = otherState.type;

			EmitParams ep(s_soundNames[static_cast<int>(otherState.type)]);
			ep.origin.x = otherPos.pos.x * 0.01f;
			g_sounds->EmitSound(&ep);
		}
	}
	StateUpdate::DefeatMessage::messages.clear();

	for (auto it = Sys::State::entityComponent.begin(); !it.atEnd(); ++it)
	{
		State& state = *Sys::State::Get(it);
		++sysState.counts[static_cast<int>(state.type)];
	}
}

void ECSSystem<Draw>::Process(Draw& sysState)
{
	for (auto it = Sys::Position::entityComponent.begin(); !it.atEnd(); ++it)
	{
		Position& curPos = *Sys::Position::Get(it);
		State& state = *Sys::State::Get(*it);

		// draw
		const AARectangle rect = sysState.atlEntries[static_cast<int>(state.type)]->rect;

		PFXVertex_t* verts;
		if (sysState.pfxGroup->AllocateGeom(4, 4, &verts, nullptr, true) == -1)
			break;

		verts[0].point = Vector3D(0, 0, 0);
		verts[1].point = Vector3D(0, s_spriteSize, 0);
		verts[2].point = Vector3D(s_spriteSize, 0, 0);
		verts[3].point = Vector3D(s_spriteSize, s_spriteSize, 0);

		verts[0].texcoord = rect.GetLeftTop();
		verts[1].texcoord = rect.GetLeftBottom();
		verts[2].texcoord = rect.GetRightTop();
		verts[3].texcoord = rect.GetRightBottom();

		const Vector3D transform = Vector3D(curPos.pos, 0.0f);

		verts[0].point += transform;
		verts[1].point += transform;
		verts[2].point += transform;
		verts[3].point += transform;
	}
}

//---------------------------------------------

DECLARE_CVAR(g_maxObjects, "200", nullptr, CV_ARCHIVE);

enum ESoundChannelType
{
	CHAN_STATIC = 0,

	CHAN_STREAM,
	CHAN_MENU,

	CHAN_COUNT
};

static ChannelDef s_soundChannels[] = {
	DEFINE_SOUND_CHANNEL(CHAN_STATIC, 16),	// anything that dont fit categories below
	DEFINE_SOUND_CHANNEL(CHAN_STREAM, 2),
	DEFINE_SOUND_CHANNEL(CHAN_MENU, 16),	// hit sounds
};
static_assert(elementsOf(s_soundChannels) == CHAN_COUNT, "ESoundChannelType needs to be in sync with s_soundChannels");

static CStaticAutoPtr<CState_SampleGameDemo> g_State_SampleGameDemo;

CState_SampleGameDemo::CState_SampleGameDemo()
{

}

// when changed to this state
// @from - used to transfer data
void CState_SampleGameDemo::OnEnter(CBaseStateHandler* from)
{
	g_sounds->Init(1000.0f, s_soundChannels, elementsOf(s_soundChannels));
	g_sounds->PrecacheSound("effect.rock");
	g_sounds->PrecacheSound("effect.paper");
	g_sounds->PrecacheSound("effect.scissors");

	g_pPFXRenderer->Init();

	m_moviePlayer = CRefPtr_new(CMoviePlayer);

	m_moviePlayer->OnCompleted += [&]() {
		m_moviePlayer->Rewind();
	};

	static constexpr const char* VIDEO_FILE_NAME = "resources/video/testvideo.mp4";

	const bool result = m_moviePlayer->Init(VIDEO_FILE_NAME);
	ASSERT(result);
	if (result)
	{
		m_moviePlayer->Start();
		//m_videoPlayer->SetTimeScale(0.5f);

		KVSection soundSec;
		soundSec.SetName("test.video_stream");
		soundSec.SetKey("wave", EqString('$') + VIDEO_FILE_NAME);
		soundSec.SetKey("is2d", true);
		soundSec.SetKey("loop", true);
		soundSec.SetKey("channel", "CHAN_STREAM");
		g_sounds->CreateSoundScript(&soundSec);
		g_sounds->PrecacheSound(soundSec.GetName());

		EmitParams ep(soundSec.GetName());
		g_sounds->EmitSound(&ep);
	}

	{
		static constexpr const int particlesGroupId = StringToHashConst("particles/translucent");

		m_pfxGroup = g_pPFXRenderer->CreateBatch("particles/translucent");
	}
}

// when the state changes to something
// @to - used to transfer data
void CState_SampleGameDemo::OnLeave(CBaseStateHandler* to)
{
	m_moviePlayer = nullptr;

	g_pPFXRenderer->Shutdown();
	m_pfxGroup = nullptr;
}

void CState_SampleGameDemo::InitGame()
{
	int count = 0;
	for (int i = 0; i < g_maxObjects.GetInt() / 3; ++i)
	{
		{
			const int entIdx = count++;
			ECSComponent<Position>::GetOrCreate(entIdx, Vector2D(RandomFloat(-1000.0f, 1000.0f), RandomFloat(-1000.0f, 1000.0f)));
			ECSComponent<State>::GetOrCreate(entIdx, RPSType::ROCK);
		}

		{
			const int entIdx = count++;
			ECSComponent<Position>::GetOrCreate(entIdx, Vector2D(RandomFloat(-1000.0f, 1000.0f), RandomFloat(-1000.0f, 1000.0f)));
			ECSComponent<State>::GetOrCreate(entIdx, RPSType::PAPER);
		}

		{
			const int entIdx = count++;
			ECSComponent<Position>::GetOrCreate(entIdx, Vector2D(RandomFloat(-1000.0f, 1000.0f), RandomFloat(-1000.0f, 1000.0f)));
			ECSComponent<State>::GetOrCreate(entIdx, RPSType::SCISSORS);
		}

		/*
		{
			RPSObject& newObj = m_objects.append();
			newObj.pos = Vector2D(RandomFloat(-1000.0f, 1000.0f), RandomFloat(-1000.0f, 1000.0f));
			newObj.type = RPSType::ROCK;
		}

		{
			RPSObject& newObj = m_objects.append();
			newObj.pos = Vector2D(RandomFloat(-1000.0f, 1000.0f), RandomFloat(-1000.0f, 1000.0f));
			newObj.type = RPSType::PAPER;
		}

		{
			RPSObject& newObj = m_objects.append();
			newObj.pos = Vector2D(RandomFloat(-1000.0f, 1000.0f), RandomFloat(-1000.0f, 1000.0f));
			newObj.type = RPSType::SCISSORS;
		}
		*/


	}
}

// when 'false' returned the next state goes on
bool CState_SampleGameDemo::Update(float fDt)
{
	m_moviePlayer->Present();

	const IVector2D& screenSize = g_pHost->GetWindowSize();

	g_pShaderAPI->Clear(true, true, false, ColorRGBA(0.5f));
	materials->SetupOrtho((-screenSize.x / 2) * m_zoomLevel, (screenSize.x / 2) * m_zoomLevel, (screenSize.y / 2) * m_zoomLevel, (-screenSize.y / 2) * m_zoomLevel, -1000, 1000);
	materials->SetMatrix(MATRIXMODE_VIEW, translate(-m_pan.x, -m_pan.y, 0.0f));

	materials->SetAmbientColor(color_white);

	// draw video
	{
		ITexturePtr videoTexture = m_moviePlayer->GetImage();

		MatTextureProxy(materials->FindGlobalMaterialVar(StringToHashConst("basetexture"))).Set(m_moviePlayer->GetImage());
		materials->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ZERO, BLENDFUNC_ADD);
		materials->SetRasterizerStates(CULL_NONE, FILL_SOLID);
		materials->SetDepthStates(false, false);
		materials->BindMaterial(materials->GetDefaultMaterial());

		AARectangle rect1(0, 0, 0, 0);
		AARectangle rect1UV(0, 0, 1, 1);

		if(videoTexture)
			rect1.Expand(Vector2D(videoTexture->GetWidth(), videoTexture->GetHeight()));

		CMeshBuilder meshBuilder(materials->GetDynamicMesh());
		meshBuilder.Begin(PRIM_TRIANGLES);
		{
			meshBuilder.Color4(color_white);
			meshBuilder.TexturedQuad2(
				rect1.GetVertex(0),
				rect1.GetVertex(1),
				rect1.GetVertex(2),
				rect1.GetVertex(3),
				rect1UV.GetVertex(0),
				rect1UV.GetVertex(1),
				rect1UV.GetVertex(2),
				rect1UV.GetVertex(3));
		}

		meshBuilder.End();
	}

	// Test code for math stuff
	{
		MatTextureProxy(materials->FindGlobalMaterialVar(StringToHashConst("basetexture"))).Set(nullptr);
		materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, BLENDFUNC_ADD);
		materials->SetRasterizerStates(CULL_NONE, FILL_SOLID);
		materials->SetDepthStates(false, false);
		materials->BindMaterial(materials->GetDefaultMaterial());

		AARectangle rect1(0, 0, 0, 0);
		AARectangle rect2(0, 0, 0, 0);

		rect1.Expand(Vector2D(40.0f, 25.0f));
		rect2.Expand(Vector2D(15.0f, 35.0f));

		static float time = 0.0f;
		time += fDt;
		const Vector3D box1PosRot(-60.0f, 15.0f, time * 45.0f);
		const Vector3D box2PosRot(15.0f, 0.0f, -time * 55.0f);

		const Matrix2x2 box1Rot = rotate2(DEG2RAD(box1PosRot.z));
		const Matrix2x2 box2Rot = rotate2(DEG2RAD(box2PosRot.z));

		CMeshBuilder meshBuilder(materials->GetDynamicMesh());
		meshBuilder.Begin(PRIM_TRIANGLES);

		{
			const float separationValue = RectangleRectangleSeparation(
				box1PosRot.xy(), box1Rot.rows[0], box1Rot.rows[1], rect1.GetSize() * 0.5f,
				box2PosRot.xy(), box2Rot.rows[0], box2Rot.rows[1], rect2.GetSize() * 0.5f);

			if (separationValue > 0.0f)
				meshBuilder.Color4f(0.25f, 1.0f, 0.25f, 0.7f);
			else
				meshBuilder.Color4f(1.0f, 0.25f, 0.25f, 0.7f);

			debugoverlay->Text3D(Vector3D(box1PosRot.xy() * Vector2D(1, -1), 0.0f), 1000.0f, color_white, EqString::Format("Separation: %.2f", separationValue));
			
			meshBuilder.Quad2(
				box1PosRot.xy()*Vector2D(1,-1) + box1Rot * rect1.GetVertex(0),
				box1PosRot.xy()*Vector2D(1,-1) + box1Rot * rect1.GetVertex(1),
				box1PosRot.xy()*Vector2D(1,-1) + box1Rot * rect1.GetVertex(2),
				box1PosRot.xy()*Vector2D(1,-1) + box1Rot * rect1.GetVertex(3));

			meshBuilder.Color4f(0.25f, 0.25f, 1.0f, 0.7f);
			meshBuilder.Quad2(
				box2PosRot.xy()*Vector2D(1,-1) + box2Rot * rect2.GetVertex(0), 
				box2PosRot.xy()*Vector2D(1,-1) + box2Rot * rect2.GetVertex(1), 
				box2PosRot.xy()*Vector2D(1,-1) + box2Rot * rect2.GetVertex(2),
				box2PosRot.xy()*Vector2D(1,-1) + box2Rot * rect2.GetVertex(3));
		}

		meshBuilder.End();
	}

	const AtlasEntry* atlRock = m_pfxGroup->FindEntry("rock");
	const AtlasEntry* atlPaper = m_pfxGroup->FindEntry("paper");
	const AtlasEntry* atlScissors = m_pfxGroup->FindEntry("scissors");

	const AtlasEntry* atlEntries[] = {
		atlRock, atlPaper, atlScissors
	};

	if (m_objects.numElem() == 0)
	{
		InitGame();
	}

	ECSSystem<Movement>::Process(Movement{ fDt });
	StateUpdate stateUpdate;
	ECSSystem<StateUpdate>::Process(stateUpdate);
	ECSSystem<Draw>::Process(Draw{ atlEntries, m_pfxGroup });

	/*
	int counts[static_cast<int>(RPSType::COUNT)]{ 0 };
	for (int i = 0; i < m_objects.numElem(); ++i)
	{
		RPSObject& curObj = m_objects[i];
		++counts[static_cast<int>(curObj.type)];

		Vector2D moveVector(0.0f);
		float criticalMass = 1.0f;

		// check for nearby objects
		// and intersect!
		for (int j = 0; j < m_objects.numElem(); ++j)
		{
			RPSObject& otherObj = m_objects[j];

			if (&curObj == &otherObj)
				continue;

			const int whoDefeats = CheckWhoDefeats(curObj.type, otherObj.type);
			const float distSqr = distanceSqr(curObj.pos, otherObj.pos);

			if (whoDefeats == 1)
				moveVector += normalize(otherObj.pos - curObj.pos) / distSqr;
			else if (whoDefeats == 2)
				moveVector -= normalize(otherObj.pos - curObj.pos) / distSqr * 0.25f;
			else
				criticalMass += rsqrtf(distSqr);

			if (distanceSqr(curObj.pos, otherObj.pos) > s_radiusSqr)
				continue;

			if (whoDefeats == 1) // a beaten b
			{
				otherObj.type = curObj.type;

				EmitParams ep(s_soundNames[static_cast<int>(curObj.type)]);
				ep.origin.x = curObj.pos.x * 0.01f;
				g_sounds->EmitSound(&ep);
			}
			else if (whoDefeats == 2) // b beaten a
			{
				curObj.type = otherObj.type;

				EmitParams ep(s_soundNames[static_cast<int>(otherObj.type)]);
				ep.origin.x = otherObj.pos.x * 0.01f;
				g_sounds->EmitSound(&ep);
			}
		}

		// move to closest objects
		//if(fabs(moveVector.x) > F_EPS && fabs(moveVector.y) > F_EPS)
		curObj.pos += moveVector * s_moveSpeed * criticalMass * fDt;

		// draw
		{
			const AARectangle rect = atlEntries[static_cast<int>(curObj.type)]->rect;

			PFXVertex_t* verts;
			if (m_pfxGroup->AllocateGeom(4, 4, &verts, nullptr, true) == -1)
				break;

			verts[0].point = Vector3D(0, 0, 0);
			verts[1].point = Vector3D(0, s_spriteSize, 0);
			verts[2].point = Vector3D(s_spriteSize, 0, 0);
			verts[3].point = Vector3D(s_spriteSize, s_spriteSize, 0);

			verts[0].texcoord = rect.GetLeftTop();
			verts[1].texcoord = rect.GetLeftBottom();
			verts[2].texcoord = rect.GetRightTop();
			verts[3].texcoord = rect.GetRightBottom();

			const Vector3D transform = Vector3D(curObj.pos.x, curObj.pos.y, 0.0f);

			verts[0].point += transform;
			verts[1].point += transform;
			verts[2].point += transform;
			verts[3].point += transform;
		}
	}*/

	int numLost = 0;

	for (int i = 0; i < static_cast<int>(RPSType::COUNT); ++i)
	{
		if (stateUpdate.counts[i] == 0)
			++numLost;
	}

	if(numLost == 2)
		m_objects.clear();

	g_pPFXRenderer->Render(0);

	g_sounds->Update();
	return true;
}

void CState_SampleGameDemo::HandleKeyPress(int key, bool down)
{
	if (key == KEY_SPACE && !down)
	{
		// reset game
		m_objects.clear();
	}
}

void CState_SampleGameDemo::HandleMouseClick(int x, int y, int buttons, bool down)
{
	if(buttons == MOU_B1)
		m_mouseDown = down;
}

void CState_SampleGameDemo::HandleMouseMove(int x, int y, float deltaX, float deltaY)
{
	if (m_mouseDown)
	{
		m_pan.x += deltaX * m_zoomLevel * 20.0f;
		m_pan.y -= deltaY * m_zoomLevel * 20.0f;
	}
}

void CState_SampleGameDemo::HandleMouseWheel(int x, int y, int scroll)
{
	m_zoomLevel -= scroll * 0.05f;
	m_zoomLevel = clamp(m_zoomLevel, 0.05f, 10.0f);
}

void CState_SampleGameDemo::HandleJoyAxis(short axis, short value)
{
}

void CState_SampleGameDemo::GetMouseCursorProperties(bool& visible, bool& centered) 
{
	visible = true; 
	centered = false; 
}