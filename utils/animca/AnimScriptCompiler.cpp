//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////

#include "core/IFileSystem.h"
#include "core/DebugInterface.h"
#include "core/ICmdLineParser.h"
#include "utils/KeyValues.h"

#include "math/DkMath.h"
#include "utils/align.h"
#include "egf/dsm_esm_loader.h"
#include "egf/model.h"

#include <zlib.h>

//************************************
// Checks model ident
//************************************
bool IsValidModelIdentifier(int id)
{
	switch(id)
	{
		case EQUILIBRIUM_MODEL_SIGNATURE:
			return true;
	}
	return false;
}

void ConvertHeaderToLatestVersion(basemodelheader_t* pHdr)
{
	// initial, not used
}

// loads all supported Equilibrium model formats
studiohdr_t* Studio_LoadModel(const char* pszPath)
{
	long len = 0;
	char* _buffer = g_fileSystem->GetFileBuffer(pszPath,&len);

	if(!_buffer)
	{
		MsgError("Can't open '%s'",pszPath);
		return NULL;
	}

	basemodelheader_t* pBaseHdr = (basemodelheader_t*)_buffer;
	if(!IsValidModelIdentifier(pBaseHdr->ident))
	{
		delete [] _buffer;
		MsgError("Invalid model file '%s'\n",pszPath);
		return NULL;
	}

	ConvertHeaderToLatestVersion(pBaseHdr);

	// TODO: Double data protection!!!

	studiohdr_t* pHdr = (studiohdr_t*)pBaseHdr;

	if(pHdr->version != EQUILIBRIUM_MODEL_VERSION)
	{
		MsgError("Wrong model model version, should be %i, excepted %i\n",EQUILIBRIUM_MODEL_VERSION,pBaseHdr->version);
		delete [] _buffer;
		return NULL;
	}

	if(len != pHdr->length)
	{
		MsgError("Model size is not valid (%d versus %d in header)!\n",len, pBaseHdr->size);
		delete [] _buffer;
		return NULL;
	}

	return pHdr;
}

studiohdr_t*				g_model = NULL;
int							g_numbones = 0;

// animations
DkList<studioAnimation_t>	g_animations;

// sequences
DkList<sequencedesc_t>		g_sequences;

// events
DkList<sequenceevent_t>		g_events;

// pose controllers
DkList<posecontroller_t>	g_posecontrollers;

kvkeybase_t*				g_model_usage = NULL;
kvkeybase_t*				g_outputfilename = NULL;

EqString					animPath("./");

void Cleanup()
{
	// delete model
	PPFree((ubyte*)g_model);

	// delete animations
	for(int i = 0; i < g_animations.numElem(); i++)
	{
		for(int j = 0; j < g_numbones; j++)
			delete [] g_animations[i].bones[j].keyFrames;

		delete [] g_animations[i].bones;
	}

	g_animations.clear();
	g_sequences.clear();
	g_events.clear();
	g_posecontrollers.clear();
	g_model_usage = NULL;
	g_outputfilename = NULL;

	animPath = "./";
}

//************************************
// Finds and returns sequence index by name
//************************************
int	GetSequenceIndex(const char* name)
{
	for(int i = 0; i < g_sequences.numElem(); i++)
	{
		if(!stricmp(g_sequences[i].name, name))
			return i;
	}

	return -1;
}

//************************************
// Finds and returns animation index by name
//************************************
int	GetAnimationIndex(const char* name)
{
	for(int i = 0; i < g_animations.numElem(); i++)
	{
		if(!stricmp(g_animations[i].name, name))
			return i;
	}

	return -1;
}

//************************************
// Finds and returns pose controller index by name
//************************************
int	GetPoseControllerIndex(const char* name)
{
	for(int i = 0; i < g_posecontrollers.numElem(); i++)
	{
		if(!stricmp(g_posecontrollers[i].name, name))
			return i;
	}

	return -1;
}

/*

//************************************
// Shifts animation start
//************************************
void ShiftAnimationFrames(studioBoneFrame_t* bone, int new_start_frame)
{
	animframe_t* frames_copy = new animframe_t[bone->numFrames];

	for(int i = 0; i < new_start_frame; i++)
	{
		frames_copy[i] = bone->keyFrames[(bone->numFrames-1) - new_start_frame];
	}

	int c = 0;
	for(int i = new_start_frame-1; i < bone->numFrames; i++; c++)
	{
		frames_copy[i] = bone->keyFrames[c++];
	}

	delete [] bone->keyFrames;

	bone->keyFrames = frames_copy;
}
*/

//*******************************************************
// TODO: use from bonesetup.h
//*******************************************************
void TranslateAnimationFrames(studioBoneFrame_t* bone, Vector3D &offset)
{
	for(int i = 0; i < bone->numFrames; i++)
	{
		bone->keyFrames[i].vecBonePosition += offset;
	}
}

