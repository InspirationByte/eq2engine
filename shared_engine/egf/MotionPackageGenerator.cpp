//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////

#include <zlib.h>

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "core/ICommandLine.h"
#include "utils/KeyValues.h"
#include "utils/Tokenizer.h"

#include "MotionPackageGenerator.h"

#include "egf/dsm_loader.h"
#include "egf/dsm_esm_loader.h"
#include "egf/dsm_fbx_loader.h"
#include "egf/model.h"
#include "studiofile/StudioLoader.h"

namespace SharedModel {
	extern float readFloat(Tokenizer& tok);
}

using namespace SharedModel;

static void FreeAnimationData(studioAnimation_t* anim, int numBones)
{
	if (anim->bones)
	{
		for (int i = 0; i < numBones; i++)
			PPFree(anim->bones[i].keyFrames);
	}

	PPFree(anim->bones);
}

#define BONE_NOT_SET 65536

struct animCaBoneFrames_t
{
	Array<animframe_t> frames{ PP_SL };
	Matrix4x4 bonematrix;
	int nParentBone;
};


CMotionPackageGenerator::~CMotionPackageGenerator()
{
	Cleanup();
}


void CMotionPackageGenerator::Cleanup()
{
	// delete animations
	for(int i = 0; i < m_animations.numElem(); i++)
	{
		FreeAnimationData(&m_animations[i], m_model->numBones);
	}

	m_animations.clear();
	m_sequences.clear();
	m_events.clear();
	m_posecontrollers.clear();

	m_animPath = "./";
	Studio_FreeModel(m_model);
	m_model = nullptr;
}

//************************************
// Finds and returns sequence index by name
//************************************
int	CMotionPackageGenerator::GetSequenceIndex(const char* name)
{
	for(int i = 0; i < m_sequences.numElem(); i++)
	{
		if(!stricmp(m_sequences[i].name, name))
			return i;
	}

	return -1;
}

//************************************
// Finds and returns animation index by name
//************************************
int	CMotionPackageGenerator::GetAnimationIndex(const char* name)
{
	for(int i = 0; i < m_animations.numElem(); i++)
	{
		if(!stricmp(m_animations[i].name, name))
			return i;
	}

	return -1;
}

//************************************
// Finds and returns pose controller index by name
//************************************
int	CMotionPackageGenerator::GetPoseControllerIndex(const char* name)
{
	for(int i = 0; i < m_posecontrollers.numElem(); i++)
	{
		if(!stricmp(m_posecontrollers[i].name, name))
			return i;
	}

	return -1;
}

/*
//************************************
// Shifts animation start
//************************************
void CMotionPackageGenerator::ShiftAnimationFrames(studioBoneFrame_t* bone, int new_start_frame)
{
	animframe_t* frames_copy = PPAllocStructArray(animframe_t, bone->numFrames);

	for(int i = 0; i < new_start_frame; i++)
	{
		frames_copy[i] = bone->keyFrames[(bone->numFrames-1) - new_start_frame];
	}

	int c = 0;
	for(int i = new_start_frame-1; i < bone->numFrames; i++; c++)
	{
		frames_copy[i] = bone->keyFrames[c++];
	}

	PPFree(bone->keyFrames);

	bone->keyFrames = frames_copy;
}
*/

