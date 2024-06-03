//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Shared Model loader, obj support
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "utils/Tokenizer.h"
#include "utils/KeyValues.h"
#include "math/Utility.h"
#include "dsm_obj_loader.h"
#include "dsm_loader.h"

#define MAX_VERTS_PER_POLYGON 16

namespace SharedModel
{

extern float readFloat(Tokenizer& tok);
int xstrsplitws(char* str, char** pointer_array)
{
	char c = *str;

	int num_indices = 0;

	bool bAdd = true;

	while (c != '\0')
	{
		c = *str;

		if (bAdd)
		{
			pointer_array[num_indices] = str;
			num_indices++;
			bAdd = false;
		}

		if (CType::IsSpace(c))
		{
			bAdd = true;
			*str = '\0'; // make null-string
		}

		str++;
	}

	return num_indices;
}

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


struct obj_material_t
{
	char name[1024];
	char texture[1024];
};

bool LoadMTL(const char* filename, Array<obj_material_t> &material_list)
{
	Tokenizer tok;

	if (!tok.setFile(filename))
	{
		MsgError("Couldn't open MTL file '%s'", filename);
		return false;
	}

	obj_material_t* current = nullptr;

	char *str;

	while ((str = tok.next()) != nullptr)
	{
		if(str[0] == '#')
		{
			tok.goToNextLine();
			continue;
		}
		else if(!stricmp(str, "newmtl"))
		{
			obj_material_t mat;
			strcpy(mat.name, tok.next(isNotNewLine));
			strcpy(mat.texture, mat.name);

			current = &material_list[material_list.append(mat)];
		}
		else if(!stricmp(str, "map_Kd"))
		{
			if(current)
				strcpy(current->texture, tok.next(isNotNewLine));
		}
		else
			tok.goToNextLine();
	}

	Msg("Num materials: %d\n", material_list.numElem());

	return true;
}

const char* GetMTLTexture(const char* pszMaterial, Array<obj_material_t> &material_list)
{
	//Msg("search for: %s\n", pszMaterial);
	for(int i = 0; i < material_list.numElem(); i++)
	{
		//Msg("search id: %d\n", i);
		if(!strcmp(material_list[i].name, pszMaterial))
			return material_list[i].texture;
	}

	return pszMaterial;
}

bool LoadOBJ(DSModel* model, const char* filename)
{
	Array<obj_material_t> material_list(PP_SL);

	Tokenizer tok;

	char* pBuffer = (char*)g_fileSystem->GetFileBuffer(filename);

	if (!pBuffer)
	{
		MsgError("Couldn't open OBJ file '%s'", filename);
		return false;
	}

	tok.setString(pBuffer);

	// done with it
	PPFree(pBuffer);

	char *str;

	int nVerts = 0;
	int nTexCoords = 0;
	int nNormals = 0;
	int nFaces = 0;

	// read counters for fast alloc
	while ((str = tok.next()) != nullptr)
	{
		if(!stricmp(str, "v"))
		{
			nVerts++;
		}
		else if(!stricmp(str, "vt"))
		{
			nTexCoords++;
		}
		else if(!stricmp(str, "vn"))
		{
			nNormals++;
		}
		else if(!stricmp(str, "f"))
		{
			nFaces++;
		}

		tok.goToNextLine();
	}

	Msg("%d verts, %d normals, %d texcoords, %d faces total in OBJ\n", nVerts, nNormals, nTexCoords, nFaces);

	// reset reader
	tok.reset();

	bool bUseMTL = false;

	// For now, don't use MTL

	//EqString mtl_file_name(filename);
	//mtl_file_name = mtl_file_name.Path_Strip_Ext() + ".mtl";
	//if(LoadMTL((char*)mtl_file_name.GetData(), material_list))
	///	bUseMTL = true;

	model->name = "temp";
	EqString materialName = "error";

	bool bLoaded = false;
	Array<Vector3D> vertices(PP_SL);
	Array<Vector2D> texcoords(PP_SL);
	Array<Vector3D> normals(PP_SL);

	vertices.resize(nVerts);
	texcoords.resize(nTexCoords);
	normals.resize(nNormals);

	DSVertex verts[MAX_VERTS_PER_POLYGON];
	int vindices[MAX_VERTS_PER_POLYGON];
	int tindices[MAX_VERTS_PER_POLYGON];
	int nindices[MAX_VERTS_PER_POLYGON];

	DSMesh* curgroup = nullptr;

	bool gl_to_eq = true;
	bool blend_to_eq = false;

	bool reverseNormals = false;

	int numComments = 0;

	// NOTE: we always invert X axis on import and export

	while (str = tok.next())
	{
		if(str[0] == '#')
		{
			numComments++;

			if (numComments <= 3) // check for blender comment to activate hack
			{
				char* check_tok = tok.next();
				if (check_tok && !stricmp(check_tok, "blender"))
				{
					gl_to_eq = true;
					//blend_to_eq = true;
					//reverseNormals = true;
				}
				else
					str = check_tok;	// check, maybe there is a actual props
			}
		}

		if(str[0] == 'v')
		{
			//char stored_str[3] = {str[0], str[1], 0};

			if(str[1] == 't')
			{
				// parse vector3
				Vector2D t;

				t.x = readFloat(tok);
				t.y = readFloat(tok);

				if(gl_to_eq)
					t.y = 1.0f - t.y;

				texcoords.append(t);
			}
			else if(str[1] == 'n')
			{
				// parse vector3
				Vector3D n;

				n.x = -readFloat(tok);
				n.y = readFloat(tok);
				n.z = readFloat(tok);

				if(blend_to_eq)
					n = Vector3D(n.z, n.y, n.x);

				normals.append(n);
			}
			else
			{
				// parse vector3
				Vector3D v;

				v.x = -readFloat( tok );
				v.y = readFloat( tok );
				v.z = readFloat( tok );

				if(blend_to_eq)
					v = Vector3D(v.z, v.y, v.x);

				vertices.append(v);
			}
		}
		else if(!stricmp(str, "f"))
		{
			if(!curgroup)
			{
				curgroup = PPNew DSMesh;

				model->meshes.append(curgroup);

				if(bUseMTL)
					curgroup->texture = GetMTLTexture(materialName, material_list);
				else
					curgroup->texture = materialName;
			}

			int slashcount = 0;
			bool doubleslash = false;

			bool has_v = false;
			bool has_vt = false;
			bool has_vn = false;

			char string[512];
			string[0] = '\0';

			strcpy(string, tok.nextLine()+1);

			char* str_idxs[MAX_VERTS_PER_POLYGON] = { nullptr };

			xstrsplitws(string, str_idxs);

			int i = 0;

			for(i = 0; i < MAX_VERTS_PER_POLYGON; i++)
			{
				char* pstr = str_idxs[i];

				if(!pstr)
					break;

				int len = strlen( pstr );

				if(len == 0)
					break;

				// detect on first iteration what format of vertex we have
				if(i == 0)
				{
					// face format handling
					slashcount = strchcount(pstr, '/');
					doubleslash = (strstr(pstr,"//") != nullptr);
				}

				// v//vn
				if (doubleslash && (slashcount == 2))
				{
					has_v = has_vn = 1;
					sscanf( pstr,"%d//%d",&vindices[ i ],&nindices[ i ] );
					vindices[ i ] -= 1;
					nindices[ i ] -= 1;
				}
				// v/vt/vn
				else if (!doubleslash && (slashcount == 2))
				{
					has_v = has_vt = has_vn = 1;
					sscanf( pstr,"%d/%d/%d",&vindices[ i ],&tindices[ i ],&nindices[ i ] );

					vindices[ i ] -= 1;
					tindices[ i ] -= 1;
					nindices[ i ] -= 1;
				}
				// v/vt
				else if (!doubleslash && (slashcount == 1))
				{
					has_v = has_vt = 1;
					sscanf( pstr,"%d/%d",&vindices[ i ],&tindices[ i ] );

					vindices[ i ] -= 1;
					tindices[ i ] -= 1;
				}
				// only vertex
				else
				{
					has_v = true;
					vindices[i] = atoi(pstr)-1;
				}
			}

			if (i <= 2)
				continue;
			
			// Map<int, DSVertex> verts;
			// Array<int> indexList;
			

			// triangle fan
			for(int v = 0; v < i; v++)
			{
				DSVertex vert;

				if (!vertices.inRange(vindices[v]))
				{
					ASSERT_FAIL("FIX YOUR OBJ! %d, max is %d\n", vindices[v], vertices.numElem());
					continue;
				}

				verts[v].position = vertices[vindices[v]];

				if(has_vt)
					verts[v].texcoord = texcoords[tindices[v]];
				if(has_vn)
					verts[v].normal = normals[nindices[v]];
			}

			for(int v = 0; v < i-2; v++)
			{
				Vector3D cnormal;
				ComputeTriangleNormal(verts[0].position, verts[v + 1].position, verts[v + 2].position, cnormal);

				if(dot(cnormal, verts[0].normal) < 0.0f)
				{
					reverseNormals = true;
					break;
				}
			}

			DSVertex v0,v1,v2;
			for(int v = 0; v < i-2; v++)
			{
				if(reverseNormals)
				{
					v2 = verts[0];
					v1 = verts[v + 1];
					v0 = verts[v + 2];

					//v0.normal *= -1.0f;
					//v1.normal *= -1.0f;
					//v2.normal *= -1.0f;
				}
				else 
				{
					v0 = verts[0];
					v1 = verts[v + 1];
					v2 = verts[v + 2];
				}

				curgroup->verts.append(v0);
				curgroup->verts.append(v1);
				curgroup->verts.append(v2);
			}

			// skip 'goToNextLine' as we already called 'nextLine'
			continue;
		}
		else if(str[0] == 'g')
		{
			curgroup = nullptr;
			//strcpy(material_name, "no_group_name");
		}
		/*
		else if(str[0] == 's')
		{
			curgroup = nullptr;
			strcpy(material_name, "temp");
			//strcpy(material_name, tok.nextLine());
		}
		*/
		else if(!stricmp(str, "usemtl"))
		{
			curgroup = nullptr;
			materialName = tok.next(isNotNewLine);
		}

		tok.goToNextLine();
	}


	if(normals.numElem() == 0)
	{
		MsgWarning("WARNING: No normals found in %s. Did you forget to export it?\n", filename);
	}

	if(model->meshes.numElem() > 0)
	{
		bLoaded = true;
	}

	return bLoaded;
}

bool SaveOBJ(DSModel* model, const char* filename)
{
	IFilePtr pFile = g_fileSystem->Open(filename, "wt", SP_ROOT);
	if(!pFile)
	{
		MsgError("Failed to open for write '%s'!\n", filename);
		return false;
	}

	// NOTE: we always invert X axis on import and export

	EqString mtlFileName = fnmPathStripExt(filename) + ".mtl";

	pFile->Print("# OBJ generated by E2 dsm_obj_loader.cpp\n\n");
	pFile->Print("mtllib %s\r\n", mtlFileName.ToCString());
	pFile->Print("o %s\n", model->name.Length() ? model->name.ToCString() : fnmPathStripExt(fnmPathStripPath(filename)).ToCString());

	IFilePtr mtlFile = g_fileSystem->Open(mtlFileName, "wt", SP_ROOT);

	Set<int> addedMaterials(PP_SL);

	for(SharedModel::DSMesh* group : model->meshes)
	{
		for(const DSVertex& vert : group->verts)
		{
			const Vector3D v = vert.position;
			pFile->Print("v %f %f %f\n", -v.x, v.y, v.z);
		}

		for (const DSVertex& vert : group->verts)
		{
			const Vector2D t = vert.texcoord;
			pFile->Print("vt %f %f\n", t.x, 1.0f - t.y);
		}

		for (const DSVertex& vert : group->verts)
		{
			const Vector3D n = vert.normal;
			pFile->Print("vn %f %f %f\n", -n.x, n.y, n.z);
		}

		if (group->indices.numElem())
		{
			const int nameHash = StringToHash(group->texture);
			if (addedMaterials.find(nameHash).atEnd())
			{
				mtlFile->Print("newmtl %s\r\n", group->texture.ToCString());
				KVSection matSection;
				if (KV_LoadFromFile("materials/" + group->texture + ".mat", SP_MOD, &matSection))
				{
					EqString textureName = KV_GetValueString(matSection.FindSection("basetexture"), 0, group->texture) + _Es(".dds");
					mtlFile->Print("map_Kd %s\r\n", g_fileSystem->GetAbsolutePath(SP_MOD, "materials/" + textureName).ToCString());
				}
				else
				{
					// NOTE: guessed, could be different path.
					mtlFile->Print("map_Kd %s\r\n", g_fileSystem->GetAbsolutePath(SP_MOD, (group->texture + ".dds")).ToCString());
				}

				addedMaterials.insert(nameHash);
			}

			pFile->Print("usemtl %s\n", group->texture.ToCString());

		}

		for (int j = 0; j < group->indices.numElem(); j += 3)
		{
			const int indices[] = {
				group->indices[j+2] + 1,
				group->indices[j+1] + 1,
				group->indices[j+0] + 1
			};

			pFile->Print("f %d/%d/%d %d/%d/%d %d/%d/%d\n",	indices[0], indices[0], indices[0],
															indices[1], indices[1], indices[1],
															indices[2], indices[2], indices[2]);
		}
	}

	return true;
}

}