//******************************************************
// Subtracts the animation frames
// IT ONLY SUBTRACTS BY FIRST FRAME OF otherbone
//******************************************************
void SubtractAnimationFrames(studioBoneFrame_t* bone, studioBoneFrame_t* otherbone)
{
	for(int i = 0; i < bone->numFrames; i++)
	{
		bone->keyFrames[i].vecBonePosition -= otherbone->keyFrames[0].vecBonePosition;
		bone->keyFrames[i].angBoneAngles -= otherbone->keyFrames[0].angBoneAngles;
	}
}

//*******************************************************
// Advances every frame position on reversed velocity
// Helps with motion capture issues.
//*******************************************************
void VelocityBackTransform(studioBoneFrame_t* bone, Vector3D &velocity)
{
	for(int i = 0; i < bone->numFrames; i++)
	{
		bone->keyFrames[i].vecBonePosition -= velocity*float(i);
	}
}

#define BONE_NOT_SET 65536

//*******************************************************
// key marked as empty?
//*******************************************************
inline bool IsEmptyKeyframe(animframe_t* keyFrame)
{
	if(keyFrame->vecBonePosition.x == BONE_NOT_SET || keyFrame->angBoneAngles.x == BONE_NOT_SET)
		return true;

	return false;
}

//*******************************************************
// Fills empty frames of animation
// and interpolates non-empty to them.
//*******************************************************
void InterpolateBoneAnimationFrames(studioBoneFrame_t* bone)
{
	float lastKeyframeTime = 0;
	float nextKeyframeTime = 0;

	animframe_t *lastKeyFrame = NULL;
	animframe_t *nextInterpKeyFrame = NULL;

	// TODO: interpolate bone at the missing animation frames
	for(int i = 0; i < bone->numFrames; i++)
	{
		if(!lastKeyFrame)
		{
			lastKeyframeTime = i;
			lastKeyFrame = &bone->keyFrames[i]; // set last key frame
			continue;
		}

		for(int j = i; j < bone->numFrames; j++)
		{
			if(!nextInterpKeyFrame && !IsEmptyKeyframe(&bone->keyFrames[j]))
			{
				//Msg("Interpolation destination key set to %d\n", j);
				nextInterpKeyFrame = &bone->keyFrames[j];
				nextKeyframeTime = j;
				break;
			}
		}

		if(lastKeyFrame && nextInterpKeyFrame)
		{
			if(!IsEmptyKeyframe(&bone->keyFrames[i]))
			{
				//Msg("Interp ends\n");
				lastKeyFrame = NULL;
				nextInterpKeyFrame = NULL;
			}
			else
			{
				if(IsEmptyKeyframe(lastKeyFrame))
					MsgError("Can't interpolate when start frame is NULL\n");

				if(IsEmptyKeyframe(nextInterpKeyFrame))
					MsgError("Can't interpolate when end frame is NULL\n");

				// do basic interpolation
				bone->keyFrames[i].vecBonePosition = lerp(lastKeyFrame->vecBonePosition, nextInterpKeyFrame->vecBonePosition, (float)(i - lastKeyframeTime) / (float)(nextKeyframeTime - lastKeyframeTime));
				bone->keyFrames[i].angBoneAngles = lerp(lastKeyFrame->angBoneAngles, nextInterpKeyFrame->angBoneAngles, (float)(i - lastKeyframeTime) / (float)(nextKeyframeTime - lastKeyframeTime));
			}
		}
		else if(lastKeyFrame && !nextInterpKeyFrame)
		{
			// if there is no next frame, don't do interpolation, simple copy
			bone->keyFrames[i].vecBonePosition = lastKeyFrame->vecBonePosition;
			bone->keyFrames[i].angBoneAngles = lastKeyFrame->angBoneAngles;
		}

		if(!lastKeyFrame)
		{
			lastKeyframeTime = i;
			lastKeyFrame = &bone->keyFrames[i]; // set last key frame
			continue;
		}

		for(int j = i; j < bone->numFrames; j++)
		{
			if(!nextInterpKeyFrame && !IsEmptyKeyframe(&bone->keyFrames[j]))
			{
				//Msg("Interpolation destination key set to %d\n", j);
				nextInterpKeyFrame = &bone->keyFrames[j];
				nextKeyframeTime = j;
				break;
			}
		}
	}
}

//************************************
// TODO: use from bonesetup.h
//************************************
inline void InterpolateFrameTransform(animframe_t &frame1, animframe_t &frame2, float value, animframe_t &out)
{
	out.angBoneAngles = lerp(frame1.angBoneAngles, frame2.angBoneAngles, value);
	out.vecBonePosition = lerp(frame1.vecBonePosition, frame2.vecBonePosition, value);
}

//************************************
// Crops animated bones
//************************************
void CropAnimationBoneFrames(studioBoneFrame_t* pBone, int newStart, int newFrames)
{
	if(newStart >= pBone->numFrames)
	{
		MsgError("Error in crop: min frame value is bigger than number of frames of animation\n");
		return;
	}

	if(newFrames == -1)
		newFrames = pBone->numFrames - newStart;

	if(newFrames > (pBone->numFrames - newStart))
	{
		MsgError("Error in crop: max frame value is bigger than number of frames of animation, it must be ident\n");
		return;
	}

	animframe_t* new_frames = new animframe_t[newFrames];

	for(int i = 0; i < newFrames; i++)
	{
		new_frames[i] = pBone->keyFrames[i + newStart];
	}

	delete [] pBone->keyFrames;
	pBone->keyFrames = new_frames;

	pBone->numFrames = newFrames;
}