//*******************************************************
// TODO: use from bonesetup.h
//*******************************************************
void CMotionPackageGenerator::TranslateAnimationFrames(studioBoneAnimation_t* bone, Vector3D &offset)
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
void CMotionPackageGenerator::SubtractAnimationFrames(studioBoneAnimation_t* bone, studioBoneAnimation_t* otherbone)
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
void CMotionPackageGenerator::VelocityBackTransform(studioBoneAnimation_t* bone, Vector3D &velocity)
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
void CMotionPackageGenerator::InterpolateBoneAnimationFrames(studioBoneAnimation_t* bone)
{
	float lastKeyframeTime = 0;
	float nextKeyframeTime = 0;

	animframe_t *lastKeyFrame = nullptr;
	animframe_t *nextInterpKeyFrame = nullptr;

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
				lastKeyFrame = nullptr;
				nextInterpKeyFrame = nullptr;
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
void CMotionPackageGenerator::CropAnimationBoneFrames(studioBoneAnimation_t* pBone, int newStart, int newEnd)
{
	const int newLength = newEnd - newStart + 1;

	if(newStart >= pBone->numFrames)
	{
		MsgError("Crop error: newStart (%d) >= numFrames (%d)\n", newStart, pBone->numFrames);
		return;
	}

	// crop start
	if (newEnd == -1)
		newEnd = pBone->numFrames - 1;

	if (newEnd >= pBone->numFrames)
	{
		MsgError("Crop error: newEnd (%d) >= numFrames (%d)\n", newStart, pBone->numFrames);
		return;
	}

	animframe_t* new_frames = PPAllocStructArray(animframe_t, newLength);

	for(int i = 0; i < newLength; i++)
	{
		new_frames[i] = pBone->keyFrames[i + newStart];
	}

	PPFree(pBone->keyFrames);
	pBone->keyFrames = new_frames;
	pBone->numFrames = newLength;
}

//************************************
// Crops animation
//************************************
void CMotionPackageGenerator::CropAnimationDimensions(studioAnimation_t* pAnim, int newStart, int newEnd)
{
	for(int i = 0; i < m_model->numBones; i++)
		CropAnimationBoneFrames(&pAnim->bones[i], newStart, newEnd);
}

//************************************
// Reverse animated bones
//************************************
void CMotionPackageGenerator::ReverseAnimationBoneFrames(studioBoneAnimation_t* pBone)
{
	animframe_t* new_frames = PPAllocStructArray(animframe_t, pBone->numFrames);

	for(int i = 0; i < pBone->numFrames; i++)
	{
		int rev_idx = (pBone->numFrames-1)-i;

		new_frames[i] = pBone->keyFrames[rev_idx];
	}

	PPFree(pBone->keyFrames);
	pBone->keyFrames = new_frames;
}

//************************************
// Reverse animation
//************************************

void CMotionPackageGenerator::ReverseAnimation(studioAnimation_t* pAnim )
{
	for(int i = 0; i < m_model->numBones; i++)
		ReverseAnimationBoneFrames( &pAnim->bones[i] );
}

//************************************
// computes time from frame time
// TODO: move to bonesetup.h
//************************************
inline void GetCurrAndNextFrameFromTime(float time, int max, int *curr, int *next)
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
void CMotionPackageGenerator::RemapBoneFrames(studioBoneAnimation_t* pBone, int newLength)
{
	animframe_t* newFrames = PPAllocStructArray(animframe_t, newLength);
	bool* bSetFrames = PPAllocStructArray(bool, newLength);

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

	PPFree(pBone->keyFrames);
	PPFree(bSetFrames);

	pBone->keyFrames = newFrames;
	pBone->numFrames = newLength;

	InterpolateBoneAnimationFrames(pBone);
}

//************************************
// Scales animation length
//************************************
void CMotionPackageGenerator::RemapAnimationLength(studioAnimation_t* pAnim, int newLength)
{
	for(int i = 0; i < m_model->numBones; i++)
		RemapBoneFrames(&pAnim->bones[i], newLength);
}

//************************************
// Setups ESA bones for conversion
//************************************

void CMotionPackageGenerator::SetupESABones(DSModel* pModel, animCaBoneFrames_t* bones)
{/*
	// setup each bone's transformation
	for(int8 i = 0; i < pModel->bones.numElem(); i++)
	{
		DSBone* bone = pModel->bones[i];

		// setup transformation
		Matrix4x4 localTrans = identity4;

		bones[i].nParentBone = bone->parentIdx;

		localTrans.setRotation(bone->angles);
		localTrans.setTranslation(bone->position);

		bones[i].bonematrix	= localTrans;
		bones[i].quat = Quaternion(bone->angles.x, bone->angles.y, bone->angles.z);
	}
	*/
	// NOTE: here we taking reference model bones from studiohdr
	// because I'm not sure that the bones in ESA file are valid

	// setup each bone's transformation
	for(int8 i = 0; i < m_model->numBones; i++)
	{
		studioBoneDesc_t* bone = m_model->pBone(i);

		// setup transformation
		Matrix4x4 localTrans = identity4;

		bones[i].nParentBone = bone->parent;

		localTrans.setRotation(bone->rotation);
		localTrans.setTranslation(bone->position);

		bones[i].bonematrix	= localTrans;
	}
}

//************************************
// Loads DSA frames
//************************************
static bool ReadFramesForBone(Tokenizer& tok, Array<animCaBoneFrames_t>& bones)
{
	char *str;

	bool bCouldRead = false;

	while ((str = tok.next()) != nullptr)
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

			bones[nBone].frames.append( frame );
			// next bone
		}
		else
			tok.goToNextLine();
	}

	return false;
}

