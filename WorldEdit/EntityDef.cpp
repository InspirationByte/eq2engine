//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Editor definition file
//////////////////////////////////////////////////////////////////////////////////

#include "EntityDef.h"
#include "materialsystem/IMaterialSystem.h"

#include "ILocalize.h"


DkList<edef_entity_t*>	g_defs;
EqString				g_entdefname("NULL");
edef_entity_t*			g_baseentity;

EqWString LocalizedEDEF(char *pszkey, kvkeybase_t *sec)
{
	kvkeybase_t *pair = sec->FindKeyBase(pszkey);
	if(pair)
	{
		const char* value = KV_GetValueString(pair);

		if(value[0] == '#')
			return DKLOC(value +1, EqWString(value).c_str());
		else
			return value;
	}
	else
		return L"";
}

Vector2D UTIL_StringToVector2(const char *str)
{
	Vector2D vec(0,0);

	sscanf(str, "%g %g", &vec.x, &vec.y);

	return vec;
}

ColorRGB UTIL_StringToColor3(const char *str)
{
	//Msg("UTIL_StringToColor3 input: %s\n", str);
	ColorRGB col(0,0.5f,0);

	sscanf(str, "%g %g %g", &col.x, &col.y, &col.z);

	//Msg("UTIL_StringToColor3 output: %g %g %g\n", col.x, col.y, col.z);

	return col;
}

ColorRGBA UTIL_StringToColor4(const char *str)
{
	ColorRGBA col(0,0.5f,0,1);

	sscanf(str, "%g %g %g %g", &col.x, &col.y, &col.z, &col.w);

	return col;
}

eDef_ParamType_e EDef_ResolveParamTypeFromString(const char* pStr)
{
	for(int i = 0; i < PARAM_TYPE_COUNT; i++)
	{
		if(!stricmp(pStr, param_type_text[i]))
		{
			return (eDef_ParamType_e)i;
		}
	}

	return PARAM_TYPE_INVALID;
}

bool ParseEDEFParameter(kvkeybase_t* pPair, edef_param_t *param)
{
	// resolve type string
	param->typeString = pPair->name;
	eDef_ParamType_e valType = EDef_ResolveParamTypeFromString( param->typeString.GetData() );

	param->type = valType;

	// resolve key name
	param->key = KV_GetValueString(pPair, 0);

	// resolve description key
	param->description = LocalizedString((char*)KV_GetValueString(pPair, 1));

	if(valType == PARAM_TYPE_CHOICE)
	{
		param->choice.choice_num = 0;

		// enum choice values
		for(int i = 0; i < pPair->keys.numElem(); i++)
			param->choice.choice_num++;

		param->choice.choice_list = new edef_choiceval_t[param->choice.choice_num];

		int nChoice = 0;

		// load choice values
		for(int i = 0; i < pPair->keys.numElem(); i++)
		{
			kvkeybase_t* pair = pPair->keys[i];

			edef_choiceval_t& val = param->choice.choice_list[nChoice];

			val.desc = LocalizedString(KV_GetValueString(pair));
			val.value = pair->name;
			nChoice++;
		}
	}

	return true;
}

