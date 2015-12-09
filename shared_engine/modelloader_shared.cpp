//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech engine model loader
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"
#include "modelloader_shared.h"
#include "IFileSystem.h"
#include <zlib.h>

bool IsValidModelIdentifier(int id)
{
	switch(id)
	{
		case EQUILIBRIUM_MODEL_SIGNATURE:
			return true;
#ifdef EQUILIBRIUMFX_MODEL_FORMAT
		case EQUILIBRIUMFX_MODEL_SIGNATURE:
			return true;
#endif
#ifdef SUNSHINE_MODEL_FORMAT
		case SUNSHINE_MODEL_SIGNATURE:
			return true;
#endif
#ifdef SUNSHINEFX_MODEL_FORMAT
		case SUNSHINEFX_MODEL_SIGNATURE:
			return true;
#endif
	}
	return false;
}

void ConvertHeaderToLatestVersion(basemodelheader_t* pHdr)
{

}

// loads all supported darktech model formats
studiohdr_t* Studio_LoadModel(const char* pszPath)
{
	long len = 0;

	DKFILE* file = GetFileSystem()->Open(pszPath, "rb");

	if(!file)
	{
		MsgError("Can't open model file '%s'\n",pszPath);
		return NULL;
	}

	GetFileSystem()->Seek(file,0,SEEK_END);
	len = GetFileSystem()->Tell(file);
	GetFileSystem()->Seek(file,0,SEEK_SET);

	char* _buffer = (char*)PPAlloc(len+32); // +32 bytes for conversion issues

	GetFileSystem()->Read(_buffer, 1, len, file);
	GetFileSystem()->Close(file);

	basemodelheader_t* pBaseHdr = (basemodelheader_t*)_buffer;

	if(!IsValidModelIdentifier( pBaseHdr->m_nIdentifier ))
	{
		PPFree(_buffer);
		MsgError("Invalid model file '%s'\n",pszPath);
		return NULL;
	}

	ConvertHeaderToLatestVersion( pBaseHdr );

	// TODO: Double data protection!!! (hash lookup)

	studiohdr_t* pHdr = (studiohdr_t*)pBaseHdr;

	if(pHdr->version != EQUILIBRIUM_MODEL_VERSION)
	{
		MsgError("Wrong model '%s' version, excepted %i, but model version is %i\n",pszPath, EQUILIBRIUM_MODEL_VERSION,pBaseHdr->m_nVersion);
		PPFree(_buffer);
		return NULL;
	}

	if(len != pHdr->length)
	{
		MsgError("Model is not valid (%d versus %d in header)!\n",len,pBaseHdr->m_nLength);
		PPFree(_buffer);
		return NULL;
	}

	/*
#ifndef EDITOR
	char* str = (char*)stackalloc(strlen(pszPath+1));
	strcpy(str, pszPath);
	FixSlashes(str);

	if(stricmp(str, pHdr->modelname))
	{
		MsgError("Model %s is not valid model, didn't you replaced model?\n", pszPath);
		CacheFree(hunkHiMark);
		return NULL;
	}
#endif
	*/
	

	return pHdr;
}

