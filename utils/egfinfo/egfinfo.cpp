//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description://////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>

#include "Platform.h"
#include "Utils/strtools.h"
#include "IDkCore.h"
#include "Utils/Align.h"
#include "DebugInterface.h"
#include "cmdlib.h"

#include <iostream>
#include <malloc.h>

#ifdef _WIN32
#include <tchar.h>
#include <crtdbg.h>
#else
#include <unistd.h>
#endif

#include "modelloader_shared.h"
#include "dsm_loader.h"

const char* GetFormatName(int id)
{
	switch(id)
	{
		case EQUILIBRIUM_MODEL_SIGNATURE:
			return "Equilibrium Geometry Format (EGF)";
#ifdef EQUILIBRIUMFX_MODEL_FORMAT
		case EQUILIBRIUMFX_MODEL_SIGNATURE:
			return "Equilibrium FX Geometry Format (EGX)";
#endif
#ifdef SUNSHINE_MODEL_FORMAT
		case SUNSHINE_MODEL_SIGNATURE:
			return "Sunshine Geometry Format (SGF)";
#endif
#ifdef SUNSHINEFX_MODEL_FORMAT
		case SUNSHINEFX_MODEL_SIGNATURE:
			return "Sunshine FX Geometry Format (SGX)";
#endif
	}
	return "invalid format";
}

void cc_egf_info_f(DkList<EqString> *args)
{
	if(args)
	{
		if(args->numElem() <= 0)
		{
			MsgWarning("Example: egf_info <filename>.egf");
			return;
		}

		modelheader_t* pHdr = Studio_LoadModel( (char*)args->ptr()[0].GetData() );

		if(!pHdr)
			return;

		MsgAccept("'%s' loaded as:\n%s\n",args->ptr()[0].GetData(), GetFormatName( pHdr->ident ));

		MsgAccept("Version: %d\n\n",pHdr->version);
		Msg("Num. of models: %d\n",pHdr->nummodels);
		Msg("Num. of bones: %d\n",pHdr->numbones);

		Msg(" \nGroup info:\n");
		int vertexNum = 0;
		int trisNum = 0;
		for(int i = 0; i < pHdr->nummodels;i++)
		{
			studiomodeldesc_t* pModel = pHdr->pModelDesc(i);
			Msg(" Model reference %i\n",i);

			Msg("  Group count: %d\n", pModel->numgroups);

			for(int j = 0; j < pModel->numgroups; j++)
			{
				Msg("    Group %d num. of vertices: %d\n", j, pModel->pGroup(j)->numvertices);
				Msg("    Group %d num. of triangles: %d\n \n", j , pModel->pGroup(j)->numindices / 3);

				vertexNum += pModel->pGroup(j)->numvertices;
				trisNum += pModel->pGroup(j)->numindices / 3;
			}
		}

		Msg("Total vertices: %d\n",vertexNum);
		Msg("Total triangles: %d\n",trisNum);

		Msg(" \nBone info:\n");
		for(int i = 0; i < pHdr->numbones;i++)
		{
			bonedesc_t* pBone = pHdr->pBone(i);
			Msg(" Bone '%s'\n",pBone->name);
			Msg("    Parent bone index: %d\n \n",pBone->parent);
		}

		PPFree(pHdr);
	}
}
ConCommand cc_egf_info("egf",cc_egf_info_f,"Print information about the model");

