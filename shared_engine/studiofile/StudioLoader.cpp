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

bool Studio_LoadMotionData(const char* pszPath, StudioMotionData& motionData)
{
	ubyte* pData = g_fileSystem->GetFileBuffer(pszPath);
	ubyte* pStart = pData;
	if(!pData)
		return false;

	defer{
		PPFree(pStart);
	};

	const lumpfilehdr_t* pHdr = (lumpfilehdr_t*)pData;
	if(pHdr->ident != ANIMFILE_IDENT)
	{
		MsgError("%s: not a motion package file\n", pszPath);
		return false;
	}

	if(pHdr->version != ANIMFILE_VERSION)
	{
		MsgError("%s: bad motion package version\n", pszPath);
		return false;
	}

	pData += sizeof(lumpfilehdr_t);

	int uncomressedFramesSize = 0;

	// parse motion package
	for(int lump = 0; lump < pHdr->numLumps; lump++)
	{
		const lumpfilelump_t* pLump = (lumpfilelump_t*)pData;
		pData += sizeof(lumpfilelump_t);

		switch(pLump->type)
		{
			case ANIMFILE_ANIMATIONS:
			{
				const int numAnimDescs = pLump->size / sizeof(animationdesc_t);
				motionData.animations = PPNewArrayRef(animationdesc_t, numAnimDescs);
				
				memcpy(motionData.animations.ptr(), pData, pLump->size);
				break;
			}
			case ANIMFILE_ANIMATIONFRAMES:
			{
				const int numAnimFrames = pLump->size / sizeof(animframe_t);
				motionData.frames = PPNewArrayRef(animframe_t, numAnimFrames);
					
				memcpy(motionData.frames.ptr(), pData, pLump->size);
				break;
			}
			case ANIMFILE_UNCOMPRESSEDFRAMESIZE:
			{
				uncomressedFramesSize = *(int*)pData;
				break;
			}
			case ANIMFILE_COMPRESSEDFRAMES:
			{
				ASSERT(uncomressedFramesSize > 0);

				const int numAnimFrames = uncomressedFramesSize / sizeof(animframe_t);
				motionData.frames = PPNewArrayRef(animframe_t, numAnimFrames);

				unsigned long realSize = uncomressedFramesSize;
				const int status = uncompress((ubyte*)motionData.frames.ptr(), &realSize, pData, pLump->size);
				if (status == Z_OK)
				{
					ASSERT(realSize == uncomressedFramesSize);
				}
				else
					MsgError("ERROR! Cannot decompress animation frames from %s (error %d)!\n", pszPath, status);

				break;
			}
			case ANIMFILE_SEQUENCES:
			{
				const int numSequences = pLump->size / sizeof(sequencedesc_t);

				motionData.sequences = PPNewArrayRef(sequencedesc_t, numSequences);
				memcpy(motionData.sequences.ptr(), pData, pLump->size);
				break;
			}
			case ANIMFILE_EVENTS:
			{
				const int numEvents = pLump->size / sizeof(sequenceevent_t);

				motionData.events = PPNewArrayRef(sequenceevent_t, numEvents);
				memcpy(motionData.events.ptr(), pData, pLump->size);
				break;
			}
			case ANIMFILE_POSECONTROLLERS:
			{
				const int numPoseControllers = pLump->size / sizeof(posecontroller_t);

				motionData.poseControllers = PPNewArrayRef(posecontroller_t, numPoseControllers);
				memcpy(motionData.poseControllers.ptr(), pData, pLump->size);
				break;
			}
		}

		pData += pLump->size;
	}

	motionData.name = fnmPathStripExt(fnmPathExtractName(pszPath));
	return true;
}

bool Studio_LoadPhysModel(const char* pszPath, StudioPhysData& physData)
{
	ubyte* pData = g_fileSystem->GetFileBuffer( pszPath );
	ubyte* pStart = pData;

	if(!pData)
		return false;

	defer{
		PPFree(pStart);
	};

	const lumpfilehdr_t *pHdr = (lumpfilehdr_t*)pData;
	if(pHdr->ident != PHYSFILE_ID)
	{
		MsgError("'%s' is not a POD physics model\n", pszPath);
		return false;
	}

	if(pHdr->version != PHYSFILE_VERSION)
	{
		MsgError("POD-File '%s' has physics model version\n", pszPath);
		return false;
	}

	Array<const char*> objectNames(PP_SL);
	pData += sizeof(lumpfilehdr_t);

	for(int lump = 0; lump < pHdr->numLumps; lump++)
	{
		const lumpfilelump_t* pLump = (lumpfilelump_t*)pData;
		pData += sizeof(lumpfilelump_t);

		switch(pLump->type)
		{
			case PHYSFILE_PROPERTIES:
			{
				physmodelprops_t* props = (physmodelprops_t*)pData;
				physData.usageType = static_cast<EPhysModelUsage>(props->usageType);
				break;
			}
			case PHYSFILE_SHAPEINFO:
			{
				const int numGeomInfos = pLump->size / sizeof(physgeominfo_t);
				ArrayCRef<physgeominfo_t> shapesInfoFile(reinterpret_cast<physgeominfo_t*>(pData), numGeomInfos);

				physData.shapes = PPNewArrayRef(StudioPhyShapeData, numGeomInfos);
				for(int i = 0; i < numGeomInfos; i++)
				{
					physData.shapes[i].cacheRef = nullptr;
					memcpy(&physData.shapes[i].shapeInfo, &shapesInfoFile[i], sizeof(physgeominfo_t));
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

				physData.objects = PPNewArrayRef(StudioPhyObjData, numObjInfos);
				for(int i = 0; i < physObjsFile.numElem(); i++)
				{
					StudioPhyObjData& objData = physData.objects[i];
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
					physData.joints = PPNewArrayRef(physjoint_t, numJointInfos);
					memcpy(physData.joints.ptr(), pData, pLump->size);
				}
				break;
			}
			case PHYSFILE_VERTEXDATA:
			{
				const int numVerts = pLump->size / sizeof(Vector3D);
				physData.vertices = PPNewArrayRef(Vector3D, numVerts);

				memcpy(physData.vertices.ptr(), pData, pLump->size);
				break;
			}
			case PHYSFILE_INDEXDATA:
			{
				const int numIndices = pLump->size / sizeof(int);
				physData.indices = PPNewArrayRef(int, numIndices);

				memcpy(physData.indices.ptr(), pData, pLump->size);
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
	return true;
}

void Studio_FreeModel(studioHdr_t* pModel)
{
	PPFree(pModel);
}

void Studio_FreeMotionData(StudioMotionData& data)
{
	PPDeleteArrayRef(data.frames);
	PPDeleteArrayRef(data.sequences);
	PPDeleteArrayRef(data.events);
	PPDeleteArrayRef(data.poseControllers);
	PPDeleteArrayRef(data.animations);
}

void Studio_FreePhysModel(StudioPhysData& model)
{
	PPDeleteArrayRef(model.indices);
	PPDeleteArrayRef(model.vertices);
	PPDeleteArrayRef(model.shapes);
	PPDeleteArrayRef(model.objects);
	PPDeleteArrayRef(model.joints);
}