studiomotiondata_t* Studio_LoadMotionData(const char* pszPath, int boneCount)
{
	ubyte* pData = (ubyte*)GetFileSystem()->GetFileBuffer(pszPath);
	ubyte* pStart = pData;

	if(!pData)
	{
		return NULL;
	}

	DevMsg(2,"Loading %s motion package\n", pszPath);

	animpackagehdr_t* pHDR = (animpackagehdr_t*)pData;

	if(pHDR->ident != ANIMCA_IDENT)
	{
		MsgError("%s: not a motion package file\n", pszPath);
		PPFree(pData);
		return NULL;
	}

	if(pHDR->version != ANIMCA_VERSION)
	{
		MsgError("Bad motion package version, please update or reinstall the game.\n", pszPath);
		PPFree(pData);
		return NULL;
	}

	pData += sizeof(animpackagehdr_t);

	studiomotiondata_t* pMotion = (studiomotiondata_t*)PPAlloc(sizeof(studiomotiondata_t));

	int numAnimDescs = 0;
	int numAnimFrames = 0;

	animationdesc_t*	animationdescs = NULL;
	animframe_t*		animframes = NULL;

	bool	anim_frames_decompressed	= false;
	int		nUncompressedFramesSize		= 0;

	// parse motion package
	for(int lump = 0; lump < pHDR->numLumps; lump++)
	{
		animpackagelump_t* pLump = (animpackagelump_t*)pData;
		pData += sizeof(animpackagelump_t);

		switch(pLump->type)
		{
			case ANIMCA_ANIMATIONS:
			{
				numAnimDescs = pLump->size / sizeof(animationdesc_t);
				animationdescs = (animationdesc_t*)pData;

				//	Msg("Num anim descs: %d\n", numAnimDescs);
				DevMsg(2,"all animations: %d\n", numAnimDescs);

				break;
			}
			case ANIMCA_ANIMATIONFRAMES:
			{
				numAnimFrames = pLump->size / sizeof(animframe_t);
				animframes = (animframe_t*)pData;

				DevMsg(2,"all animframes: %d\n", numAnimFrames);

				anim_frames_decompressed = false;

				break;
			}
			case ANIMCA_UNCOMPRESSEDFRAMESIZE:
			{
				nUncompressedFramesSize = *(int*)pData;
				break;
			}
			case ANIMCA_COMPRESSEDFRAMES:
			{
				animframes = (animframe_t*)PPAlloc(nUncompressedFramesSize + 150);

				unsigned long realSize = nUncompressedFramesSize;

				int status = uncompress((ubyte*)animframes,&realSize,pData,pLump->size);

				if (status == Z_OK)
				{
					numAnimFrames = realSize / sizeof(animframe_t);
					DevMsg(2,"all animframes: %d\n", numAnimFrames);

					anim_frames_decompressed = true;
				}
				else
					MsgError("ERROR! Cannot decompress animation frames from %s (error %d)!\n", pszPath, status);

				break;
			}
			case ANIMCA_SEQUENCES:
			{
				pMotion->numsequences = pLump->size / sizeof(sequencedesc_t);

				pMotion->sequences = (sequencedesc_t*)PPAlloc(pLump->size);
				memcpy(pMotion->sequences, pData, pLump->size);

				DevMsg(2,"sequence descs: %d\n", pMotion->numsequences);

				break;
			}
			case ANIMCA_EVENTS:
			{
				pMotion->numevents = pLump->size / sizeof(sequenceevent_t);

				pMotion->events = (sequenceevent_t*)PPAlloc(pLump->size);
				memcpy(pMotion->events, pData, pLump->size);

				DevMsg(2,"events: %d\n", pMotion->numevents);

				break;
			}
			case ANIMCA_POSECONTROLLERS:
			{
				pMotion->numposecontrollers = pLump->size / sizeof(posecontroller_t);

				pMotion->posecontrollers = (posecontroller_t*)PPAlloc(pLump->size);
				memcpy(pMotion->posecontrollers, pData, pLump->size);

				DevMsg(2,"pose controllers: %d\n", pMotion->numposecontrollers);

				break;
			}
		}

		pData += pLump->size;
	}

	// first processing done, convert animca animations to darktech format.
	pMotion->animations = (modelanimation_t*)PPAlloc(sizeof(modelanimation_t)*numAnimDescs);

	//Msg("Num anim descs: %d\n", numAnimDescs);

	pMotion->numanimations = numAnimDescs;

	pMotion->frames = (animframe_t*)PPAlloc(numAnimFrames * sizeof(animframe_t));
	memcpy(pMotion->frames, animframes, numAnimFrames * sizeof(animframe_t));

	for(int i = 0; i < pMotion->numanimations; i++)
	{
		memset(&pMotion->animations[i], 0, sizeof(modelanimation_t));

		strcpy(pMotion->animations[i].name, animationdescs[i].name);

		pMotion->animations[i].bones = (boneframe_t*)PPAlloc(sizeof(boneframe_t)*boneCount);

		// determine frame count of animation
		int numFrames = animationdescs[i].numFrames / boneCount;

		for(int j = 0; j < boneCount; j++)
		{
			pMotion->animations[i].bones[j].numframes = numFrames;
			pMotion->animations[i].bones[j].keyframes = pMotion->frames + (animationdescs[i].firstFrame + j*numFrames);
		}
	}

	if(anim_frames_decompressed)
	{
		PPFree(animframes);
	}

	PPFree(pStart);

	return pMotion;
}