bool ReadFrames(CMotionPackageGenerator& generator, Tokenizer& tok, DSModel* pModel, studioAnimation_t* pAnim)
{
	char *str;

	bool bCouldRead = false;

	int nFrameIndex = 0;

	Array<animCaBoneFrames_t> bones(PP_SL);
	bones.setNum( pModel->bones.numElem() );

	// FIXME: do it after ReadBones, not here
	generator.SetupESABones(pModel, bones.ptr());

	while ((str = tok.next()) != nullptr)
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
		pAnim->bones[i].keyFrames = PPAllocStructArray(animframe_t, numFrames);

		// copy frames
		memcpy(pAnim->bones[i].keyFrames, bones[i].frames.ptr(), numFrames * sizeof(animframe_t));

		// try fix and iterpolate
		//InterpolateBoneAnimationFrames( &pAnim->bones[i] );

		// convert rotations to local

	}

	return true;
}


void CMotionPackageGenerator::LoadFBXAnimations(const KVSection* section)
{
	const char* fbxFileName = KV_GetValueString(section);

	EqString finalFileName = fbxFileName;

	// load from exporter-supported path
	if (g_fileSystem->FileExist((m_animPath + "/anims/" + fbxFileName).GetData()))
		finalFileName = m_animPath + "/anims/" + fbxFileName;
	else
		finalFileName = m_animPath + "/" + fbxFileName;

	SharedModel::LoadFBXAnimations(m_animations, finalFileName, KV_GetValueString(section->FindSection("meshFilter"), 0, nullptr));
}

//************************************
// Loads animation from file
//************************************
int CMotionPackageGenerator::LoadAnimationFromESA(const char* filename)
{
	EqString finalFileName = filename;

	// load from exporter-supported path
	if(g_fileSystem->FileExist( (m_animPath + "/anims/" + filename + ".esa").GetData() ))
		finalFileName = m_animPath + "/anims/" + filename + ".esa";
	else
		finalFileName = m_animPath + "/" + filename + ".esa";

	Tokenizer tok;
	if (!tok.setFile( finalFileName.GetData() ))
	{
		MsgError("Couldn't open ESA file '%s'\n", finalFileName.GetData());
		return -1;
	}

	// make new model animation
	const int newAnimIndex = m_animations.numElem();
	studioAnimation_t& modelAnim = m_animations.append();
	strcpy(modelAnim.name, filename); // set it externally from file name

	DSModel tempDSM;

	char* str;
	while ((str = tok.next()) != nullptr)
	{
		if(!stricmp(str, "bones"))
		{
			if(!ReadBones(tok, &tempDSM))
				return -1;
		}
		else if(!stricmp(str, "frames"))
		{
			if(m_model->numBones != tempDSM.bones.numElem())
			{
				MsgError("Invalid bones! Please re-export model!\n");
				FreeAnimationData(&modelAnim, m_model->numBones);
				m_animations.removeIndex(newAnimIndex);
				return -1;
			}

			modelAnim.bones = PPAllocStructArray(studioBoneAnimation_t, m_model->numBones);

			if(!ReadFrames(*this, tok, &tempDSM, &modelAnim))
			{
				FreeAnimationData(&modelAnim, m_model->numBones);
				m_animations.removeIndex(newAnimIndex);
				return -1;
			}
		}
		else
			tok.goToNextLine();
	}
	return newAnimIndex;
}