//************************************
// Crops animation
//************************************
void CropAnimationDimensions(studioAnimation_t* pAnim, int newStart, int newFrames)
{
	for(int i = 0; i < g_model->numBones; i++)
		CropAnimationBoneFrames(&pAnim->bones[i], newStart, newFrames);
}

//************************************
// Reverse animated bones
//************************************
void ReverseAnimationBoneFrames(studioBoneFrame_t* pBone)
{
	animframe_t* new_frames = new animframe_t[pBone->numFrames];

	for(int i = 0; i < pBone->numFrames; i++)
	{
		int rev_idx = (pBone->numFrames-1)-i;

		new_frames[i] = pBone->keyFrames[rev_idx];
	}

	delete [] pBone->keyFrames;
	pBone->keyFrames = new_frames;
}

//************************************
// Reverse animation
//************************************

void ReverseAnimation(studioAnimation_t* pAnim )
{
	for(int i = 0; i < g_model->numBones; i++)
		ReverseAnimationBoneFrames( &pAnim->bones[i] );
}

//************************************
// computes time from frame time
// TODO: move to bonesetup.h
//************************************
void GetCurrAndNextFrameFromTime(float time, int max, int *curr, int *next)
{
	// compute frame numbers
	*curr = floor(time+1)-1;

	if(next)
		*next = *curr+1;

	// check frame bounds
	if(next && *next >= max)
		*next = max-1;

	if(*curr > max-2)
		*curr = max-2;

	if(*curr < 0)
		*curr = 0;

	if(next && *next < 0)
		*next = 0;
}

//************************************
// Scales bone animation length
//************************************
void RemapBoneFrames(studioBoneFrame_t* pBone, int newLength)
{
	animframe_t*	newFrames = new animframe_t[newLength];
	bool*			bSetFrames = new bool[newLength];

	memset(bSetFrames, 0, sizeof(bool) * newLength);

	for(int i = 0; i < newLength; i++)
	{
		newFrames[i].vecBonePosition.x = BONE_NOT_SET;
		newFrames[i].angBoneAngles.x = BONE_NOT_SET;
	}

	float fFrameFactor = (float)newLength / (float)pBone->numFrames;
	float fFrameFactor_to_old = (float)pBone->numFrames / (float)newLength;


	bSetFrames[0] = true;
	bSetFrames[newLength-1] = true;

	newFrames[0] = pBone->keyFrames[0];
	newFrames[newLength-1] = pBone->keyFrames[pBone->numFrames-1];

	for(int i = 0; i < pBone->numFrames; i++)
	{
		float fframe_id = fFrameFactor*(float)i;

		int new_frame_1 = 0;
		int new_frame_2 = 0;
		GetCurrAndNextFrameFromTime(fframe_id, newLength, &new_frame_1, &new_frame_2);

		int interp_frame1 = 0;
		int interp_frame2 = 0;
		float timefactor_to_old = fFrameFactor_to_old*(float)new_frame_1;
		GetCurrAndNextFrameFromTime(timefactor_to_old, pBone->numFrames, &interp_frame1, &interp_frame2);

		if(!bSetFrames[i])
		{
			float ftime_interp = timefactor_to_old - (int)interp_frame1;

			InterpolateFrameTransform(pBone->keyFrames[interp_frame1], pBone->keyFrames[interp_frame2], ftime_interp, newFrames[new_frame_1]);
		}

		bSetFrames[i] = true;
	}

	// finally, replace bones

	delete [] pBone->keyFrames;
	delete [] bSetFrames;

	pBone->keyFrames = newFrames;
	pBone->numFrames = newLength;

	InterpolateBoneAnimationFrames(pBone);
}

//************************************
// Scales animation length
//************************************
void RemapAnimationLength(studioAnimation_t* pAnim, int newLength)
{
	for(int i = 0; i < g_model->numBones; i++)
		RemapBoneFrames(&pAnim->bones[i], newLength);
}

//************************************
// Setups ESA bones for conversion
//************************************

struct animCaBoneFrames_t
{
	DkList<animframe_t> frames;
	Matrix4x4 bonematrix;
	int nParentBone;
};

void SetupESABones(dsmmodel_t* pModel, animCaBoneFrames_t* bones)
{/*
	// setup each bone's transformation
	for(int8 i = 0; i < pModel->bones.numElem(); i++)
	{
		dsmskelbone_t* bone = pModel->bones[i];

		// setup transformation
		Matrix4x4 localTrans = identity4();

		bones[i].nParentBone = bone->parent_id;

		localTrans.setRotation(bone->angles);
		localTrans.setTranslation(bone->position);

		bones[i].bonematrix	= localTrans;
		bones[i].quat = Quaternion(bone->angles.x, bone->angles.y, bone->angles.z);
	}
	*/
	// NOTE: here we taking reference model bones from studiohdr
	// because I'm not sure that the bones in ESA file are valid

	// setup each bone's transformation
	for(int8 i = 0; i < g_model->numBones; i++)
	{
		bonedesc_t* bone = g_model->pBone(i);

		// setup transformation
		Matrix4x4 localTrans = identity4();

		bones[i].nParentBone = bone->parent;

		localTrans.setRotation(bone->rotation);
		localTrans.setTranslation(bone->position);

		bones[i].bonematrix	= localTrans;
	}
}