bool ParseEDEFEntity(kvkeybase_t* pSection)
{
	edef_entity_t* pDef = new edef_entity_t;
	pDef->basedef = NULL;
	pDef->bboxmins = Vector3D(-4);
	pDef->bboxmaxs = Vector3D(4);

	kvkeybase_t *pPair = pSection->FindKeyBase("baseclass");
	if(pPair)
	{
		edef_entity_t* pBaseDef = EDef_Find( KV_GetValueString(pPair) );
		if(!pBaseDef)
		{
			MsgError("\n***Error*** class '%s' not found as baseclass of '%s'\n", KV_GetValueString(pPair), pDef->classname.GetData());
			return false;
		}
		else
		{
			*pDef = *pBaseDef;
			pDef->basedef = pBaseDef;
			pDef->sprite = NULL;
			pDef->parameters.clear();
		}
	}
	else
	{
		if(g_baseentity != NULL)
		{
			*pDef = *g_baseentity;
			pDef->basedef = g_baseentity;
			pDef->sprite = NULL;
			pDef->parameters.clear();
		}
	}

	// threat section name as classname
	pDef->classname = pSection->name;
	pDef->description = LocalizedEDEF("desc", pSection);

	pPair = pSection->FindKeyBase("color");

	if((pPair && pDef->basedef) || !pDef->basedef)
		pDef->drawcolor = UTIL_StringToColor4(KV_GetValueString(pPair));

	pPair = pSection->FindKeyBase("colorfromkey");

	if((pPair && pDef->basedef) || !pDef->basedef)
		pDef->colorfromkey = KV_GetValueBool(pPair);

	pPair = pSection->FindKeyBase("showradius");

	if((pPair && pDef->basedef) || !pDef->basedef)
		pDef->showradius = KV_GetValueBool(pPair);

	pPair = pSection->FindKeyBase("model");

	if(pPair)
	{
		pDef->modelname = KV_GetValueString(pPair);
		g_studioModelCache->PrecacheModel(pDef->modelname.GetData());
	}
	else
	{
		if(!pDef->basedef)
			pDef->modelname = "";
	}

	int mod_index = g_studioModelCache->GetModelIndex(pDef->modelname.GetData());

	if(mod_index == -1)
		pDef->model = NULL;
	else
		pDef->model = g_studioModelCache->GetModel(mod_index);

	pPair = pSection->FindKeyBase("sprite");

	if(pPair)
	{
		pDef->modelname = KV_GetValueString(pPair);

		pDef->sprite = materials->GetMaterial(pDef->modelname.GetData());
		pDef->sprite->Ref_Grab();
	}
	else
	{
		if(!pDef->basedef)
		{
			pDef->modelname = "";
			pDef->sprite = NULL;
		}
	}

	pPair = pSection->FindKeyBase("showinlist");

	if(pPair)
		pDef->showinlist = KV_GetValueBool(pPair);
	else
		pDef->showinlist = true;

	pPair = pSection->FindKeyBase("showarrow");

	if(pPair)
		pDef->showanglearrow = KV_GetValueBool(pPair);
	else
	{
		if(!pDef->basedef)
		pDef->showanglearrow = false;
	}

	pPair = pSection->FindKeyBase("mins");

	if(pPair)
		pDef->bboxmins = UTIL_StringToColor3(pPair->values[0]);

	pPair = pSection->FindKeyBase("maxs");

	if(pPair)
		pDef->bboxmaxs = UTIL_StringToColor3(pPair->values[0]);

	// parse entity parameters if it have.
	kvkeybase_t* pParamsSec = pSection->FindKeyBase("parameters", KV_FLAG_SECTION);
	if(pParamsSec)
	{
		// parse keys
		for(int i = 0; i < pParamsSec->keys.numElem(); i++)
		{
			edef_param_t param;

			if(ParseEDEFParameter(pParamsSec->keys[i], &param))
				pDef->parameters.append(param);
		}
	}

	kvkeybase_t* pInputSection = pSection->FindKeyBase("Inputs", KV_FLAG_SECTION);
	if(pInputSection)
	{
		for(int i = 0; i < pInputSection->keys.numElem(); i++)
		{
			edef_input_t input;

			// copy name
			input.name = KV_GetValueString(pInputSection->keys[i]);

			// resolve value type from string
			input.valuetype = EDef_ResolveParamTypeFromString(pInputSection->keys[i]->name);

			// add to list
			pDef->inputs.append(input);
		}
	}

	kvkeybase_t* pOutputSection = pSection->FindKeyBase("Outputs", KV_FLAG_SECTION);
	if(pOutputSection)
	{
		for(int i = 0; i < pOutputSection->keys.numElem(); i++)
		{
			edef_output_t output;

			// copy name
			output.name = pOutputSection->keys[i]->name;

			// TODO: description

			// add to list
			pDef->outputs.append(output);
		}
	}

	Msg("Entity '%s' parsed\n", pDef->classname.GetData());

	// assign the id of 
	if((g_baseentity == NULL) && !stricmp(pSection->name, "baseentity"))
	{
		g_baseentity = pDef;
	}

	g_defs.append(pDef);

	return true;
}