// duplicates the animation for further processing. Returns new index
int CMotionPackageGenerator::DuplicateAnimationByIndex(int animIndex)
{
	ASSERT(animIndex != -1);
	studioAnimation_t& sourceAnim = m_animations[animIndex];

	const int newAnimIndex = m_animations.numElem();
	studioAnimation_t& modelAnim = m_animations.append();

	// some data should work as simple as that
	// but we have to clone keyframe data
	modelAnim = sourceAnim;

	modelAnim.bones = PPAllocStructArray(studioBoneAnimation_t, m_model->numBones);
	for (int i = 0; i < m_model->numBones; ++i)
	{
		const int numFrames = sourceAnim.bones[i].numFrames;
		modelAnim.bones[i].numFrames = numFrames;
		modelAnim.bones[i].keyFrames = PPAllocStructArray(animframe_t, numFrames);

		// copy frames
		memcpy(modelAnim.bones[i].keyFrames, sourceAnim.bones[i].keyFrames, numFrames * sizeof(animframe_t));
	}

	return newAnimIndex;
}

//************************************
// Loads animation from key-values parameters and applies.
//************************************
void CMotionPackageGenerator::LoadAnimation(const KVSection* section)
{
	KVSection* pPathKey = section->FindSection("path");

	if(!pPathKey)
	{
		MsgError("Animation script error! Parameter 'path' in section 'animation' is missing\n");
		return;
	}

	const char* animName = KV_GetValueString(pPathKey);

	KVSection* externalpath = section->FindSection("externalfile");

	if(externalpath)
		animName = KV_GetValueString(externalpath);

	Msg(" loading animation '%s' as '%s'\n", animName, KV_GetValueString(section));

	Vector3D anim_offset(0);
	Vector3D anim_movevelocity(0);

	KVSection* offsetPos = section->FindSection("offset");

	anim_offset = KV_GetVector3D(offsetPos, 0, vec3_zero);

	KVSection* velocity = section->FindSection("movevelocity");
	if(velocity)
	{
		float frame_rate = 1;

		anim_movevelocity = KV_GetVector3D(velocity, 0, vec3_zero);
		frame_rate = KV_GetValueFloat(velocity, 3, 1.0f);

		anim_movevelocity /= frame_rate;
	}

	KVSection* cust_length_key = section->FindSection("customlength");
	int custom_length = KV_GetValueInt(cust_length_key, 0, -1);

	bool enable_cutting = false;
	bool reverse = false;
	int cut_start = 0;
	int cut_end = -1;

	KVSection* cut_anim_key = section->FindSection("crop");
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

	KVSection* rev_key = section->FindSection("reverse");

	reverse = KV_GetValueBool(rev_key, 0, false);

	int anim_index = GetAnimationIndex(animName);

	if(anim_index != -1)
		anim_index = DuplicateAnimationByIndex(anim_index);

	if (anim_index == -1) // try to load new one if not found
		anim_index = LoadAnimationFromESA(animName);

	if(anim_index == -1)
	{
		MsgError("ERROR: Cannot open animation file '%s'!\n", animName);
		return;
	}

	studioAnimation_t* pAnim = &m_animations[ anim_index ];

	for(int i = 0; i < m_model->numBones; i++)
	{
		if (m_model->pBone(i)->parent != -1)
			continue;

		if(length(anim_offset) > 0)
			Msg("Animation offset: %f %f %f\n", anim_offset.x, anim_offset.y, anim_offset.z);

		// move bones to user-defined position
		TranslateAnimationFrames(&pAnim->bones[i], anim_offset);

		// transform model back using linear velocity
		VelocityBackTransform(&pAnim->bones[i], anim_movevelocity);
	}

	KVSection* subtract_key = section->FindSection("subtract");

	if(subtract_key)
	{
		int subtract_by_anim = GetAnimationIndex( KV_GetValueString(subtract_key) );
		if(subtract_by_anim != -1)
		{
			for(int i = 0; i < m_model->numBones; i++)
				SubtractAnimationFrames(&pAnim->bones[i], &m_animations[subtract_by_anim].bones[i]);
		}
		else
		{
			MsgError("Subtract: animation %s not found\n", subtract_key->values[0]);
		}
	}

	if(enable_cutting)
	{
		Msg("  Cropping from [0 %d] to [%d %d]\n", pAnim->bones[0].numFrames, cut_start, cut_end);
		CropAnimationDimensions( pAnim, cut_start, cut_end );

		if(cut_start == cut_end)
			RemapAnimationLength(pAnim, 2);
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
	strcpy(pAnim->name, KV_GetValueString(section));
}

//************************************
// Parses animation list from script
//************************************
bool CMotionPackageGenerator::ParseAnimations(const KVSection* section)
{
	Msg("Processing animations\n");
	for(int i = 0; i < section->keys.numElem(); i++)
	{
		if(!stricmp(section->keys[i]->name, "animation"))
		{
			LoadAnimation( section->keys[i] );
		}
	}

	return m_animations.numElem() > 0;
}

//************************************
// Parses pose parameters from script
//************************************
void CMotionPackageGenerator::ParsePoseparameters(const KVSection* section)
{
	Msg("Processing pose parameters\n");
	for(int i = 0; i < section->keys.numElem(); i++)
	{
		KVSection* poseParamKey = section->keys[i];

		if(!stricmp(poseParamKey->name, "poseparameter"))
		{
			if(poseParamKey->values.numElem() < 3)
			{
				MsgError("Incorrect usage. Example: poseparameter <poseparam name> <min range> <max range>");
				continue;
			}

			posecontroller_t controller;

			strcpy(controller.name, KV_GetValueString(poseParamKey, 0));
			controller.blendRange[0] = KV_GetValueFloat(poseParamKey, 1);
			controller.blendRange[1] = KV_GetValueFloat(poseParamKey, 2);

			m_posecontrollers.append(controller);
		}
	}
}

//************************************
// Loads sequence parameters
//************************************
void CMotionPackageGenerator::LoadSequence(const KVSection* section, const char* seq_name)
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
				MsgError("No such sequence '%s' for making layer in sequence '%s'\n", KV_GetValueString(section->keys[i]), seq_name);
				return;
			}

			desc.sequenceblends[desc.numSequenceBlends] = seq_index;
			desc.numSequenceBlends++;
		}
	}

	// length alignment, for differrent animation lengths, takes the first animation as etalon
	KVSection* pAlignLengthKey = section->FindSection("alignlengths");

	bAlignAnimationLengths = KV_GetValueBool(pAlignLengthKey);

	int arg_index = g_cmdLine->FindArgument("-forcealign");
	if(arg_index != -1)
	{
		bAlignAnimationLengths = true;
	}

	// parse default parameters
	KVSection* pFramerateKey = section->FindSection("framerate");

	if(!pFramerateKey)
		desc.framerate = 30;
	else
		desc.framerate = KV_GetValueFloat(pFramerateKey);

	const KVSection* animListKvs = section->FindSection("weights");
	if(animListKvs)
	{
		desc.numAnimations = animListKvs->keys.numElem();

		if(desc.numAnimations == 0)
		{
			MsgError("No values in 'weights' section.\n");
			return;
		}

		for(int i = 0; i < animListKvs->keys.numElem(); i++)
		{
			if (stricmp(animListKvs->keys[i]->name, "animation"))
				continue;

			const char* animName = KV_GetValueString(animListKvs->keys[i]);
			int anim_index = GetAnimationIndex(animName);

			if(anim_index == -1) // try to load new one if not found
				anim_index = LoadAnimationFromESA(animName);

			if(anim_index == -1)
			{
				MsgError("No such animation '%s'\n", animName);
				return;
			}

			desc.animations[i] = anim_index;
		}

		// check for validness
		int checkFrameCount = m_animations[desc.animations[0]].bones[0].numFrames;
		const int oldFrameCount = checkFrameCount;

		if(bAlignAnimationLengths)
		{
			for(int i = 0; i < desc.numAnimations; i++)
			{
				int real_frames = m_animations[desc.animations[i]].bones[0].numFrames;

				if(real_frames > checkFrameCount)
					checkFrameCount = real_frames;
			}

			desc.framerate *= (float)checkFrameCount / (float)oldFrameCount;
		}

		for(int i = 0; i < desc.numAnimations; i++)
		{
			const int real_frames = m_animations[desc.animations[i]].bones[0].numFrames;

			if(m_animations[desc.animations[i]].bones[0].numFrames != checkFrameCount)
			{
				if(bAlignAnimationLengths)
				{
					if(checkFrameCount < real_frames)
					{
						MsgWarning("WARNING! Animation quality reduction.\n");
					}

					RemapAnimationLength(&m_animations[desc.animations[i]], checkFrameCount);
				}
				else
					MsgWarning("WARNING! All blending animations must use same frame count! Re-export them or use parameter 'alignlengths'\n");
			}
		}

	}
	else
	{
		desc.numAnimations = 1;

		KVSection* pKey = section->FindSection("animation");
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
		MsgError("No animations for sequence '%s'\n", desc.name);
		return;
	}

	// parse default parameters
	strcpy(desc.activity, KV_GetValueString(section->FindSection("activity"), 0, "ACT_INVALID"));

	Msg("  Adding sequence '%s' with activity '%s'\n", desc.name, desc.activity);

	// parse loop flag
	KVSection* pLoopKey = section->FindSection("loop");
	desc.flags |= KV_GetValueBool(pLoopKey) ? SEQFLAG_LOOP : 0;

	// parse blend between slots flag
	KVSection* pSlotBlend = section->FindSection("slotblend");
	desc.flags |= KV_GetValueBool(pSlotBlend) ? SEQFLAG_SLOTBLEND : 0;

	// parse notransition flag
	KVSection* pNoTransitionKey = section->FindSection("notransition");
	desc.flags |= KV_GetValueBool(pNoTransitionKey) ? SEQFLAG_NO_TRANSITION : 0;

	// parse autoplay flag
	KVSection* pAutoplayKey = section->FindSection("autoplay");
	desc.flags |= KV_GetValueBool(pAutoplayKey) ? SEQFLAG_AUTOPLAY : 0;

	// parse transitiontime value
	KVSection* pTransitionTimekey = section->FindSection("transitiontime");
	if(pTransitionTimekey)
		desc.transitiontime = KV_GetValueFloat(pTransitionTimekey);
	else
		desc.transitiontime = SEQ_DEFAULT_TRANSITION_TIME;

	// parse events
	KVSection* event_list = section->FindSection("events");

	if(event_list)
	{
		for(int i = 0; i < event_list->keys.numElem(); i++)
		{
			float ev_frame = (float)atof(event_list->keys[i]->name);

			KVSection* pEventCommand = event_list->keys[i]->FindSection("command");
			KVSection* pEventOptions = event_list->keys[i]->FindSection("options");

			if(pEventCommand && pEventOptions)
			{
				// create event
				sequenceevent_t seq_event;
				memset(&seq_event,0,sizeof(sequenceevent_t));

				seq_event.frame = ev_frame;
				strcpy(seq_event.command, KV_GetValueString(pEventCommand));
				strcpy(seq_event.parameter, KV_GetValueString(pEventOptions));

				// add event to list.
				desc.events[desc.numEvents] = m_events.append(seq_event);
				desc.numEvents++;
			}
		}
	}

	const char* poseParameterName = KV_GetValueString(section->FindSection("poseparameter"), 0, nullptr);

	if (poseParameterName)
	{
		desc.posecontroller = GetPoseControllerIndex(poseParameterName);
		if (desc.posecontroller == -1)
			MsgWarning("Pose parameter '%s' for sequence '%s' not found\n", seq_name);
	}
	else
		desc.posecontroller = -1;

	desc.slot = KV_GetValueInt(section->FindSection("slot"), 0, 0);

	// add sequence
	m_sequences.append(desc);
}