//************************************
// Loads DSA frames
//************************************

bool ReadFramesForBone(Tokenizer& tok, DkList<animCaBoneFrames_t>& bones)
{
	char *str;

	bool bCouldRead = false;

	while ((str = tok.next()) != NULL)
	{
		if(str[0] == '{')
		{
			bCouldRead = true;
		}
		else if(str[0] == '}')
		{
			// frame done, go read next
			return true;
		}
		else if(bCouldRead)
		{
			// read time
			int nBone = atoi(str);

			//Msg("bone index: %d\n", nBone);

			animframe_t frame;
			frame.vecBonePosition.x = readFloat(tok);
			frame.vecBonePosition.y = readFloat(tok);
			frame.vecBonePosition.z = readFloat(tok);

			frame.angBoneAngles.x = readFloat(tok);
			frame.angBoneAngles.y = readFloat(tok);
			frame.angBoneAngles.z = readFloat(tok);

			/*
			// convert from absolute local positioning
			Matrix4x4 boneMatrix = identity4();
			boneMatrix.setRotation(frame.angBoneAngles);
			boneMatrix.setTranslation(frame.vecBonePosition);

			boneMatrix = boneMatrix*!bones[nBone].bonematrix;

			frame.vecBonePosition = boneMatrix.rows[3].xyz();
			frame.angBoneAngles = EulerMatrixXYZ(boneMatrix.getRotationComponent());
			*/
			bones[nBone].frames.append( frame );
			// next bone
		}
		else
			tok.goToNextLine();
	}

	return false;
}

bool ReadFrames(Tokenizer& tok, dsmmodel_t* pModel, studioAnimation_t* pAnim)
{
	char *str;

	bool bCouldRead = false;

	int nFrameIndex = 0;

	DkList<animCaBoneFrames_t> bones;
	bones.setNum( pModel->bones.numElem() );

	// FIXME: do it after ReadBones, not here
	SetupESABones(pModel, bones.ptr());

	while ((str = tok.next()) != NULL)
	{
		if(str[0] == '{')
		{
			bCouldRead = true;
		}
		else if(str[0] == '}')
		{
			break;
		}
		else if(bCouldRead)
		{

			// read time
			float fFrameTime = atof(str);

			//Msg("frame %g read\n", fFrameTime);

			if(!ReadFramesForBone(tok, bones))
				return false;

			nFrameIndex++;
		}
		else
			tok.goToNextLine();
	}

	// copy all data
	for(int i = 0; i < bones.numElem(); i++)
	{
		int numFrames = bones[i].frames.numElem();
		pAnim->bones[i].numFrames = numFrames;
		pAnim->bones[i].keyFrames = new animframe_t[numFrames];

		// copy frames
		memcpy(pAnim->bones[i].keyFrames, bones[i].frames.ptr(), numFrames * sizeof(animframe_t));

		// try fix and iterpolate
		//InterpolateBoneAnimationFrames( &pAnim->bones[i] );

		// convert rotations to local

	}

	return true;
}

//************************************
// Loads animation from file
//************************************
int LoadAnimationFromESA(const char* filename)
{
	EqString finalFileName = filename;

	// load from exporter-supported path
	if(g_fileSystem->FileExist( (animPath + "/anims/" + filename + ".esa").GetData() ))
		finalFileName = animPath + "/anims/" + filename + ".esa";
	else
		finalFileName = animPath + "/" + filename + ".esa";

	Tokenizer tok;

	if (!tok.setFile( finalFileName.GetData() ))
	{
		MsgError("Couldn't open ESA file '%s'", finalFileName.GetData());
		return -1;
	}

	char *str;

	dsmmodel_t tempDSM;

	// make new model animation
	studioAnimation_t modelAnim;

	memset(&modelAnim, 0, sizeof(studioAnimation_t));

	strcpy(modelAnim.name,filename); // set it externally from file name

	Matrix4x4 matAbsoluteBones[256];

	// find faces section
	while ((str = tok.next()) != NULL)
	{
		if(!stricmp(str, "bones"))
		{
			if(!ReadBones(tok, &tempDSM))
				return -1;
		}
		else if(!stricmp(str, "frames"))
		{
			if(g_numbones == 0)
				g_numbones = tempDSM.bones.numElem();

			if(g_model->numBones < g_numbones)
			{
				MsgError("Invalid bones! Please re-export model!\n");
				delete [] modelAnim.bones;
				return -1;
			}

			modelAnim.bones = new studioBoneFrame_t[g_numbones];

			if(!ReadFrames(tok, &tempDSM, &modelAnim))
			{
				delete [] modelAnim.bones;
				FreeDSM(&tempDSM);
				return -1;
			}
		}
		else
			tok.goToNextLine();
	}

	FreeDSM(&tempDSM);

	return g_animations.append(modelAnim);
}