int edef_sorter( edef_entity_t* const &a, edef_entity_t* const &b )
{
	if(!a->classname.Compare("baseentity"))
		return 0;

	return stricmp(a->classname.GetData(), b->classname.GetData());
}

bool EDef_Load(const char* filename, bool clean)
{
	if(clean)
		EDef_Unload();

	KeyValues kv;

	if(kv.LoadFromFile(filename))
	{
		kvkeybase_t* root = kv.GetRootSection();

		// parse includes
		for(int i = 0; i < root->keys.numElem(); i++)
		{
			kvkeybase_t* key = root->keys[i];

			if(!stricmp(key->name, "#include"))
				EDef_Load(KV_GetValueString(key), false );
			else if(!stricmp(key->name, "localizedtokensfile"))
				g_localizer->AddTokensFile(KV_GetValueString(key));
		}

		// parse entities
		for(int i = 0; i < kv.GetRootSection()->keys.numElem(); i++)
		{
			if(!kv.GetRootSection()->keys[i]->keys.numElem())
				continue;

			if(!ParseEDEFEntity(kv.GetRootSection()->keys[i]))
				return false;
		}
	}
	else
	{
		MsgError("\n***Error*** Can't load entity definitions '%s'\n", filename);
		//return false;
	}

	g_defs.sort(edef_sorter);

	return true;
}

//
// Reloads entity definitions
//
void EDef_Flush()
{
	EDef_Load(g_entdefname.GetData());
}

//
// Unloads entity definitions
//
void EDef_Unload()
{

}

//
// Finds entity definition by name
//
edef_entity_t* EDef_Find(const char* pszClassName)
{
	for(int i = 0; i < g_defs.numElem(); i++)
	{
		if(!stricmp(g_defs[i]->classname.GetData(), pszClassName))
			return g_defs[i];
	}

	return NULL;
}

//
// Recursively finds parameter
//
edef_param_t* FindParameterDef_r(const char* pszName, edef_entity_t* pDef)
{
	if(!pDef)
		return NULL;

	for(int i = 0; i < pDef->parameters.numElem(); i++)
	{
		if(!stricmp(pDef->parameters[i].key.GetData()/*string.data*/, pszName))
			return &pDef->parameters[i];
	}

	return FindParameterDef_r(pszName, pDef->basedef);
}

//
// Recursively finds parameter
//
edef_param_t* EDef_FindParameter(const char* pszName, edef_entity_t* pDef)
{
	if(!pDef)
		return NULL;

	return FindParameterDef_r(pszName, pDef);
}

//
// Recursively fills parameter list
//
void GetFullParamList_r(DkList<edef_param_t*> &list, edef_entity_t* pDef)
{
	if(!pDef)
		return;

	for(int i = 0; i < pDef->parameters.numElem(); i++)
		list.append(&pDef->parameters[i]);

	GetFullParamList_r(list, pDef->basedef);
}

//
// Recursively fills parameter list
//
void EDef_GetFullParamList(DkList<edef_param_t*> &list, edef_entity_t* pDef)
{
	GetFullParamList_r(list, pDef);
}

//
// Recursively fills output list
//
void GetFullOutputList_r(DkList<edef_output_t*> &list, edef_entity_t* pDef)
{
	if(!pDef)
		return;

	for(int i = 0; i < pDef->outputs.numElem(); i++)
		list.append(&pDef->outputs[i]);

	GetFullOutputList_r(list, pDef->basedef);
}

//
// Recursively fills output list
//
void EDef_GetFullOutputList(DkList<edef_output_t*> &list, edef_entity_t* pDef)
{
	GetFullOutputList_r(list, pDef);
}

//
// Recursively fills output list
//
void GetFullInputList_r(DkList<edef_input_t*> &list, edef_entity_t* pDef)
{
	if(!pDef)
		return;

	for(int i = 0; i < pDef->inputs.numElem(); i++)
		list.append(&pDef->inputs[i]);

	GetFullInputList_r(list, pDef->basedef);
}

//
// Recursively fills output list
//
void EDef_GetFullInputList(DkList<edef_input_t*> &list, edef_entity_t* pDef)
{
	GetFullInputList_r(list, pDef);
}