void DumpModel( studiohdr_t* pHDR, studiomodeldesc_t* pModel, const char* pszFileName )
{
	dsmmodel_t* pDSMModel = new dsmmodel_t;

	for(int i = 0; i < pModel->numgroups; i++)
	{
		modelgroupdesc_t* pGroupDesc = pModel->pGroup(i);

		int mat_index = pGroupDesc->materialIndex;
		EqString material( pHDR->pMaterial(mat_index)->materialname );

		dsmgroup_t* pGroup = new dsmgroup_t;
		pDSMModel->groups.append(pGroup);

		pGroup->usetriangleindices = true;

		strcpy(pGroup->texture, material.GetData());

		pGroup->verts.resize( pGroupDesc->numvertices );

		for(int j = 0; j < pGroupDesc->numvertices; j++)
		{
			dsmvertex_t vert;

			vert.position = pGroupDesc->pVertex(j)->point;
			vert.normal = pGroupDesc->pVertex(j)->normal;
			vert.texcoord = pGroupDesc->pVertex(j)->texCoord;

			memcpy(vert.weights, pGroupDesc->pVertex(j)->boneweights.weight, sizeof(float)*4);
			vert.weight_bones[0] = pGroupDesc->pVertex(j)->boneweights.bones[0];
			vert.weight_bones[1] = pGroupDesc->pVertex(j)->boneweights.bones[1];
			vert.weight_bones[2] = pGroupDesc->pVertex(j)->boneweights.bones[2];
			vert.weight_bones[3] = pGroupDesc->pVertex(j)->boneweights.bones[3];

			vert.numWeights = pGroupDesc->pVertex(j)->boneweights.numweights;

			pGroup->verts.append( vert );
		}

		pGroup->verts.resize( pGroupDesc->numindices );

		for(int j = 0; j < pGroupDesc->numindices; j++)
			pGroup->indices.append( (int)(*pGroupDesc->pVertexIdx(j)) );

		MsgInfo("Group %d verts: %d tris: %d material: %s\n", i, pGroupDesc->numvertices, pGroupDesc->numindices / 3, material.GetData());
	}

	MsgInfo("Writing DSM '%s'\n", pszFileName);

	SaveSharedModel( pDSMModel, (char*)pszFileName );

	FreeDSM(pDSMModel);
}

void DumpPhysModel( physmodeldata_t* pModel, const char* pszFileName )
{
	dsmmodel_t* pDSMModel = new dsmmodel_t;

	dsmgroup_t* pGroup = new dsmgroup_t;
	pDSMModel->groups.append(pGroup);

	pGroup->usetriangleindices = true;

	strcpy(pGroup->texture, "physics");

	pGroup->verts.resize( pModel->numVertices );
	pGroup->indices.resize( pModel->numIndices );

	for(int i = 0; i < pModel->numVertices; i++)
	{
		dsmvertex_t vert;
		vert.position = pModel->vertices[i];

		pGroup->verts.append( vert );
	}

	for(int i = 0; i < pModel->numIndices; i++)
	{
		pGroup->indices.append(pModel->indices[i]);
	}

	MsgInfo("Writing DSM '%s'\n", pszFileName);

	SaveSharedModel( pDSMModel, (char*)pszFileName );

	FreeDSM(pDSMModel);
}