//************************************
// Parses sequence list
//************************************
void CMotionPackageGenerator::ParseSequences(const KVSection* section)
{
	Msg("Processing sequences\n");
	for(int i = 0; i < section->keys.numElem(); i++)
	{
		KVSection* seqSec = section->keys[i];

		if(!stricmp(seqSec->GetName(), "sequence"))
		{
			LoadSequence(seqSec, KV_GetValueString(seqSec, 0, "no_name"));
		}
	}
}

//************************************
// Converts animations to writible format
//************************************
void CMotionPackageGenerator::ConvertAnimationsToWrite()
{
	for(int i = 0; i < m_animations.numElem(); i++)
	{
		animationdesc_t anim;
		memset(&anim, 0, sizeof(animationdesc_t));

		anim.firstFrame = m_animframes.numElem();

		strcpy(anim.name, m_animations[i].name);

		// convert bones.
		for(int j = 0; j < m_model->numBones; j++)
		{
			const studioBoneAnimation_t& boneFrame = m_animations[i].bones[j];
			for(int k = 0; k < boneFrame.numFrames; ++k)
				m_animframes.append(boneFrame.keyFrames[k]);

			anim.numFrames += boneFrame.numFrames;
		}

		m_animationdescs.append(anim);
	}
}

void WriteAnimationPackage(const char* packageFileName);