bool Studio_LoadPhysModel(const char* pszPath, physmodeldata_t* pModel)
{
	memset(pModel, 0, sizeof(physmodeldata_t));

	if(GetFileSystem()->FileExist( pszPath ))
	{
		ubyte* pData = (ubyte*)GetFileSystem()->GetFileBuffer( pszPath );

		ubyte* pStart = pData;

		if(!pData)
			return false;

		DevMsg(2, "Loading POD '%s'\n", pszPath);

		physmodelhdr_t *pHdr = (physmodelhdr_t*)pData;
	
		if(pHdr->ident != PHYSMODEL_ID)
		{
			MsgError("%s is not a POD physics model\n", pszPath);
			PPFree(pData);
			return false;
		}

		if(pHdr->version != PHYSMODEL_VERSION)
		{
			MsgError("POD-File %s has physics model version\n", pszPath);
			PPFree(pData);
			return false;
		}

		pData += sizeof(physmodelhdr_t);

		int nLumps = pHdr->num_lumps;
		for(int lump = 0; lump < nLumps; lump++)
		{
			physmodellump_t* pLump = (physmodellump_t*)pData;
			pData += sizeof(physmodellump_t);

			switch(pLump->type)
			{
				case PHYSLUMP_PROPERTIES:
				{
					physmodelprops_t* props = (physmodelprops_t*)pData;
					pModel->modeltype = props->model_usage;

					DevMsg(2, "PHYSLUMP_PROPERTIES\n");

					break;
				}
				case PHYSLUMP_GEOMETRYINFO:
				{
					int numGeomInfos = pLump->size / sizeof(physgeominfo_t);

					physgeominfo_t* pGeomInfos = (physgeominfo_t*)pData;

					pModel->numshapes = numGeomInfos;
					pModel->shapes = (physmodelshapecache_t*)PPAlloc(numGeomInfos*sizeof(physmodelshapecache_t));
					
					for(int i = 0; i < numGeomInfos; i++)
					{
						pModel->shapes[i].cachedata = NULL;

						// copy shape info
						memcpy(&pModel->shapes[i].shape_info, &pGeomInfos[i], sizeof(physgeominfo_t));
					}
					
					DevMsg(2, "PHYSLUMP_GEOMETRYINFO size = %d (cnt = %d)\n", pLump->size, numGeomInfos);

					break;
				}
				case PHYSLUMP_OBJECTS:
				{
					int numObjInfos = pLump->size / sizeof(physobject_t);
					physobject_t* pObjData = (physobject_t*)pData;

					pModel->numobjects = numObjInfos;
					pModel->objects = (physobjectdata_t*)PPAlloc(numObjInfos*sizeof(physobjectdata_t));

					for(int i = 0; i < numObjInfos; i++)
					{
						// copy shape info
						memcpy(&pModel->objects[i].object, &pObjData[i], sizeof(physobject_t));

						for(int j = 0; j < MAX_GEOM_PER_OBJECT; j++)
						{
							pModel->objects[i].shapeCache[j] = 0;
						}
					}

					DevMsg(2, "PHYSLUMP_OBJECTS size = %d (cnt = %d)\n", pLump->size, numObjInfos);

					break;
				}
				case PHYSLUMP_JOINTDATA:
				{
					int numJointInfos = pLump->size / sizeof(physjoint_t);
					physjoint_t* pJointData = (physjoint_t*)pData;

					pModel->numjoints = numJointInfos;

					if(pModel->numjoints)
					{
						pModel->joints = (physjoint_t*)PPAlloc(pLump->size);
						memcpy(pModel->joints, pJointData, pLump->size );
					}

					DevMsg(2, "PHYSLUMP_JOINTDATA\n");

					break;
				}
				case PHYSLUMP_VERTEXDATA:
				{
					int numVerts = pLump->size / sizeof(Vector3D);
					Vector3D* pVertexData = (Vector3D*)pData;

					pModel->numVertices = numVerts;
					pModel->vertices = (Vector3D*)PPAlloc(pLump->size);
					memcpy(pModel->vertices, pVertexData, pLump->size );

					DevMsg(2, "PHYSLUMP_VERTEXDATA\n");

					break;
				}
				case PHYSLUMP_INDEXDATA:
				{
					int numIndices = pLump->size / sizeof(int);
					int* pIndexData = (int*)pData;

					pModel->numIndices = numIndices;
					pModel->indices = (int*)PPAlloc(pLump->size);
					memcpy(pModel->indices, pIndexData, pLump->size );

					DevMsg(2, "PHYSLUMP_INDEXDATA\n");

					break;
				}
				default:
				{
					MsgWarning("Invalid physmodel lump type. Please update engine.\n");
					break;
				}
			}

			pData += pLump->size;
		}

		PPFree(pStart);
	}
	else
	{
		return false;
	}

	return true;
}

void Studio_FreeMotionData(studiomotiondata_t* pData, int numBones)
{
	for(int i = 0; i < pData->numanimations; i++)
	{
		//for(int j = 0; j < numBones; j++)
			//PPFree(pData->animations[i].bones[j].keyframes);

		PPFree(pData->animations[i].bones);
	}

	PPFree(pData->frames);
	PPFree(pData->sequences);
	PPFree(pData->events);
	PPFree(pData->posecontrollers);
	PPFree(pData->animations);

	PPFree(pData);
}

void Studio_FreePhysModel(physmodeldata_t* pModel)
{
	DevMsg(2, "Studio_FreePhysModel()\n");

	PPFree(pModel->indices);
	PPFree(pModel->vertices);
	PPFree(pModel->shapes);
	PPFree(pModel->objects);
	PPFree(pModel->joints);

	memset(pModel, 0, sizeof(physmodeldata_t));
}