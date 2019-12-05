//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Billboard list file and renderer
//////////////////////////////////////////////////////////////////////////////////

#ifdef GAME_DRIVERS
#include "world.h"
#endif // GAME_DRIVERS
#include "BillboardList.h"

#include "ConVar.h"

#include "IFileSystem.h"
#include "utils/strtools.h"
#include "utils/Tokenizer.h"
#include "IDebugOverlay.h"
#include "IEqModel.h"


#ifdef TREEGEN

// THIS IS OBJ PARTS

bool isNotWhiteSpace(const char ch)
{
	return (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n');
}

float readFloat(Tokenizer &tok)
{
	char *str = tok.next( isNotWhiteSpace );
	//Msg("readFloat: str=%s\n", str);

	return (float)atof(str);
}

extern int xstrsplitws(char* str, char **pointer_array);

int strchcount( char *str, char ch )
{
	int count = 0;
	while (*str++)
	{
		if (*str == ch)
			count++;
	}

	return count;
}

bool isNotNewLine(const char ch)
{
	return (ch != '\r' && ch != '\n');
}

bool isNumericAlpha(const char ch)
{
	return (ch == 'e' || ch == '-' || ch == '+' || ch == '.') || isNumeric(ch);
}

bool LoadObjAsPointList(const char* filename, DkList<Vector3D>& outPoints)
{
	Tokenizer tok;

	char* pBuffer = g_fileSystem->GetFileBuffer(filename);

	if (!pBuffer)
	{
		MsgError("Couldn't open OBJ file '%s'", filename);
		return false;
	}

	tok.setString(pBuffer);

	// done with it
	PPFree(pBuffer);

	char *str;

	// reset reader
	tok.reset();

	bool gl_to_eq = true;
	bool blend_to_eq = false;

	while ((str = tok.next()) != NULL)
	{
		if(str[0] == '#')
		{
			char* check_tok = tok.next();
			if(check_tok && !stricmp(check_tok, "blender"))
			{
				gl_to_eq = true;
				blend_to_eq = true;
				//reverseNormals = true;
			}

			tok.goToNextLine();
			continue;
		}
		else if(str[0] == 'v')
		{
			char stored_str[3] = {str[0], str[1], 0};

			if(str[1] == 't')
			{

			}
			else if(str[1] == 'n')
			{

			}
			else
			{
				// parse vector3
				Vector3D v;

				v.x = readFloat( tok );
				v.y = readFloat( tok );
				v.z = readFloat( tok );

				if(blend_to_eq)
					v = Vector3D(v.z, v.y, v.x);

				outPoints.append(v);
			}
		}
		tok.goToNextLine();
	}

	return true;
}

extern IEqModel* g_pModel;

#endif // TREEGEN

#ifdef GAME_DRIVERS
extern CPFXAtlasGroup* g_treeAtlas;
#endif // GAME_DRIVERS

CBillboardList::CBillboardList()
{
	m_renderGroup = NULL;
}

CBillboardList::~CBillboardList()
{
}

void CBillboardList::LoadBlb( kvkeybase_t* kvs )
{
	// TODO: better atlas cache
	//const char* atlasName = KV_GetValueString(kvs->FindKeyBase("atlas"), 0, "scripts/billboard_trees.atlas");

#ifdef GAME_DRIVERS
	m_renderGroup = g_treeAtlas;
#endif // GAME_DRIVERS

	for(int i = 0; i < kvs->keys.numElem(); i++)
	{
		if(!stricmp(kvs->keys[i]->name, "blb"))
		{
			kvkeybase_t* entry = kvs->keys[i];

			blbsprite_t sprite;
			sprite.entry = m_renderGroup->FindEntry( KV_GetValueString(entry) );
			sprite.position = KV_GetVector3D(entry, 1);
			sprite.scale = KV_GetValueFloat(entry, 4);

			m_aabb.AddVertex(sprite.position);

			/*
			for(int j = 0; j < m_sprites.numElem(); j++)
			{
				float fDist = length(sprite.position-m_sprites[j].position);

				sprite.distfactor = fDist;
			}*/

			m_sprites.append(sprite);
		}
	}

	float aabbSize = length(m_aabb.GetSize());

	for(int i = 0; i < m_sprites.numElem(); i++)
	{
		blbsprite_t& sprite = m_sprites[i];
		sprite.distfactor = (aabbSize - length(sprite.position - m_aabb.GetCenter())) / aabbSize;
	}
}

void CBillboardList::SaveBlb( const char* filename )
{
#ifdef TREEGEN
	KeyValues kvs;

	kvs.GetRootSection()->AddKeyBase("model", g_pModel->GetName());
	kvs.GetRootSection()->AddKeyBase("atlas", "scripts/billboard_trees.atlas");

	for(int i = 0; i < m_sprites.numElem(); i++)
	{
		kvkeybase_t* kvb = kvs.GetRootSection()->AddKeyBase("blb");
		kvb->AddValue(m_sprites[i].entry->name);
		kvb->AddValue(varargs("%g", m_sprites[i].position.x));
		kvb->AddValue(varargs("%g", m_sprites[i].position.y));
		kvb->AddValue(varargs("%g", m_sprites[i].position.z));
		kvb->AddValue(varargs("%g", m_sprites[i].scale));
	}

	kvs.SaveToFile(filename);
#endif // TREEGEN
}

#define BILLBOARD_POINTS_DISTFACTOR (0.4f)

void CBillboardList::Generate( Vector3D* pointList, int numPoints, float minSize, float maxSize )
{
#ifdef TREEGEN
	m_sprites.clear();
	m_aabb.Reset();

	for(int i = 0; i < numPoints; i++)
	{
		float fPrefferedSize = RandomFloat(minSize, maxSize);

		bool add = true;

		for(int j = 0; j < m_sprites.numElem(); j++)
		{
			float fDist = distance(pointList[i], m_sprites[j].position);

			if(fDist < fPrefferedSize*BILLBOARD_POINTS_DISTFACTOR)
			{
				add = false;
				break;
			}
		}

		if(!add)
			continue;

		int preferredSprite = RandomInt(0,m_renderGroup->GetEntryCount()-1);

		blbsprite_t sprite;
		sprite.entry = m_renderGroup->GetEntry(preferredSprite);
		sprite.position = pointList[i];
		sprite.scale = fPrefferedSize;

		m_aabb.AddVertex(sprite.position);

		m_sprites.append(sprite);
	}

#endif // TREEGEN
}

void CBillboardList::DestroyBlb()
{
	m_sprites.clear();
	m_renderGroup = NULL;
}

const float BILLBOARD_DISAPPEAR_DISTANCE = 180.0f;

ConVar r_drawBillboardLists("r_drawBillboardLists", "1", "Draw billboard lists (used by trees)", CV_CHEAT );
ConVar r_billboardDistanceScaling("r_billboardDistanceScaling", "0.007", nullptr, CV_CHEAT);

void CBillboardList::DrawBillboards()
{
    if(!r_drawBillboardLists.GetBool())
        return;

	PFXBillboard_t effect;

	Matrix4x4 viewMat, worldMat;
	materials->GetMatrix(MATRIXMODE_VIEW, viewMat);
	materials->GetMatrix(MATRIXMODE_WORLD, worldMat);

	Vector3D transformPos = transpose(worldMat).getTranslationComponent();

	Vector3D blbCenter = m_aabb.GetCenter() + transformPos;

#ifdef GAME_DRIVERS

	BoundingBox lightBbox(m_aabb.minPoint+transformPos, m_aabb.maxPoint+transformPos);

	wlight_t applyLights[MAX_LIGHTS];
	int numLights = 0;
	g_pGameWorld->GetLightList(lightBbox, applyLights, numLights);

	float fDistToCenter = length(g_pGameWorld->m_view.GetOrigin() - transformPos);
	float distCurved = powf(fDistToCenter / BILLBOARD_DISAPPEAR_DISTANCE, 0.5f)*BILLBOARD_DISAPPEAR_DISTANCE;
	//float fTreeSizeFactor = pow(1.0f / m_aabb.GetSize().x, 2.0f)*5.0f;

#endif // GAME_DRIVERS

	int numDrawnSprites = 0;

	worldinfo_t worldInfo = g_pGameWorld->m_info;

	for(int i = 0; i < m_sprites.numElem(); i++)
	{
		const blbsprite_t& sprite = m_sprites[i];

		// TODO: light color on it!

		effect.vOrigin = (worldMat*Vector4D(sprite.position, 1)).xyz();

#ifdef GAME_DRIVERS

		effect.vColor = worldInfo.ambientColor;

		Vector3D posAsNormal = fastNormalize(effect.vOrigin-blbCenter);

		if(distCurved > BILLBOARD_DISAPPEAR_DISTANCE*sprite.distfactor )
		{
			if(dot(posAsNormal, viewMat.rows[2].xyz()) > -0.5f)
				continue;
		}

		float fSunDiffuse = saturate(dot(posAsNormal, worldInfo.sunDir)*0.5f+0.5f);

		// central billboards are darker
		//float fLightFactor = length(effect.vOrigin.xz()-blbCenter.xz())*fTreeSizeFactor;

		effect.vColor += worldInfo.sunColor * fSunDiffuse;// *fLightFactor;

		for(int j = 0; j < numLights; j++)
		{
			Vector3D lvec = (effect.vOrigin-applyLights[j].position.xyz())*applyLights[j].position.w;

			float fDiffuse = saturate(-dot(posAsNormal, fastNormalize(lvec))*0.85f+0.15f);
			float fAtten = 1.0f-saturate(dot(lvec, lvec));
			effect.vColor += applyLights[j].color * fDiffuse*fAtten;
		}
#else
		effect.vColor = color4_white;
#endif // GAME_DRIVERS

		numDrawnSprites++;

		effect.nFlags = EFFECT_FLAG_NO_FRUSTUM_CHECK;
		effect.fZAngle = 0.0f;

		effect.group = m_renderGroup;

		effect.fWide = effect.fTall = sprite.scale + (distCurved*sprite.distfactor)*r_billboardDistanceScaling.GetFloat();

		effect.tex = sprite.entry;

		Effects_DrawBillboard(&effect, viewMat, NULL);
	}

#ifdef TREEGEN
	debugoverlay->Text(ColorRGBA(1,1,1,1), "Billboard count: %d\n", m_sprites.numElem());
#else
	//debugoverlay->Text3D(transformPos, ColorRGBA(1,1,1,1), "%d sprites\n", numDrawnSprites);
#endif // TREEGEN
}