//************************************
// Makes standard pose.
//************************************
void CMotionPackageGenerator::MakeDefaultPoseAnimation()
{
	// make new model animation
	studioAnimation_t modelAnim;

	memset(&modelAnim, 0, sizeof(studioAnimation_t));

	strcpy(modelAnim.name, "default"); // set it externally from file name

	modelAnim.bones = PPAllocStructArray(studioBoneAnimation_t, m_model->numBones);
	for(int i = 0; i < m_model->numBones; i++)
	{
		studioBoneAnimation_t& boneFrame = modelAnim.bones[i];
		boneFrame.numFrames = 1;
		boneFrame.keyFrames = PPAllocStructArray(animframe_t, 1);
		boneFrame.keyFrames[0].angBoneAngles = vec3_zero;
		boneFrame.keyFrames[0].vecBonePosition = vec3_zero;
	}

	m_animations.append(modelAnim);
}

//************************************
// Compiles animation script
//************************************
bool CMotionPackageGenerator::CompileScript(const char* filename)
{
	KeyValues kvs;

	if (!kvs.LoadFromFile(filename))
	{
		MsgError("Cannot open %s!\n", filename);
		return false;
	}

	KVSection* sec = kvs.GetRootSection();

	KVSection* outPackageKey = sec->FindSection("Package");
	if(!outPackageKey)
	{
		MsgError("No 'Package' key specified - must be output package file name\n");
		return false;
	}

	KVSection* egfModelKey = sec->FindSection("Model");
	if (!egfModelKey)
	{
		MsgError("No 'Model' key specified - must be EGF file name\n");
		return false;
	}

	EqString egfFilename = KV_GetValueString(egfModelKey);
	EqString mopFilename = KV_GetValueString(outPackageKey);
			
	m_model = Studio_LoadModel(egfFilename);
	if (m_model == nullptr)
	{
		MsgError("Failed to load studio model '%s'\n", egfFilename.ToCString());
		return false;
	}

	m_animPath = _Es(filename).Path_Strip_Name();

	KVSection* animSourceKey = sec->FindSection("FBXSource");
	if (animSourceKey)
	{
		LoadFBXAnimations(animSourceKey);
	}
	
	// begin script compilation
	MakeDefaultPoseAnimation();

	// parse all animations in this script.
	ParseAnimations(sec);

	// parse all pose parameters
	ParsePoseparameters(sec);

	// parse sequences
	ParseSequences(sec);

	// write made package
	WriteAnimationPackage(mopFilename);

	// master cleanup
	Cleanup();

	return true;
}


