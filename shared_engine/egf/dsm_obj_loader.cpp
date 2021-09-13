//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Shared Model loader, obj support
//////////////////////////////////////////////////////////////////////////////////

#include "dsm_obj_loader.h"

#include "core/DebugInterface.h"
#include "core/IFileSystem.h"
#include "utils/Tokenizer.h"

#include "math/Utility.h"

#include <stdio.h>

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

		if (isspace(c))
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

bool LoadMTL(const char* filename, DkList<obj_material_t> &material_list)
{
	Tokenizer tok;

	if (!tok.setFile(filename))
	{
		MsgError("Couldn't open MTL file '%s'", filename);
		return false;
	}

	obj_material_t* current = NULL;

	char *str;

	while ((str = tok.next()) != NULL)
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

char* GetMTLTexture(char* pszMaterial, DkList<obj_material_t> &material_list)
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

bool LoadOBJ(dsmmodel_t* model, const char* filename)
{
	DkList<obj_material_t> material_list;

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

	int nVerts = 0;
	int nTexCoords = 0;
	int nNormals = 0;
	int nFaces = 0;

	// read counters for fast alloc
	while ((str = tok.next()) != NULL)
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

	strcpy(model->name, "temp");

	bool bLoaded = false;

	char material_name[1024];
	strcpy(material_name, "error");

	DkList<Vector3D> vertices;
	DkList<Vector2D> texcoords;
	DkList<Vector3D> normals;

	vertices.resize(nVerts);
	texcoords.resize(nTexCoords);
	normals.resize(nNormals);

	dsmgroup_t* curgroup = NULL;
	//int*		group_remap = new int[nVerts];

	bool gl_to_eq = true;
	bool blend_to_eq = false;

	bool reverseNormals = false;

	int numComments = 0;

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
					blend_to_eq = true;
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

				n.x = readFloat(tok);
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

				v.x = readFloat( tok );
				v.y = readFloat( tok );
				v.z = readFloat( tok );

				if(blend_to_eq)
					v = Vector3D(v.z, v.y, v.x);

				vertices.append(v);
			}
		}
		else if(!stricmp(str, "f"))
		{
			Msg("face!\n");

			if(!curgroup)
			{
				curgroup = new dsmgroup_t;

				model->groups.append(curgroup);

				if(bUseMTL)
					strcpy(curgroup->texture, GetMTLTexture(material_name, material_list));
				else
					strcpy(curgroup->texture, material_name);
			}

			int vindices[32];
			int tindices[32];
			int nindices[32];

			int slashcount = 0;
			bool doubleslash = false;

			bool has_v = false;
			bool has_vt = false;
			bool has_vn = false;

			char string[512];
			string[0] = '\0';

			strcpy(string, tok.nextLine()+1);

			char* str_idxs[32] = {NULL};

			xstrsplitws(string, str_idxs);

			int i = 0;

			for(i = 0; i < 32; i++)
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
					doubleslash = (strstr(pstr,"//") != NULL);
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

			if(i > 2)
			{
				dsmvertex_t verts[32];

				// triangle fan
				for(int v = 0; v < i; v++)
				{
					dsmvertex_t vert;

					if (!vertices.inRange(vindices[v]))
					{
						ErrorMsg("FIX YOUR OBJ! %d, max is %d\n", vindices[v], vertices.numElem());
						continue;
					}

					verts[v].position = vertices[vindices[v]];

					if(has_vt)
						verts[v].texcoord = texcoords[tindices[v]];
					if(has_vn)
						verts[v].normal = normals[nindices[v]];
				}

				if(blend_to_eq)
				{
					for(int v = 0; v < i-2; v++)
					{
						Vector3D cnormal;
						ComputeTriangleNormal(verts[0].position, verts[v + 2].position, verts[v + 1].position, cnormal);

						if(dot(cnormal, verts[0].normal) < 0.0f)
						{
							reverseNormals = true;
						}
					}

					dsmvertex_t v0,v1,v2;
					for(int v = 0; v < i-2; v++)
					{
						v0 = verts[0];
						v1 = verts[v+2];
						v2 = verts[v+1];

						if(reverseNormals)
						{
							v0.normal *= -1.0f;
							v1.normal *= -1.0f;
							v2.normal *= -1.0f;
						}

						curgroup->verts.append(v0);
						curgroup->verts.append(v1);
						curgroup->verts.append(v2);
					}
				}
				else
				{
					for(int v = 0; v < i-2; v++)
					{
						Vector3D cnormal;
						ComputeTriangleNormal(verts[0].position, verts[v + 1].position, verts[v + 2].position, cnormal);

						if(dot(cnormal, verts[0].normal) < 0.0f)
						{
							reverseNormals = true;
						}
					}

					dsmvertex_t v0,v1,v2;
					for(int v = 0; v < i-2; v++)
					{
						v0 = verts[0];
						v1 = verts[v+1];
						v2 = verts[v+2];

						if(reverseNormals)
						{
							v0.normal *= -1.0f;
							v1.normal *= -1.0f;
							v2.normal *= -1.0f;
						}

						curgroup->verts.append(v0);
						curgroup->verts.append(v1);
						curgroup->verts.append(v2);
					}
				}
			}

			// skip 'goToNextLine' as we already called 'nextLine'
			continue;
		}
		else if(str[0] == 'g')
		{
			curgroup = NULL;
			//strcpy(material_name, "no_group_name");
		}
		/*
		else if(str[0] == 's')
		{
			curgroup = NULL;
			strcpy(material_name, "temp");
			//strcpy(material_name, tok.nextLine());
		}
		*/
		else if(!stricmp(str, "usemtl"))
		{
			curgroup = NULL;
			strcpy(material_name, tok.next(isNotNewLine));
		}

		tok.goToNextLine();
	}

	//delete [] group_remap;

	if(normals.numElem() == 0)
	{
		MsgWarning("WARNING: No normals found in %s. Did you forget to export it?\n", filename);
	}

	if(model->groups.numElem() > 0)
	{
		bLoaded = true;
	}

	return bLoaded;
}

