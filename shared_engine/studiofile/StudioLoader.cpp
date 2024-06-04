//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Graphics File loader
//////////////////////////////////////////////////////////////////////////////////

#include <zlib.h>

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "StudioLoader.h"

static bool IsValidModelIdentifier(int id)
{
	if(EQUILIBRIUM_MODEL_SIGNATURE == id)
		return true;

	return false;
}

static void ConvertHeaderToLatestVersion(basemodelheader_t* pBaseHdr)
{
	studioHdr_t* pHdr = (studioHdr_t*)pBaseHdr;
	const bool convertVertexFormat = !(pHdr->flags & STUDIO_FLAG_NEW_VERTEX_FMT);
	if (convertVertexFormat)
	{
		for (int i = 0; i < pHdr->numMeshGroups; ++i)
		{
			studioMeshGroupDesc_t* meshGroupDesc = pHdr->pMeshGroupDesc(i);

			// old vertex format has all features except color
			for (int j = 0; j < meshGroupDesc->numMeshes; ++j)
				meshGroupDesc->pMesh(j)->vertexType = STUDIO_VERTFLAG_POS_UV | STUDIO_VERTFLAG_TBN | STUDIO_VERTFLAG_BONEWEIGHT;
		}
	}
}

// loads all supported EGF model formats
studioHdr_t* Studio_LoadModel(const char* pszPath)
{
	IFilePtr file = g_fileSystem->Open(pszPath, "rb");

	if(!file)
	{
		MsgError("Can't open model file '%s'\n",pszPath);
		return nullptr;
	}

	const int len = file->GetSize();
	char* _buffer = (char*)PPAlloc(len+32); // +32 bytes for conversion issues

	file->Read(_buffer, 1, len);
	file = nullptr;

	basemodelheader_t* pBaseHdr = (basemodelheader_t*)_buffer;

	if(!IsValidModelIdentifier( pBaseHdr->ident ))
	{
		PPFree(_buffer);
		MsgError("Invalid model file '%s'\n",pszPath);
		return nullptr;
	}

	ConvertHeaderToLatestVersion( pBaseHdr );

	// TODO: Double data protection!!! (hash lookup)

	studioHdr_t* pHdr = (studioHdr_t*)pBaseHdr;

	if(pHdr->version != EQUILIBRIUM_MODEL_VERSION)
	{
		MsgError("Wrong model '%s' version, excepted %i, but model version is %i\n",pszPath, EQUILIBRIUM_MODEL_VERSION,pBaseHdr->version);
		PPFree(_buffer);
		return nullptr;
	}

	if(len != pHdr->length)
	{
		MsgError("Model is not valid (read %d vs size %d in header)!\n",len, pBaseHdr->size);
		PPFree(_buffer);
		return nullptr;
	}

	return pHdr;
}