//************************************
// Loads animation from key-values parameters
// and applies.
//************************************
void LoadAnimation(kvkeybase_t* section)
{
	kvkeybase_t* pNameKey = section->FindKeyBase("name");

	if(!pNameKey)
	{
		MsgError("Animation script error! Not found parameter 'name' in section 'animation'\n");
		return;
	}

	EqString filename(KV_GetValueString(pNameKey));

	kvkeybase_t* externalpath = section->FindKeyBase("externalfile");

	if(externalpath)
		filename = KV_GetValueString(externalpath);

	Msg(" loading animation '%s' as '%s'\n", filename.GetData(), pNameKey->values[0]);

	Vector3D anim_offset(0);
	Vector3D anim_movevelocity(0);

	kvkeybase_t* offsetPos = section->FindKeyBase("offset");

	anim_offset = KV_GetVector3D(offsetPos, 0, vec3_zero);

	kvkeybase_t* velocity = section->FindKeyBase("movevelocity");
	if(velocity)
	{
		float frame_rate = 1;

		anim_movevelocity = KV_GetVector3D(velocity, 0, vec3_zero);
		frame_rate = KV_GetValueFloat(velocity, 3, 1.0f);

		anim_movevelocity /= frame_rate;
	}

	kvkeybase_t* cust_length_key = section->FindKeyBase("customlength");
	int custom_length = -1;

	custom_length = KV_GetValueInt(cust_length_key, 0, 1);

	bool enable_cutting = false;
	bool reverse = false;
	int cut_start = 0;
	int cut_end = -1;

	kvkeybase_t* cut_anim_key = section->FindKeyBase("crop");
	if(cut_anim_key)
	{
		cut_start = KV_GetValueInt(cut_anim_key, 0, -1);
		cut_end = KV_GetValueInt(cut_anim_key, 0, -1);

		if(cut_start == -1)
		{
			MsgError("Key error: crop must have at least one value (min)\n");
			MsgWarning("Usage: crop (min frame) (max frame)\n");
		}
		else
			enable_cutting = true;
	}

	kvkeybase_t* rev_key = section->FindKeyBase("reverse");

	reverse = KV_GetValueBool(rev_key, 0, false);

	int anim_index = LoadAnimationFromESA( filename.GetData() );

	if(anim_index == -1)
	{
		MsgError("ERROR: Cannot open animation file '%s'!\n", filename.GetData());
		return;
	}

	studioAnimation_t* pAnim = &g_animations[ anim_index ];

	for(int i = 0; i < g_numbones; i++)
	{
		if(g_model->pBone(i)->parent == -1)
		{
			if(length(anim_offset) > 0)
				Msg("Animation offset: %f %f %f\n", anim_offset.x, anim_offset.y, anim_offset.z);

			// move bones to user-defined position
			TranslateAnimationFrames(&pAnim->bones[i], anim_offset);

			// transform model back using linear velocity
			VelocityBackTransform(&pAnim->bones[i], anim_movevelocity);
		}
	}

	kvkeybase_t* subtract_key = section->FindKeyBase("subtract");

	if(subtract_key)
	{
		int subtract_by_anim = GetAnimationIndex( KV_GetValueString(subtract_key) );
		if(subtract_by_anim != -1)
		{
			for(int i = 0; i < g_numbones; i++)
				SubtractAnimationFrames(&pAnim->bones[i], &g_animations[subtract_by_anim].bones[i]);
		}
		else
		{
			MsgError("Subtract: animation %s not found\n", subtract_key->values[0]);
		}
	}

	if(enable_cutting)
	{
		Msg("Cropping from %d to %d\n", cut_start, cut_end);
		CropAnimationDimensions( pAnim, cut_start, cut_end );
	}

	if(custom_length != -1)
	{
		Msg("Changing length to %d\n", custom_length);
		RemapAnimationLength( pAnim, custom_length );
	}

	if(reverse)
	{
		Msg("Reverse\n");
		ReverseAnimation( pAnim );
	}

	// make final name
	strcpy(pAnim->name, KV_GetValueString(pNameKey));
}

//************************************
// Parses animation list from script
//************************************
bool ParseAnimations(kvkeybase_t* section)
{
	for(int i = 0; i < section->keys.numElem(); i++)
	{
		if(!stricmp(section->keys[i]->name, "animation"))
		{
			LoadAnimation( section->keys[i] );
		}
	}

	return g_animations.numElem() > 0;
}

