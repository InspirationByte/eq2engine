//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Graphics File loader
//////////////////////////////////////////////////////////////////////////////////

#include "modelloader_shared.h"

#include "core/DebugInterface.h"
#include "core/IFileSystem.h"

#include <zlib.h>

bool IsValidModelIdentifier(int id)
{
	if(EQUILIBRIUM_MODEL_SIGNATURE == id)
		return true;

	return false;
}

void ConvertHeaderToLatestVersion(basemodelheader_t* pHdr)
{

}

// loads all supported EGF model formats
studiohdr_t* Studio_LoadModel(const char* pszPath)
{
	long len = 0;

	IFile* file = g_fileSystem->Open(pszPath, "rb");

	if(!file)
	{
		MsgError("Can't open model file '%s'\n",pszPath);
		return NULL;
	}

	len = file->GetSize();

	char* _buffer = (char*)PPAlloc(len+32); // +32 bytes for conversion issues

	file->Read(_buffer, 1, len);
	g_fileSystem->Close(file);

	basemodelheader_t* pBaseHdr = (basemodelheader_t*)_buffer;

	if(!IsValidModelIdentifier( pBaseHdr->ident ))
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
		MsgError("Wrong model '%s' version, excepted %i, but model version is %i\n",pszPath, EQUILIBRIUM_MODEL_VERSION,pBaseHdr->version);
		PPFree(_buffer);
		return NULL;
	}

	if(len != pHdr->length)
	{
		MsgError("Model is not valid (%d versus %d in header)!\n",len,pBaseHdr->size);
		PPFree(_buffer);
		return NULL;
	}

	/*
#ifndef EDITOR
	char* str = (char*)stackalloc(strlen(pszPath+1));
	strcpy(str, pszPath);
	FixSlashes(str);

	if(stricmp(str, pHdr->modelName))
	{
		MsgError("Model %s is not valid model, didn't you replaced model?\n", pszPath);
		CacheFree(hunkHiMark);
		return NULL;
	}
#endif
	*/


	return pHdr;
}

studioMotionData_t* Studio_LoadMotionData(const char* pszPath, int boneCount)
{
	ubyte* pData = (ubyte*)g_fileSystem->GetFileBuffer(pszPath);
	ubyte* pStart = pData;

	if(!pData)
	{
		return NULL;
	}

	DevMsg(DEVMSG_CORE,"Loading %s motion package\n", pszPath);

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

	studioMotionData_t* pMotion = (studioMotionData_t*)PPAlloc(sizeof(studioMotionData_t));

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
				DevMsg(DEVMSG_CORE,"all animations: %d\n", numAnimDescs);

				break;
			}
			case ANIMCA_ANIMATIONFRAMES:
			{
				numAnimFrames = pLump->size / sizeof(animframe_t);
				animframes = (animframe_t*)pData;

				DevMsg(DEVMSG_CORE,"all animframes: %d\n", numAnimFrames);

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
					DevMsg(DEVMSG_CORE,"all animframes: %d\n", numAnimFrames);

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

				DevMsg(DEVMSG_CORE,"sequence descs: %d\n", pMotion->numsequences);

				break;
			}
			case ANIMCA_EVENTS:
			{
				pMotion->numEvents = pLump->size / sizeof(sequenceevent_t);

				pMotion->events = (sequenceevent_t*)PPAlloc(pLump->size);
				memcpy(pMotion->events, pData, pLump->size);

				DevMsg(DEVMSG_CORE,"events: %d\n", pMotion->numEvents);

				break;
			}
			case ANIMCA_POSECONTROLLERS:
			{
				pMotion->numPoseControllers = pLump->size / sizeof(posecontroller_t);

				pMotion->poseControllers = (posecontroller_t*)PPAlloc(pLump->size);
				memcpy(pMotion->poseControllers, pData, pLump->size);

				DevMsg(DEVMSG_CORE,"pose controllers: %d\n", pMotion->numPoseControllers);

				break;
			}
		}

		pData += pLump->size;
	}

	// first processing done, convert animca animations to EGF format.
	pMotion->animations = (studioAnimation_t*)PPAlloc(sizeof(studioAnimation_t)*numAnimDescs);

	//Msg("Num anim descs: %d\n", numAnimDescs);

	pMotion->numAnimations = numAnimDescs;

	pMotion->frames = (animframe_t*)PPAlloc(numAnimFrames * sizeof(animframe_t));
	memcpy(pMotion->frames, animframes, numAnimFrames * sizeof(animframe_t));

	for(int i = 0; i < pMotion->numAnimations; i++)
	{
		memset(&pMotion->animations[i], 0, sizeof(studioAnimation_t));

		strcpy(pMotion->animations[i].name, animationdescs[i].name);

		pMotion->animations[i].bones = (studioBoneFrame_t*)PPAlloc(sizeof(studioBoneFrame_t)*boneCount);

		// determine frame count of animation
		int numFrames = animationdescs[i].numFrames / boneCount;

		for(int j = 0; j < boneCount; j++)
		{
			pMotion->animations[i].bones[j].numFrames = numFrames;
			pMotion->animations[i].bones[j].keyFrames = pMotion->frames + (animationdescs[i].firstFrame + j*numFrames);
		}
	}

	if(anim_frames_decompressed)
	{
		PPFree(animframes);
	}

	PPFree(pStart);

	return pMotion;
}