void CopyLumpToFile(IVirtualStream* data, int lump_type, ubyte* toCopy, int toCopySize)
{
	lumpfilelump_t lump;
	lump.size = toCopySize;
	lump.type = lump_type;
	data->Write(&lump, 1, sizeof(lump));
	data->Write(toCopy, toCopySize, 1);
}

#define MAX_MOTIONPACKAGE_SIZE 16*1024*1024

void CMotionPackageGenerator::WriteAnimationPackage(const char* packageOutputFilename)
{
	CMemoryStream lumpDataStream(nullptr, VS_OPEN_WRITE, MAX_MOTIONPACKAGE_SIZE, PP_SL);

	lumpfilehdr_t header;
	header.ident = ANIMFILE_IDENT;
	header.version = ANIMFILE_VERSION;
	header.numLumps = 0;

	// separate m_animations on m_animationdescs and m_animframes
	ConvertAnimationsToWrite();

	CopyLumpToFile(&lumpDataStream, ANIMFILE_ANIMATIONS, (ubyte*)m_animationdescs.ptr(), m_animationdescs.numElem() * sizeof(animationdesc_t));
	header.numLumps++;

	// try to compress frame data
	const int nFramesSize = m_animframes.numElem() * sizeof(animframe_t);

	unsigned long nCompressedFramesSize = nFramesSize + 150;
	ubyte* pCompressedFrames = (ubyte*)PPAlloc(nCompressedFramesSize);

	memset(pCompressedFrames, 0, nCompressedFramesSize);

	int comp_stats = Z_ERRNO;

	// do not compress animation frames if option found
	if(g_cmdLine->FindArgument("-nocompress") == -1)
		comp_stats = compress2(pCompressedFrames, &nCompressedFramesSize, (ubyte*)m_animframes.ptr(), nFramesSize, 9);

	if(comp_stats == Z_OK)
	{
		MsgWarning("Successfully compressed frame data from %d to %d bytes\n", nFramesSize, nCompressedFramesSize);

		// write decompression data size
		CopyLumpToFile(&lumpDataStream, ANIMFILE_UNCOMPRESSEDFRAMESIZE, (ubyte*)&nFramesSize, sizeof(int));
		header.numLumps++;

		// write compressed frame data (decompress when loading MOP file)
		CopyLumpToFile(&lumpDataStream, ANIMFILE_COMPRESSEDFRAMES, pCompressedFrames, nCompressedFramesSize);
		header.numLumps++;

		PPFree(pCompressedFrames);
	}
	else
	{
		// write uncompressed frame data
		CopyLumpToFile(&lumpDataStream, ANIMFILE_ANIMATIONFRAMES, (ubyte*)m_animframes.ptr(), m_animframes.numElem() * sizeof(animframe_t));
	}

	CopyLumpToFile(&lumpDataStream, ANIMFILE_SEQUENCES, (ubyte*)m_sequences.ptr(), m_sequences.numElem() * sizeof(sequencedesc_t));
	CopyLumpToFile(&lumpDataStream, ANIMFILE_EVENTS, (ubyte*)m_events.ptr(), m_events.numElem() * sizeof(sequenceevent_t));
	CopyLumpToFile(&lumpDataStream, ANIMFILE_POSECONTROLLERS, (ubyte*)m_posecontrollers.ptr(), m_posecontrollers.numElem() * sizeof(posecontroller_t));
	header.numLumps += 3;

	IFilePtr file = g_fileSystem->Open(packageOutputFilename, "wb", SP_MOD);
	if(!file)
	{
		MsgError("Can't create file for writing!\n");
		return;
	}

	file->Write(&header, 1, sizeof(header));
	lumpDataStream.WriteToStream(file);

	Msg("Total written bytes: %d\n", file->Tell());
}