//************************************
// Parses pose parameters from script
//************************************
void ParsePoseparameters(kvkeybase_t* section)
{
	for(int i = 0; i < section->keys.numElem(); i++)
	{
		kvkeybase_t* poseParamKey = section->keys[i];

		if(!stricmp(poseParamKey->name, "poseparameter"))
		{
			if(poseParamKey->values.numElem() < 3)
			{
				MsgError("Invalid poseparameter definition. Valid format is: <poseparam name> <min range> <max range>");
				continue;
			}

			posecontroller_t controller;

			strcpy(controller.name, KV_GetValueString(poseParamKey, 0));
			controller.blendRange[0] = KV_GetValueFloat(poseParamKey, 1);
			controller.blendRange[1] = KV_GetValueFloat(poseParamKey, 2);

			g_posecontrollers.append(controller);
		}
	}
}

//************************************
// Loads sequence parameters
//************************************
void LoadSequence(kvkeybase_t* section, const char* seq_name)
{
	sequencedesc_t desc;
	memset(&desc,0,sizeof(sequencedesc_t));

	bool bAlignAnimationLengths = false;

	strcpy(desc.name, seq_name);

	// UNDONE: duplicate animation and translate if the key "translate" found.

	for(int i = 0; i < section->keys.numElem(); i++)
	{
		if(!stricmp(section->keys[i]->name, "sequencelayer"))
		{
			int seq_index = GetSequenceIndex(KV_GetValueString(section->keys[i]));

			if(seq_index == -1)
			{
				MsgError("No such sequence %s\n", KV_GetValueString(section->keys[i]));
				return;
			}

			desc.sequenceblends[desc.numSequenceBlends] = seq_index;
			desc.numSequenceBlends++;
		}
	}

	// length alignment, for differrent animation lengths, takes the first animation as etalon
	kvkeybase_t* pAlignLengthKey = section->FindKeyBase("alignlengths");

	bAlignAnimationLengths = KV_GetValueBool(pAlignLengthKey);

	int arg_index = g_cmdLine->FindArgument("-forcealign");
	if(arg_index != -1)
	{
		bAlignAnimationLengths = true;
	}

	// parse default parameters
	kvkeybase_t* pFramerateKey = section->FindKeyBase("framerate");

	if(!pFramerateKey)
		desc.framerate = 30;
	else
		desc.framerate = KV_GetValueFloat(pFramerateKey);

	kvkeybase_t* anim_list = section->FindKeyBase("weights");
	if(anim_list)
	{
		desc.numAnimations = anim_list->keys.numElem();

		if(desc.numAnimations == 0)
		{
			MsgError("No values in 'weights' section.\n");
			return;
		}

		for(int i = 0; i < anim_list->keys.numElem(); i++)
		{
			if(!stricmp(anim_list->keys[i]->name, "animation"))
			{
				int anim_index = GetAnimationIndex(KV_GetValueString(anim_list->keys[i]));

				if(anim_index == -1)
				{
					// try to load new one if not found
					anim_index = LoadAnimationFromESA(KV_GetValueString(anim_list->keys[i]));
				}

				if(anim_index == -1)
				{
					MsgError("No such animation %s\n", anim_list->keys[i]->values[0]);
					return;
				}

				desc.animations[i] = anim_index;
			}
		}

		// check for validness
		int checkFrameCount = g_animations[desc.animations[0]].bones[0].numFrames;

		int old_frame_count = checkFrameCount;

		if(bAlignAnimationLengths)
		{
			for(int i = 0; i < desc.numAnimations; i++)
			{
				int real_frames = g_animations[desc.animations[i]].bones[0].numFrames;

				if(real_frames > checkFrameCount)
					checkFrameCount = real_frames;
			}

			desc.framerate *= (float)checkFrameCount / (float)old_frame_count;
		}

		for(int i = 0; i < desc.numAnimations; i++)
		{
			int real_frames = g_animations[desc.animations[i]].bones[0].numFrames;

			if(g_animations[desc.animations[i]].bones[0].numFrames != checkFrameCount)
			{
				if(bAlignAnimationLengths)
				{
					if(checkFrameCount < real_frames)
					{
						MsgWarning("WARNING! Animation quality reduction.\n");
					}

					RemapAnimationLength(&g_animations[desc.animations[i]], checkFrameCount);
				}
				else
					MsgWarning("WARNING! All blending animations must use same frame count! Re-export them or use parameter 'alignlengths'\n");
			}
		}

	}
	else
	{
		desc.numAnimations = 1;

		kvkeybase_t* pKey = section->FindKeyBase("animation");
		if(!pKey)
		{
			MsgError("No 'animation' key.\n");
			return;
		}

		int anim_index = GetAnimationIndex(KV_GetValueString(pKey));

		if(anim_index == -1)
		{
			// try to load new one if not found
			anim_index = LoadAnimationFromESA(KV_GetValueString(pKey));
		}

		if(anim_index == -1)
		{
			MsgError("No such animation %s\n", pKey->values[0]);
			return;
		}

		desc.animations[0] = anim_index;
	}

	if(desc.numAnimations == 0)
	{
		MsgError("No animations for sequence %s\n", desc.name);
		return;
	}

	// parse default parameters
	strcpy(desc.activity, KV_GetValueString(section->FindKeyBase("activity"), 0, "ACT_INVALID"));

	// parse loop flag
	kvkeybase_t* pLoopKey = section->FindKeyBase("loop");
	desc.flags |= KV_GetValueBool(pLoopKey) ? SEQFLAG_LOOP : 0;

	// parse blend between slots flag
	kvkeybase_t* pSlotBlend = section->FindKeyBase("slotblend");
	desc.flags |= KV_GetValueBool(pSlotBlend) ? SEQFLAG_SLOTBLEND : 0;

	// parse notransition flag
	kvkeybase_t* pNoTransitionKey = section->FindKeyBase("notransition");
	desc.flags |= KV_GetValueBool(pNoTransitionKey) ? SEQFLAG_NOTRANSITION : 0;

	// parse autoplay flag
	kvkeybase_t* pAutoplayKey = section->FindKeyBase("autoplay");
	desc.flags |= KV_GetValueBool(pAutoplayKey) ? SEQFLAG_AUTOPLAY : 0;

	// parse transitiontime value
	kvkeybase_t* pTransitionTimekey = section->FindKeyBase("transitiontime");
	if(pTransitionTimekey)
		desc.transitiontime = KV_GetValueFloat(pTransitionTimekey);
	else
		desc.transitiontime = DEFAULT_TRANSITION_TIME;

	// parse events
	kvkeybase_t* event_list = section->FindKeyBase("events");

	if(event_list)
	{
		for(int i = 0; i < event_list->keys.numElem(); i++)
		{
			float ev_frame = (float)atof(event_list->keys[i]->name);

			kvkeybase_t* pEventCommand = event_list->keys[i]->FindKeyBase("command");
			kvkeybase_t* pEventOptions = event_list->keys[i]->FindKeyBase("options");

			if(pEventCommand && pEventOptions)
			{
				// create event
				sequenceevent_t seq_event;
				memset(&seq_event,0,sizeof(sequenceevent_t));

				seq_event.frame = ev_frame;
				strcpy(seq_event.command, KV_GetValueString(pEventCommand));
				strcpy(seq_event.parameter, KV_GetValueString(pEventOptions));

				// add event to list.
				desc.events[desc.numEvents] = g_events.append(seq_event);
				desc.numEvents++;
			}
		}
	}

	kvkeybase_t* pPoseParamKey = section->FindKeyBase("poseparameter");

	if(pPoseParamKey)
	{
		desc.posecontroller = GetPoseControllerIndex(KV_GetValueString(pPoseParamKey));
	}
	else
		desc.posecontroller = -1;

	// add sequence
	g_sequences.append(desc);
}