StudioMotionData* Studio_LoadMotionData(const char* pszPath, int boneCount)
{
	if (!boneCount)
		return nullptr;

	ubyte* pData = g_fileSystem->GetFileBuffer(pszPath);
	ubyte* pStart = pData;

	if(!pData)
		return nullptr;

	lumpfilehdr_t* pHDR = (lumpfilehdr_t*)pData;

	if(pHDR->ident != ANIMFILE_IDENT)
	{
		MsgError("%s: not a motion package file\n", pszPath);
		PPFree(pData);
		return nullptr;
	}

	if(pHDR->version != ANIMFILE_VERSION)
	{
		MsgError("%s: bad motion package version\n", pszPath);
		PPFree(pData);
		return nullptr;
	}

	pData += sizeof(lumpfilehdr_t);

	StudioMotionData* pMotion = (StudioMotionData*)PPAlloc(sizeof(StudioMotionData));

	int numAnimDescs = 0;
	int numAnimFrames = 0;

	animationdesc_t*	animationdescs = nullptr;
	animframe_t*		animframes = nullptr;

	bool hasCompressedAnimationFrames	= false;
	int nUncompressedFramesSize		= 0;

	// parse motion package
	for(int lump = 0; lump < pHDR->numLumps; lump++)
	{
		lumpfilelump_t* pLump = (lumpfilelump_t*)pData;
		pData += sizeof(lumpfilelump_t);

		switch(pLump->type)
		{
			case ANIMFILE_ANIMATIONS:
			{
				numAnimDescs = pLump->size / sizeof(animationdesc_t);
				animationdescs = (animationdesc_t*)pData;
				break;
			}
			case ANIMFILE_ANIMATIONFRAMES:
			{
				numAnimFrames = pLump->size / sizeof(animframe_t);
				animframes = (animframe_t*)pData;
				hasCompressedAnimationFrames = false;

				break;
			}
			case ANIMFILE_UNCOMPRESSEDFRAMESIZE:
			{
				nUncompressedFramesSize = *(int*)pData;
				break;
			}
			case ANIMFILE_COMPRESSEDFRAMES:
			{
				animframes = (animframe_t*)PPAlloc(nUncompressedFramesSize + 150);

				unsigned long realSize = nUncompressedFramesSize;

				int status = uncompress((ubyte*)animframes,&realSize,pData,pLump->size);

				if (status == Z_OK)
				{
					numAnimFrames = realSize / sizeof(animframe_t);
					hasCompressedAnimationFrames = true;
				}
				else
					MsgError("ERROR! Cannot decompress animation frames from %s (error %d)!\n", pszPath, status);

				break;
			}
			case ANIMFILE_SEQUENCES:
			{
				pMotion->numsequences = pLump->size / sizeof(sequencedesc_t);

				pMotion->sequences = (sequencedesc_t*)PPAlloc(pLump->size);
				memcpy(pMotion->sequences, pData, pLump->size);
				break;
			}
			case ANIMFILE_EVENTS:
			{
				pMotion->numEvents = pLump->size / sizeof(sequenceevent_t);

				pMotion->events = (sequenceevent_t*)PPAlloc(pLump->size);
				memcpy(pMotion->events, pData, pLump->size);
				break;
			}
			case ANIMFILE_POSECONTROLLERS:
			{
				pMotion->numPoseControllers = pLump->size / sizeof(posecontroller_t);

				pMotion->poseControllers = (posecontroller_t*)PPAlloc(pLump->size);
				memcpy(pMotion->poseControllers, pData, pLump->size);
				break;
			}
		}

		pData += pLump->size;
	}

	// first processing done, convert animca animations to EGF format.
	pMotion->animations = PPAllocStructArray(StudioAnimData, numAnimDescs);
	pMotion->numAnimations = numAnimDescs;

	pMotion->frames = PPAllocStructArray(animframe_t, numAnimFrames);
	memcpy(pMotion->frames, animframes, numAnimFrames * sizeof(animframe_t));

	for(int i = 0; i < pMotion->numAnimations; i++)
	{
		StudioAnimData& anim = pMotion->animations[i];
		strcpy(anim.name, animationdescs[i].name);

		// determine frame count of animation
		const int numFrames = animationdescs[i].numFrames / boneCount;

		anim.bones = PPAllocStructArray(StudioBoneFrames, boneCount);
		//anim.numFrames = numFrames;

		// since frames are just flat array of each bone, we can simply reference it
		for(int j = 0; j < boneCount; j++)
		{
			anim.bones[j].numFrames = numFrames;
			anim.bones[j].keyFrames = pMotion->frames + (animationdescs[i].firstFrame + j * numFrames);
		}
	}

	if(hasCompressedAnimationFrames)
	{
		PPFree(animframes);
	}

	PPFree(pStart);

	return pMotion;
}