bool Studio_LoadPhysModel(const char* pszPath, studioPhysData_t* pModel)
{
	memset(pModel, 0, sizeof(studioPhysData_t));

	if(g_fileSystem->FileExist( pszPath ))
	{
		ubyte* pData = (ubyte*)g_fileSystem->GetFileBuffer( pszPath );

		ubyte* pStart = pData;

		if(!pData)
			return false;

		DevMsg(DEVMSG_CORE, "Loading POD '%s'\n", pszPath);

		physmodelhdr_t *pHdr = (physmodelhdr_t*)pData;

		if(pHdr->ident != PHYSMODEL_ID)
		{
			MsgError("'%s' is not a POD physics model\n", pszPath);
			PPFree(pData);
			return false;
		}

		if(pHdr->version != PHYSMODEL_VERSION)
		{
			MsgError("POD-File '%s' has physics model version\n", pszPath);
			PPFree(pData);
			return false;
		}

		Array<EqString> objectNames{ PP_SL };

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

					DevMsg(DEVMSG_CORE, "PHYSLUMP_PROPERTIES\n");

					break;
				}
				case PHYSLUMP_GEOMETRYINFO:
				{
					int numGeomInfos = pLump->size / sizeof(physgeominfo_t);

					physgeominfo_t* pGeomInfos = (physgeominfo_t*)pData;

					pModel->numShapes = numGeomInfos;
					pModel->shapes = (studioPhysShapeCache_t*)PPAlloc(numGeomInfos*sizeof(studioPhysShapeCache_t));

					for(int i = 0; i < numGeomInfos; i++)
					{
						pModel->shapes[i].cachedata = NULL;

						// copy shape info
						memcpy(&pModel->shapes[i].shape_info, &pGeomInfos[i], sizeof(physgeominfo_t));
					}

					DevMsg(DEVMSG_CORE, "PHYSLUMP_GEOMETRYINFO size = %d (cnt = %d)\n", pLump->size, numGeomInfos);

					break;
				}
				case PHYSLUMP_OBJECTNAMES:
				{
					char* name = (char*)pData;

					int len = strlen(name);
					int sz = 0;

					do
					{
						char* str = name+sz;

						len = strlen(str);

						if(len > 0)
							objectNames.append(str);

						sz += len + 1;
					}while(sz < pLump->size);

					DevMsg(DEVMSG_CORE, "PHYSLUMP_OBJECTNAMES size = %d (cnt = %d)\n", pLump->size, objectNames.numElem());

					break;
				}
				case PHYSLUMP_OBJECTS:
				{
					int numObjInfos = pLump->size / sizeof(physobject_t);
					physobject_t* physObjDataLump = (physobject_t*)pData;

					pModel->numObjects = numObjInfos;
					pModel->objects = (studioPhysObject_t*)PPAlloc(numObjInfos*sizeof(studioPhysObject_t));

					for(int i = 0; i < numObjInfos; i++)
					{
						studioPhysObject_t& objData = pModel->objects[i];

						if(objectNames.numElem() > 0)
							strcpy(objData.name, objectNames[i].ToCString());

						// copy shape info
						memcpy(&objData.object, &physObjDataLump[i], sizeof(physobject_t));

						for(int j = 0; j < MAX_GEOM_PER_OBJECT; j++)
							objData.shapeCache[j] = nullptr;
					}

					DevMsg(DEVMSG_CORE, "PHYSLUMP_OBJECTS size = %d (cnt = %d)\n", pLump->size, numObjInfos);

					break;
				}
				case PHYSLUMP_JOINTDATA:
				{
					int numJointInfos = pLump->size / sizeof(physjoint_t);
					physjoint_t* pJointData = (physjoint_t*)pData;

					pModel->numJoints = numJointInfos;

					if(pModel->numJoints)
					{
						pModel->joints = (physjoint_t*)PPAlloc(pLump->size);
						memcpy(pModel->joints, pJointData, pLump->size );
					}

					DevMsg(DEVMSG_CORE, "PHYSLUMP_JOINTDATA\n");

					break;
				}
				case PHYSLUMP_VERTEXDATA:
				{
					int numVerts = pLump->size / sizeof(Vector3D);
					Vector3D* pVertexData = (Vector3D*)pData;

					pModel->numVertices = numVerts;
					pModel->vertices = (Vector3D*)PPAlloc(pLump->size);
					memcpy(pModel->vertices, pVertexData, pLump->size );

					DevMsg(DEVMSG_CORE, "PHYSLUMP_VERTEXDATA\n");

					break;
				}
				case PHYSLUMP_INDEXDATA:
				{
					int numIndices = pLump->size / sizeof(int);
					int* pIndexData = (int*)pData;

					pModel->numIndices = numIndices;
					pModel->indices = (int*)PPAlloc(pLump->size);
					memcpy(pModel->indices, pIndexData, pLump->size );

					DevMsg(DEVMSG_CORE, "PHYSLUMP_INDEXDATA\n");

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
	}
	else
	{
		return false;
	}

	return true;
}

void Studio_FreeMotionData(studioMotionData_t* data, int numBones)
{
	for(int i = 0; i < data->numAnimations; i++)
	{
		//for(int j = 0; j < numBones; j++)
			//PPFree(pData->animations[i].bones[j].keyFrames);

		PPFree(data->animations[i].bones);
	}

	PPFree(data->frames);
	PPFree(data->sequences);
	PPFree(data->events);
	PPFree(data->poseControllers);
	PPFree(data->animations);
}

void Studio_FreePhysModel(studioPhysData_t* model)
{
	PPFree(model->indices);
	PPFree(model->vertices);
	PPFree(model->shapes);
	PPFree(model->objects);
	PPFree(model->joints);
}