//************************************
// Parses sequence list
//************************************
void ParseSequences(kvkeybase_t* section)
{
	for(int i = 0; i < section->keys.numElem(); i++)
	{
		kvkeybase_t* seqSec = section->keys[i];

		if(!stricmp(seqSec->GetName(), "sequence"))
		{
			Msg("  Adding sequence %s\n", seqSec->GetName());
			LoadSequence(seqSec, KV_GetValueString(seqSec, 0, "no_name"));
		}
	}
}

DkList<animationdesc_t>		g_animationdescs;
DkList<animframe_t>			g_animframes;

//************************************
// Converts animations to writible format
//************************************
void ConvertAnimationsToWrite()
{
	for(int i = 0; i < g_animations.numElem(); i++)
	{
		animationdesc_t anim;
		memset(&anim, 0, sizeof(animationdesc_t));

		anim.firstFrame = g_animframes.numElem();

		strcpy(anim.name, g_animations[i].name);

		// convert bones.
		for(int j = 0; j < g_numbones; j++)
		{
			anim.numFrames += g_animations[i].bones[j].numFrames;

			for(int k = 0; k < g_animations[i].bones[j].numFrames; k++)
			{
				g_animframes.append(g_animations[i].bones[j].keyFrames[k]);
			}
		}

		g_animationdescs.append(anim);
	}
}

void WriteAnimationPackage();

//************************************
// Makes standard pose.
//************************************
void MakeDefaultPoseAnimation()
{
	// make new model animation
	studioAnimation_t modelAnim;

	memset(&modelAnim, 0, sizeof(studioAnimation_t));

	strcpy(modelAnim.name, "default"); // set it externally from file name

	g_numbones = g_model->numBones;

	modelAnim.bones = new studioBoneFrame_t[g_numbones];
	for(int i = 0; i < g_numbones; i++)
	{

		modelAnim.bones[i].numFrames = 1;
		modelAnim.bones[i].keyFrames = new animframe_t[1];
		modelAnim.bones[i].keyFrames[0].angBoneAngles = vec3_zero;
		modelAnim.bones[i].keyFrames[0].vecBonePosition = vec3_zero;
	}

	g_animations.append(modelAnim);
}