bool Studio_LoadPhysModel(const char* pszPath, StudioPhysData* pModel)
{
	ubyte* pData = g_fileSystem->GetFileBuffer( pszPath );
	ubyte* pStart = pData;

	if(!pData)
		return false;

	lumpfilehdr_t *pHdr = (lumpfilehdr_t*)pData;

	if(pHdr->ident != PHYSFILE_ID)
	{
		MsgError("'%s' is not a POD physics model\n", pszPath);
		PPFree(pData);
		return false;
	}

	if(pHdr->version != PHYSFILE_VERSION)
	{
		MsgError("POD-File '%s' has physics model version\n", pszPath);
		PPFree(pData);
		return false;
	}

	Array<const char*> objectNames(PP_SL);
	pData += sizeof(lumpfilehdr_t);

	const int nLumps = pHdr->numLumps;
	for(int lump = 0; lump < nLumps; lump++)
	{
		lumpfilelump_t* pLump = (lumpfilelump_t*)pData;
		pData += sizeof(lumpfilelump_t);

		switch(pLump->type)
		{
			case PHYSFILE_PROPERTIES:
			{
				physmodelprops_t* props = (physmodelprops_t*)pData;
				pModel->usageType = static_cast<EPhysModelUsage>(props->usageType);
				break;
			}
			case PHYSFILE_SHAPEINFO:
			{
				const int numGeomInfos = pLump->size / sizeof(physgeominfo_t);
				ArrayCRef<physgeominfo_t> shapesInfoFile(reinterpret_cast<physgeominfo_t*>(pData), numGeomInfos);

				pModel->shapes = PPNewArrayRef(StudioPhyShapeData, numGeomInfos);

				for(int i = 0; i < numGeomInfos; i++)
				{
					pModel->shapes[i].cacheRef = nullptr;

					// copy shape info
					memcpy(&pModel->shapes[i].shapeInfo, &shapesInfoFile[i], sizeof(physgeominfo_t));
				}
				break;
			}
			case PHYSFILE_OBJECTNAMES:
			{
				const char* name = (char*)pData;

				int sz = 0;
				do
				{
					const char* str = name + sz;
					objectNames.append(str);

					sz += strlen(str) + 1;
				}while(sz < pLump->size);

				break;
			}
			case PHYSFILE_OBJECTS:
			{
				const int numObjInfos = pLump->size / sizeof(physobject_t);
				ArrayCRef<physobject_t> physObjsFile(reinterpret_cast<physobject_t*>(pData), numObjInfos);

				pModel->objects = PPNewArrayRef(StudioPhyObjData, numObjInfos);
				for(int i = 0; i < physObjsFile.numElem(); i++)
				{
					StudioPhyObjData& objData = pModel->objects[i];
					memset(objData.shapeCacheRefs, 0, sizeof(objData.shapeCacheRefs));
					objData.object = physObjsFile[i];
					
					if(objectNames.numElem() > 0)
						objData.name = objectNames[i];
				}
				break;
			}
			case PHYSFILE_JOINTDATA:
			{
				const int numJointInfos = pLump->size / sizeof(physjoint_t);

				if(numJointInfos)
				{
					ArrayCRef<physjoint_t> jointInfosFile(reinterpret_cast<physjoint_t*>(pData), numJointInfos);
					pModel->joints = PPNewArrayRef(physjoint_t, numJointInfos);
					memcpy(pModel->joints.ptr(), jointInfosFile.ptr(), pLump->size);
				}
				break;
			}
			case PHYSFILE_VERTEXDATA:
			{
				const int numVerts = pLump->size / sizeof(Vector3D);
				ArrayCRef<Vector3D> vertsFile(reinterpret_cast<Vector3D*>(pData), numVerts);
				pModel->vertices = PPNewArrayRef(Vector3D, numVerts);

				memcpy(pModel->vertices.ptr(), vertsFile.ptr(), pLump->size);
				break;
			}
			case PHYSFILE_INDEXDATA:
			{
				const int numIndices = pLump->size / sizeof(int);
				ArrayCRef<int> indicesFile(reinterpret_cast<int*>(pData), numIndices);
				pModel->indices = PPNewArrayRef(int, numIndices);

				memcpy(pModel->indices.ptr(), indicesFile.ptr(), pLump->size);
				break;
			}
			default:
			{
				MsgWarning("*WARNING* Invalid POD-file '%s' lump type '%d'.\n", pszPath, pLump->type);
				break;
			}
		}

		pData += pLump->size;
	}

	PPFree(pStart);
	return true;
}

void Studio_FreeModel(studioHdr_t* pModel)
{
	PPFree(pModel);
}

void Studio_FreeMotionData(StudioMotionData* data, int numBones)
{
	// NOTE: no need to delete bone keyFrames since they are mapped from data->frames.
	for (int i = 0; i < data->numAnimations; i++)
		PPFree(data->animations[i].bones);

	PPFree(data->frames);
	PPFree(data->sequences);
	PPFree(data->events);
	PPFree(data->poseControllers);
	PPFree(data->animations);
}

void Studio_FreePhysModel(StudioPhysData* model)
{
	PPDeleteArrayRef(model->indices);
	PPDeleteArrayRef(model->vertices);
	PPDeleteArrayRef(model->shapes);
	PPDeleteArrayRef(model->objects);
	PPDeleteArrayRef(model->joints);
}