bool SaveOBJ(dsmmodel_t* model, const char* filename)
{
	IFile* pFile = g_fileSystem->Open(filename, "wt");
	if(!pFile)
	{
		MsgError("Failed to open for write '%s'!\n", filename);

		return false;
	}

	FILE* pFileHandle = *((FILE**)pFile);
	fprintf(pFileHandle, "# DSM_OBJ_LOADER.CPP OBJ FILE\n\n");

	for(int i = 0; i < model->groups.numElem(); i++)
	{
		fprintf(pFileHandle, "o %s\n", model->groups[i]->texture);

		for(int j = 0; j < model->groups[i]->verts.numElem(); j++)
		{
			Vector3D v = model->groups[i]->verts[j].position;
			fprintf(pFileHandle, "v %g %g %g\n", v.x, v.y, v.z);
			/*
			Vector2D t = model->groups[i]->verts[j].texcoord;
			t.y = 1.0f - t.y;
			fprintf(pFileHandle, "vt %g %g\n", t.x, t.y);

			Vector3D n = model->groups[i]->verts[j].normal;
			fprintf(pFileHandle, "vn %g %g %g\n", n.x, n.y, n.z);
			*/
		}


		for(int j = 0; j < model->groups[i]->verts.numElem(); j++)
		{
			Vector2D t = model->groups[i]->verts[j].texcoord;
			t.y = 1.0f - t.y;
			fprintf(pFileHandle, "vt %g %g\n", t.x, t.y);
		}

		for(int j = 0; j < model->groups[i]->verts.numElem(); j++)
		{
			Vector3D n = model->groups[i]->verts[j].normal;
			fprintf(pFileHandle, "vn %g %g %g\n", n.x, n.y, n.z);
		}


		fprintf(pFileHandle, "usemtl %s\n", model->groups[i]->texture);

		for(int j = 0; j < model->groups[i]->indices.numElem(); j+=3)
		{
			int indices[3];
			indices[0] = model->groups[i]->indices[j] + 1;
			indices[1] = model->groups[i]->indices[j+1] + 1;
			indices[2] = model->groups[i]->indices[j+2] + 1;

			fprintf(pFileHandle, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",	indices[0], indices[0], indices[0],
																	indices[1], indices[1], indices[1],
																	indices[2], indices[2], indices[2]);
		}
	}

	g_fileSystem->Close(pFile);

	return true;
}

}