void cc_decompile_f(DkList<EqString> *args)
{
	if(args)
	{
		if(args->numElem() <= 0)
		{
			MsgWarning("Example: egf_info <filename>.egf");
			return;
		}

		modelheader_t* pHdr = Studio_LoadModel( (char*)args->ptr()[0].GetData() );

		if(!pHdr)
			return;

		EqString egf_fileName(UTIL_GetFileName( args->ptr()[0].GetData() ));
		egf_fileName = egf_fileName.StripFileExtension();

		EqString decmodel_name = (char*)args->ptr()[0].GetData();
		decmodel_name.Insert("decompiled_", 0);

        int hierarchy_size = UTIL_GetNumSlashes( decmodel_name.GetData() );
        for (int j = 1; j < hierarchy_size; j++)
        {
            EqString dirName(UTIL_GetDirectoryNameEx( decmodel_name.GetData(), j));

			dirName = dirName.StripFileExtension();

            GetFileSystem()->MakeDir(dirName.GetData(), SP_ROOT);
        }

		EqString outPath = UTIL_GetDirectoryNameEx( decmodel_name.GetData(), hierarchy_size-1);

		MsgAccept("'%s' loaded\n\n",args->ptr()[0].GetData());
		MsgWarning("*** Decompile output path: %s\n", outPath.GetData());

		// write a esc script
		KeyValues kv;

		kvsection_t* pSection = kv.GetRootSection()->AddSection("CompileParams");
		pSection->SetKey("ModelFileName", (char*)args->ptr()[0].GetData());
		pSection->SetKey("Original_ModelFileName", pHdr->modelname);

		for(int i = 0; i < pHdr->numsearchpathdescs; i++)
		{
			kvkeyvaluepair_t* pKey = new kvkeyvaluepair_t;
			pSection->pKeys.append(pKey);

			strcpy(pKey->keyname, "materialpath");
			strcpy(pKey->value, pHdr->pMaterialSearchPath(i)->m_szSearchPathString);
		}

		for(int i = 0; i < pHdr->numbodygroups; i++)
		{
			kvkeyvaluepair_t* pKey;

			int model_id = pHdr->pBodyGroups(i)->lodmodel_index;

			// dump models
			for(int j = 0; j < 8; j++)
			{
				int8 refmodel_id = pHdr->pLodModel(model_id)->lodmodels[j];

				if(refmodel_id == -1)
					break;

				EqString dsmmodel_name(egf_fileName + "_" + _Es(pHdr->pBodyGroups(i)->name) + varargs("_ref_%d", refmodel_id));
				EqString dsmmodel_file(dsmmodel_name + ".obj");
				EqString dsmmodel_path(outPath + "/" + dsmmodel_file);

				DumpModel( pHdr, pHdr->pModelDesc(refmodel_id), dsmmodel_path.GetData() );

				pKey = new kvkeyvaluepair_t;
				pSection->pKeys.append(pKey);

				strcpy(pKey->keyname, "model");
				strcpy(pKey->value, varargs("%s %s", dsmmodel_name.GetData(), dsmmodel_file.GetData()));
			}

			pKey = new kvkeyvaluepair_t;
			pSection->pKeys.append(pKey);

			strcpy(pKey->keyname, "bodygroup");

			EqString firstmodel_name(egf_fileName + "_" + _Es(pHdr->pBodyGroups(i)->name) + varargs("_ref_%d", 0));

			strcpy(pKey->value, varargs("%s %s", pHdr->pBodyGroups(i)->name, firstmodel_name.GetData()));
		}

		// try open POD file
		EqString pod_file((char*)args->ptr()[0].GetData());
		pod_file = pod_file.StripFileExtension() + ".pod";

		physmodeldata_t physmodel;
		if(Studio_LoadPhysModel(pod_file.GetData(), &physmodel))
		{
			kvsection_t* pPhysSection = pSection->AddSection("Physics");

			EqString physmodel_name(outPath + "/" + egf_fileName + "_physmodel.obj");

			pPhysSection->SetKey("model", (char*)(egf_fileName + "_physmodel.obj").GetData());
			pPhysSection->SetKey("mass", varargs("%g", physmodel.objects[0].mass));
			pPhysSection->SetKey("surfaceprops", physmodel.objects[0].surfaceprops);
			pPhysSection->SetKey("compound", "1");

			DumpPhysModel( &physmodel, physmodel_name.GetData());
		}

		EqString esc_path( decmodel_name.StripFileExtension() + ".esc" );

		Msg("ESC File: %s\n", esc_path.GetData());

		kv.SaveToFile(esc_path.GetData(), SP_ROOT);

		PPFree(pHdr);
	}
}
ConCommand cc_decompile("decompile", cc_decompile_f, "Decompiles model");

int main(int argc, char **argv)
{
	//Only set debug info when connecting dll
	#ifdef _DEBUG
		#define _CRTDBG_MAP_ALLOC
		int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
		flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
		flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
		flag |= _CRTDBG_ALLOC_MEM_DF;
		_CrtSetDbgFlag(flag); // Set flag to the new value
		_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	#endif

#ifdef _WIN32
	Install_SpewFunction();
#endif

	// Core initialization test was passes successfully
	GetCore()->Init("model_info",argc,argv);

	// Filesystem is first!
	if(!GetFileSystem()->Init(false))
	{
		GetCore()->Shutdown();
		return 0;
	}

	// Command line execution test was passes not successfully
	GetCmdLine()->ExecuteCommandLine(true,true);

	GetCore()->Shutdown();
	return 0;
}