//************************************
// Compiles animation script
//************************************
bool CompileScript(const char* filename)
{
	KeyValues kvs;

	if (!kvs.LoadFromFile(filename))
	{
		MsgError("Cannot open %s!\n", filename);
		return false;
	}

	kvkeybase_t* sec = kvs.GetRootSection();

	g_outputfilename = sec->FindKeyBase("OutputPackage");

	if(!g_outputfilename)
	{
		MsgError("No output specified from script\n");
		return false;
	}

	g_model_usage = sec->FindKeyBase("CheckModel");

	if (!g_model_usage)
	{
		MsgError("No 'CheckModel' specified!\n");
		return false;
	}
			
	g_model = Studio_LoadModel((_Es("models/") + KV_GetValueString(g_model_usage)).GetData());

	if (g_model == NULL)
	{
		MsgError("Failed to load studio model '%s'\n", KV_GetValueString(g_model_usage));
		return false;
	}

	animPath = _Es(filename).Path_Strip_Name();

	// begin script compilation
	MakeDefaultPoseAnimation();

	Msg("Loading animations\n");
	// parse all animations in this script.
	ParseAnimations(sec);

	Msg("Loading pose parameters\n");
	// parse all pose parameters
	ParsePoseparameters(sec);

	Msg("Loading sequences\n");
	// parse sequences
	ParseSequences(sec);

	// write made package
	WriteAnimationPackage();

	// master cleanup
	Cleanup();

	return true;
}


ubyte* CopyLumpToFile(ubyte* data, int lump_type, ubyte* toCopy, int toCopySize)
{
	ubyte* ldata = data;

	animpackagelump_t *lumpdata = (animpackagelump_t*)ldata;
	lumpdata->size = toCopySize;
	lumpdata->type = lump_type;

	ldata = ldata + sizeof(animpackagelump_t);

	memcpy(ldata, toCopy, toCopySize);

	ldata = ldata + toCopySize;

	return ldata;
}

#pragma warning(disable : 4307) // integral constant overflow

#define MAX_MOTIONPACKAGE_SIZE 16*1024*1024

ubyte* pStart = NULL;
ubyte* pData = NULL;

void WriteAnimationPackage()
{
	pStart = (ubyte*)PPAlloc(MAX_MOTIONPACKAGE_SIZE);

	pData = pStart;

	animpackagehdr_t* pHdr = (animpackagehdr_t*)pData;
	pHdr->ident = ANIMCA_IDENT;
	pHdr->version = ANIMCA_VERSION;
	pHdr->numLumps = 0;

	pData += sizeof(animpackagehdr_t);

	// separate g_animations on g_animationdescs and g_animframes
	ConvertAnimationsToWrite();

	pData = CopyLumpToFile(pData, ANIMCA_ANIMATIONS, (ubyte*)g_animationdescs.ptr(), g_animationdescs.numElem() * sizeof(animationdesc_t));
	pHdr->numLumps++;

	// try to compress frame data
	int nFramesSize = g_animframes.numElem() * sizeof(animframe_t);

	unsigned long	nCompressedFramesSize = nFramesSize + 150;
	ubyte*			pCompressedFrames = (ubyte*)malloc(nCompressedFramesSize);

	memset(pCompressedFrames,0,nCompressedFramesSize);

	int comp_stats = Z_ERRNO;

	// do not compress animation frames if option found
	if(g_cmdLine->FindArgument("-nocompress") == -1)
		comp_stats = compress2(pCompressedFrames, &nCompressedFramesSize, (ubyte*)g_animframes.ptr(), nFramesSize, 9);

	if(comp_stats == Z_OK)
	{
		MsgWarning("Successfully compressed frame data from %d to %d bytes\n", nFramesSize, nCompressedFramesSize);

		// write decompression data size
		pData = CopyLumpToFile(pData, ANIMCA_UNCOMPRESSEDFRAMESIZE, (ubyte*)&nFramesSize, sizeof(int));
		pHdr->numLumps++;

		// write compressed frame data (decompress when loading MOP file)
		pData = CopyLumpToFile(pData, ANIMCA_COMPRESSEDFRAMES, pCompressedFrames, nCompressedFramesSize);
		pHdr->numLumps++;

		free(pCompressedFrames);
	}
	else
	{
		// write uncompressed frame data
		pData = CopyLumpToFile(pData, ANIMCA_ANIMATIONFRAMES, (ubyte*)g_animframes.ptr(), g_animframes.numElem() * sizeof(animframe_t));
	}

	pData = CopyLumpToFile(pData, ANIMCA_SEQUENCES, (ubyte*)g_sequences.ptr(), g_sequences.numElem() * sizeof(sequencedesc_t));
	pData = CopyLumpToFile(pData, ANIMCA_EVENTS, (ubyte*)g_events.ptr(), g_events.numElem() * sizeof(sequenceevent_t));
	pData = CopyLumpToFile(pData, ANIMCA_POSECONTROLLERS, (ubyte*)g_posecontrollers.ptr(), g_posecontrollers.numElem() * sizeof(posecontroller_t));

	pHdr->numLumps += 3;


	IFile* file = g_fileSystem->Open(("/models/" + _Es(KV_GetValueString(g_outputfilename))).GetData(), "wb", SP_MOD);
	if(!file)
	{
		MsgError("Can't create file for writing!\n");
		return;
	}

	int filesize = pData - pStart;
	file->Write(pStart, 1, filesize);

	Msg("Total written bytes: %d\n", filesize);
	g_fileSystem->Close(file);

	PPFree(pStart);
}
