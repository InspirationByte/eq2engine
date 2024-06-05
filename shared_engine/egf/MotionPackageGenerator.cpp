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

#define MIN_MOTIONPACKAGE_SIZE		(16 * 1024 * 1024)
#define BONE_NOT_SET				(65536)

namespace SharedModel {
	extern float readFloat(Tokenizer& tok);
}

using namespace SharedModel;

static void FreeAnimationData(DSAnimData* anim, int numBones)
{
	if (anim->bones)
	{
		for (int i = 0; i < numBones; i++)
			delete [] anim->bones[i].keyFrames;
	}
	delete [] anim->bones;
}

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
	for(DSAnimData& anim : m_animations)
		FreeAnimationData(&anim, m_model->numBones);

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
		if(!CString::CompareCaseIns(m_sequences[i].name, name))
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
		if(!m_animations[i].name.CompareCaseIns(name))
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
		if(!CString::CompareCaseIns(m_posecontrollers[i].name, name))
			return i;
	}

	return -1;
}

//*******************************************************
// TODO: use from bonesetup.h
//*******************************************************
void CMotionPackageGenerator::TranslateAnimationFrames(DSBoneFrames* bone, const Vector3D &offset)
{
	for(int i = 0; i < bone->numFrames; i++)
	{
		bone->keyFrames[i].position += offset;
	}
}

//******************************************************
// Subtracts the animation frames
// IT ONLY SUBTRACTS BY FIRST FRAME OF otherbone
//******************************************************
void CMotionPackageGenerator::SubtractAnimationFrames(DSBoneFrames* bone, DSBoneFrames* otherbone)
{
	for(int i = 0; i < bone->numFrames; i++)
	{
		bone->keyFrames[i].position -= otherbone->keyFrames[0].position;
		bone->keyFrames[i].angles -= otherbone->keyFrames[0].angles;
	}
}

//*******************************************************
// Subtracts root motion from the bone transform
//*******************************************************
void CMotionPackageGenerator::VelocityBackTransform(DSBoneFrames* bone, const Vector3D &velocity)
{
	for(int i = 0; i < bone->numFrames; i++)
	{
		bone->keyFrames[i].position -= velocity * float(i);
	}
}

//*******************************************************
// key marked as empty?
//*******************************************************
inline bool IsEmptyKeyframe(const DSAnimFrame& keyFrame)
{
	if(keyFrame.position.x == BONE_NOT_SET || keyFrame.angles.x == BONE_NOT_SET)
		return true;

	return false;
}

//*******************************************************
// Fills empty frames of animation
// and interpolates non-empty to them.
//*******************************************************
void CMotionPackageGenerator::InterpolateBoneAnimationFrames(DSBoneFrames* bone)
{
	float lastKeyframeTime = 0;
	float nextKeyframeTime = 0;

	DSAnimFrame* lastKeyFrame = nullptr;
	DSAnimFrame* nextInterpKeyFrame = nullptr;

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
			if(!nextInterpKeyFrame && !IsEmptyKeyframe(bone->keyFrames[j]))
			{
				//Msg("Interpolation destination key set to %d\n", j);
				nextInterpKeyFrame = &bone->keyFrames[j];
				nextKeyframeTime = j;
				break;
			}
		}

		if(lastKeyFrame && nextInterpKeyFrame)
		{
			if(!IsEmptyKeyframe(bone->keyFrames[i]))
			{
				//Msg("Interp ends\n");
				lastKeyFrame = nullptr;
				nextInterpKeyFrame = nullptr;
			}
			else
			{
				if(IsEmptyKeyframe(*lastKeyFrame))
					MsgError("Can't interpolate when start frame is NULL\n");

				if(IsEmptyKeyframe(*nextInterpKeyFrame))
					MsgError("Can't interpolate when end frame is NULL\n");

				// do basic interpolation
				bone->keyFrames[i].position = lerp(lastKeyFrame->position, nextInterpKeyFrame->position, (float)(i - lastKeyframeTime) / (float)(nextKeyframeTime - lastKeyframeTime));
				bone->keyFrames[i].angles = lerp(lastKeyFrame->angles, nextInterpKeyFrame->angles, (float)(i - lastKeyframeTime) / (float)(nextKeyframeTime - lastKeyframeTime));
			}
		}
		else if(lastKeyFrame && !nextInterpKeyFrame)
		{
			// if there is no next frame, don't do interpolation, simple copy
			bone->keyFrames[i].position = lastKeyFrame->position;
			bone->keyFrames[i].angles = lastKeyFrame->angles;
		}

		if(!lastKeyFrame)
		{
			lastKeyframeTime = i;
			lastKeyFrame = &bone->keyFrames[i]; // set last key frame
			continue;
		}

		for(int j = i; j < bone->numFrames; j++)
		{
			if(!nextInterpKeyFrame && !IsEmptyKeyframe(bone->keyFrames[j]))
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
inline void InterpolateFrameTransform(DSAnimFrame& frame1, DSAnimFrame& frame2, float value, DSAnimFrame& out)
{
	out.angles = lerp(frame1.angles, frame2.angles, value);
	out.position = lerp(frame1.position, frame2.position, value);
}

//************************************
// Crops animated bones
//************************************
void CMotionPackageGenerator::CropAnimationBoneFrames(DSBoneFrames* pBone, int newStart, int newEnd)
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

	DSAnimFrame* newFrames = PPNew DSAnimFrame[newLength];

	for(int i = 0; i < newLength; i++)
		newFrames[i] = pBone->keyFrames[i + newStart];

	delete [] pBone->keyFrames;
	pBone->keyFrames = newFrames;
	pBone->numFrames = newLength;
}

//************************************
// Crops animation
//************************************
void CMotionPackageGenerator::CropAnimationDimensions(DSAnimData* pAnim, int newStart, int newEnd)
{
	for(int i = 0; i < m_model->numBones; i++)
		CropAnimationBoneFrames(&pAnim->bones[i], newStart, newEnd);
}

//************************************
// Reverse animation
//************************************

void CMotionPackageGenerator::ReverseAnimation(DSAnimData* pAnim )
{
	for(int i = 0; i < m_model->numBones; i++)
		arrayReverse(&pAnim->bones[i], 0, pAnim->bones[i].numFrames);
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
void CMotionPackageGenerator::RemapBoneFrames(DSBoneFrames* pBone, int newLength)
{
	BitArray setFrames(PP_SL, max(pBone->numFrames, newLength));

	DSAnimFrame* newFrames = PPNew DSAnimFrame[newLength];
	for(int i = 0; i < newLength; i++)
	{
		newFrames[i].position.x = BONE_NOT_SET;
		newFrames[i].angles.x = BONE_NOT_SET;
	}

	const float frameFactor = (float)newLength / (float)pBone->numFrames;
	const float frameFactorToOld = (float)pBone->numFrames / (float)newLength;

	setFrames.setTrue(0);
	setFrames.setTrue(newLength-1);

	newFrames[0] = pBone->keyFrames[0];
	newFrames[newLength-1] = pBone->keyFrames[pBone->numFrames-1];

	for(int i = 0; i < pBone->numFrames; i++)
	{
		if (setFrames[i])
			continue;

		const float timeFactorToNew = frameFactor * (float)i;

		int newFrameA = 0;
		int newFrameB = 0;
		GetCurrAndNextFrameFromTime(timeFactorToNew, newLength, &newFrameA, &newFrameB);

		int oldFrameA = 0;
		int oldFrameB = 0;
		const float timeFactorToOld = frameFactorToOld * (float)newFrameA;
		GetCurrAndNextFrameFromTime(timeFactorToOld, pBone->numFrames, &oldFrameA, &oldFrameB);

		const float timeInterp = timeFactorToOld - (int)oldFrameA;
		InterpolateFrameTransform(pBone->keyFrames[oldFrameA], pBone->keyFrames[oldFrameB], timeInterp, newFrames[newFrameA]);
		setFrames.setTrue(i);
	}

	// finally, replace bones
	delete[] pBone->keyFrames;

	pBone->keyFrames = newFrames;
	pBone->numFrames = newLength;

	InterpolateBoneAnimationFrames(pBone);
}

//************************************
// Scales animation length
//************************************
void CMotionPackageGenerator::RemapAnimationLength(DSAnimData* pAnim, int newLength)
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

static bool ReadFrames(CMotionPackageGenerator& generator, Tokenizer& tok, DSModel* pModel, DSAnimData* pAnim)
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
		const int numFrames = bones[i].frames.numElem();
		pAnim->bones[i].numFrames = numFrames;
		pAnim->bones[i].keyFrames = PPNew DSAnimFrame[numFrames];

		// copy frames
		memcpy(pAnim->bones[i].keyFrames, bones[i].frames.ptr(), numFrames * sizeof(DSAnimFrame));

		// try fix and iterpolate
		//InterpolateBoneAnimationFrames( &currentAnim->bones[i] );

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
	DSAnimData& modelAnim = m_animations.append();
	modelAnim.name = filename;

	DSModel tempDSM;

	char* str;
	while ((str = tok.next()) != nullptr)
	{
		if(!CString::CompareCaseIns(str, "bones"))
		{
			if(!ReadBones(tok, &tempDSM))
				return -1;
		}
		else if(!CString::CompareCaseIns(str, "frames"))
		{
			if(m_model->numBones != tempDSM.bones.numElem())
			{
				MsgError("Invalid bones! Please re-export model!\n");
				FreeAnimationData(&modelAnim, m_model->numBones);
				m_animations.removeIndex(newAnimIndex);
				return -1;
			}

			modelAnim.bones = PPNew DSBoneFrames[m_model->numBones];

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
	DSAnimData& sourceAnim = m_animations[animIndex];

	const int newAnimIndex = m_animations.numElem();
	DSAnimData& modelAnim = m_animations.append();

	// some data should work as simple as that
	// but we have to clone keyframe data
	modelAnim = sourceAnim;

	modelAnim.bones = PPNew DSBoneFrames[m_model->numBones];
	for (int i = 0; i < m_model->numBones; ++i)
	{
		const int numFrames = sourceAnim.bones[i].numFrames;
		modelAnim.bones[i].numFrames = numFrames;
		modelAnim.bones[i].keyFrames = PPNew DSAnimFrame[numFrames];

		// copy frames
		memcpy(modelAnim.bones[i].keyFrames, sourceAnim.bones[i].keyFrames, numFrames * sizeof(DSAnimFrame));
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

	const KVSection* externalpath = section->FindSection("externalFile");
	if(externalpath)
		animName = KV_GetValueString(externalpath);

	Msg(" loading animation '%s' as '%s'\n", animName, KV_GetValueString(section));

	KVSection* offsetPos = section->FindSection("offset");

	Vector3D animOffsetPos = KV_GetVector3D(offsetPos, 0, vec3_zero);
	Vector3D animRootVelocity = vec3_zero;

	KVSection* moveVelocityKey = section->FindSection("moveVelocity");
	if(moveVelocityKey)
	{
		animRootVelocity = KV_GetVector3D(moveVelocityKey, 0, vec3_zero);
		const float frameRate = KV_GetValueFloat(moveVelocityKey, 3, 1.0f);

		animRootVelocity /= frameRate;
	}

	bool doCut = false;
	int cropFrom = 0;
	int cropTo = -1;

	const KVSection* cropAnimKey = section->FindSection("crop");
	if(cropAnimKey)
	{
		cropFrom = KV_GetValueInt(cropAnimKey, 0, -1);
		cropTo = KV_GetValueInt(cropAnimKey, 0, -1);

		if(cropFrom == -1)
		{
			MsgError("Key error: crop must have at least one value (min)\n");
			MsgWarning("Usage: crop (min frame) (max frame)\n");
		}
		else
			doCut = true;
	}

	int animIdx = GetAnimationIndex(animName);
	if(animIdx != -1)
		animIdx = DuplicateAnimationByIndex(animIdx);

	if (animIdx == -1) // try to load new one if not found
		animIdx = LoadAnimationFromESA(animName);

	if(animIdx == -1)
	{
		MsgError("ERROR: Cannot open animation file '%s'!\n", animName);
		return;
	}

	DSAnimData* currentAnim = &m_animations[ animIdx ];
	for(int i = 0; i < m_model->numBones; i++)
	{
		if (m_model->pBone(i)->parent != -1)
			continue;

		if(length(animOffsetPos) > 0)
			Msg("Animation offset: %f %f %f\n", animOffsetPos.x, animOffsetPos.y, animOffsetPos.z);

		// move bones to user-defined position
		TranslateAnimationFrames(&currentAnim->bones[i], animOffsetPos);

		// transform model back using linear moveVelocityKey
		VelocityBackTransform(&currentAnim->bones[i], animRootVelocity);
	}

	const KVSection* subtractKey = section->FindSection("subtract");
	if(subtractKey)
	{
		const int subtractByAnimIdx = GetAnimationIndex( KV_GetValueString(subtractKey) );
		if(subtractByAnimIdx != -1)
		{
			for(int i = 0; i < m_model->numBones; i++)
				SubtractAnimationFrames(&currentAnim->bones[i], &m_animations[subtractByAnimIdx].bones[i]);
		}
		else
		{
			MsgError("Subtract: animation %s not found\n", KV_GetValueString(subtractKey));
		}
	}

	if(doCut)
	{
		Msg("  Cropping from [0 %d] to [%d %d]\n", currentAnim->bones[0].numFrames, cropFrom, cropTo);
		CropAnimationDimensions( currentAnim, cropFrom, cropTo );

		if(cropFrom == cropTo)
			RemapAnimationLength(currentAnim, 2);
	}

	const int customLength = KV_GetValueInt(section->FindSection("customLength"), 0, -1);
	if(customLength != -1)
	{
		Msg("Changing length to %d\n", customLength);
		RemapAnimationLength( currentAnim, customLength );
	}

	const bool reverse = KV_GetValueBool(section->FindSection("reverse"), 0, false);
	if(reverse)
	{
		Msg("Reverse\n");
		ReverseAnimation( currentAnim );
	}

	// make final name
	currentAnim->name = KV_GetValueString(section);
}

//************************************
// Parses animation list from script
//************************************
bool CMotionPackageGenerator::ParseAnimations(const KVSection* section)
{
	Msg("Processing animations\n");
	for(KVKeyIterator it(section, "animation"); !it.atEnd(); ++it)
		LoadAnimation(*it);

	return m_animations.numElem() > 0;
}

//************************************
// Parses pose parameters from script
//************************************
void CMotionPackageGenerator::ParsePoseparameters(const KVSection* section)
{
	Msg("Processing pose parameters\n");
	for (KVKeyIterator it(section, "poseParameter"); !it.atEnd(); ++it)
	{
		const KVSection* poseParamKey = *it;
		if(poseParamKey->values.numElem() < 3)
		{
			MsgError("Incorrect usage. Example: poseparameter <poseparam name> <min range> <max range>");
			continue;
		}

		posecontroller_t& controller = m_posecontrollers.append();

		strcpy(controller.name, KV_GetValueString(poseParamKey, 0));
		controller.blendRange[0] = KV_GetValueFloat(poseParamKey, 1);
		controller.blendRange[1] = KV_GetValueFloat(poseParamKey, 2);
	}
}

//************************************
// Loads sequence parameters
//************************************
void CMotionPackageGenerator::LoadSequence(const KVSection* section, const char* seqName)
{
	sequencedesc_t desc;
	memset(&desc,0,sizeof(sequencedesc_t));
	strcpy(desc.name, seqName);

	// UNDONE: duplicate animation and translate if the key "translate" found.

	for (KVKeyIterator it(section, "sequenceLayer"); !it.atEnd(); ++it)
	{
		const int seqIndex = GetSequenceIndex(KV_GetValueString(*it));
		if(seqIndex == -1)
		{
			MsgError("No such sequence '%s' for making layer in sequence '%s'\n", KV_GetValueString(*it), seqName);
			return;
		}

		if (desc.numSequenceBlends >= MAX_SEQUENCE_BLENDS)
		{
			MsgError("Too many sequence layers in '%s' (max %d)\n", seqName, MAX_SEQUENCE_BLENDS);
			return;
		}

		desc.sequenceblends[desc.numSequenceBlends++] = seqIndex;
	}

	// length alignment, for differrent animation lengths, takes the first animation as etalon
	bool alignAnimationLengths = KV_GetValueBool(section->FindSection("alignLengths"));

	if(g_cmdLine->FindArgument("-forceAlign") != -1)
		alignAnimationLengths = true;

	// parse default parameters
	const KVSection* frameRateKey = section->FindSection("frameRate");
	if(!frameRateKey)
		desc.framerate = 30;
	else
		desc.framerate = KV_GetValueFloat(frameRateKey);

	const KVSection* animListKvs = section->FindSection("weights");
	if(animListKvs)
	{
		desc.numAnimations = 0;
		for(KVKeyIterator it(animListKvs, "animation"); !it.atEnd(); ++it)
		{
			const char* animName = KV_GetValueString(*it);

			int animIdx = GetAnimationIndex(animName);
			if(animIdx == -1) // try to load new one if not found
				animIdx = LoadAnimationFromESA(animName);

			if(animIdx == -1)
			{
				MsgError("No such animation '%s'\n", animName);
				return;
			}

			if (desc.numAnimations >= MAX_BLEND_WIDTH)
			{
				MsgError("Too many weights in animation '%s' (max %d)\n", animName, MAX_BLEND_WIDTH);
				return;
			}

			desc.animations[desc.numAnimations++] = animIdx;
		}

		if (desc.numAnimations == 0)
		{
			MsgError("No values in 'weights' section.\n");
			return;
		}

		// check for validness
		int checkFrameCount = m_animations[desc.animations[0]].bones[0].numFrames;
		const int oldFrameCount = checkFrameCount;

		if(alignAnimationLengths)
		{
			for(int i = 0; i < desc.numAnimations; i++)
			{
				const int frameCount = m_animations[desc.animations[i]].bones[0].numFrames;
				checkFrameCount = max(checkFrameCount, frameCount);
			}

			desc.framerate *= (float)checkFrameCount / (float)oldFrameCount;
		}

		for(int i = 0; i < desc.numAnimations; i++)
		{
			const int frameCount = m_animations[desc.animations[i]].bones[0].numFrames;

			if(m_animations[desc.animations[i]].bones[0].numFrames != checkFrameCount)
			{
				if(alignAnimationLengths)
				{
					if(checkFrameCount < frameCount)
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

		int animIdx = GetAnimationIndex(KV_GetValueString(pKey));
		if(animIdx == -1) // try to load new one if not found
			animIdx = LoadAnimationFromESA(KV_GetValueString(pKey));

		if(animIdx == -1)
		{
			MsgError("No such animation %s\n", KV_GetValueString(pKey));
			return;
		}

		desc.animations[0] = animIdx;
	}

	if(desc.numAnimations == 0)
	{
		MsgError("No animations for sequence '%s'\n", desc.name);
		return;
	}

	// parse default parameters
	strcpy(desc.activity, KV_GetValueString(section->FindSection("activity"), 0, "ACT_INVALID"));

	Msg("  Adding sequence '%s' with activity '%s'\n", desc.name, desc.activity);
	{
		desc.flags |= KV_GetValueBool(section->FindSection("loop")) ? SEQFLAG_LOOP : 0;
		desc.flags |= KV_GetValueBool(section->FindSection("slotBlend")) ? SEQFLAG_SLOTBLEND : 0;
	}

	// parse transitiontime value
	desc.transitiontime = KV_GetValueFloat(section->FindSection("transitionTime"), 0, SEQ_DEFAULT_TRANSITION_TIME);

	// parse events
	const KVSection* eventList = section->FindSection("events");
	if(eventList)
	{
		int kvSecCount = 0;
		for(const KVSection* sec : eventList->keys)
		{
			const float eventTime = (float)atof(sec->name);
			const KVSection* pEventCommand = sec->FindSection("command");
			const KVSection* pEventOptions = sec->FindSection("options");

			if(pEventCommand && pEventOptions)
			{
				if (desc.numEvents >= MAX_EVENTS_PER_SEQ)
				{
					MsgError("Too many events in sequence %s (max %d)\n", seqName, MAX_EVENTS_PER_SEQ);
					return;
				}

				// create event
				sequenceevent_t seqEvent;
				memset(&seqEvent, 0, sizeof(sequenceevent_t));

				seqEvent.frame = eventTime;
				strcpy(seqEvent.command, KV_GetValueString(pEventCommand));
				strcpy(seqEvent.parameter, KV_GetValueString(pEventOptions));

				// add event to list.
				desc.events[desc.numEvents++] = m_events.append(seqEvent);
			}
			else
			{
				MsgWarning("Event %d for sequence %s missing command and options\n", kvSecCount, desc.name);
			}
			++kvSecCount;
		}
	}

	const char* poseParameterName = KV_GetValueString(section->FindSection("poseParameter"), 0, nullptr);
	if (poseParameterName)
	{
		desc.posecontroller = GetPoseControllerIndex(poseParameterName);
		if (desc.posecontroller == -1)
			MsgWarning("Pose parameter '%s' for sequence '%s' not found (poseParameter)\n", poseParameterName, seqName);
	}
	else
		desc.posecontroller = -1;

	const char* timeParameterName = KV_GetValueString(section->FindSection("timeParameter"), 0, nullptr);
	if (timeParameterName)
	{
		desc.timecontroller = GetPoseControllerIndex(timeParameterName);
		if (desc.timecontroller == -1)
			MsgWarning("Pose parameter '%s' for sequence '%s' not found (timeParameter)\n", timeParameterName, seqName);
	}
	else
		desc.timecontroller = -1;

	desc.activityslot = KV_GetValueInt(section->FindSection("activitySlot"), 0, 0);

	// add sequence
	m_sequences.append(desc);
}

//************************************
// Parses sequence list
//************************************
void CMotionPackageGenerator::ParseSequences(const KVSection* section)
{
	Msg("Processing sequences\n");
	for (KVKeyIterator it(section, "sequence"); !it.atEnd(); ++it)
	{
		const KVSection* seqSec = *it;
		LoadSequence(seqSec, KV_GetValueString(seqSec, 0, "unnamed_seq"));
	}
}

//************************************
// Converts animations to writible format
//************************************
void CMotionPackageGenerator::ConvertAnimationsToWrite()
{
	for(DSAnimData& studioAnim : m_animations)
	{
		animationdesc_t& animDesc = m_animationdescs.append();
		memset(&animDesc, 0, sizeof(animationdesc_t));

		animDesc.firstFrame = m_animframes.numElem();
		strcpy(animDesc.name, studioAnim.name);

		// convert bones.
		for(int i = 0; i < m_model->numBones; i++)
		{
			const DSBoneFrames& boneFrame = studioAnim.bones[i];
			for(int j = 0; j < boneFrame.numFrames; ++j)
			{
				const DSAnimFrame& srcFrame = boneFrame.keyFrames[j];

				animframe_t& destFrame = m_animframes.append();
				destFrame.vecBonePosition = srcFrame.position;
				destFrame.angBoneAngles = srcFrame.angles;
			}

			animDesc.numFrames += boneFrame.numFrames;
		}	
	}
}

//************************************
// Makes standard pose.
//************************************
void CMotionPackageGenerator::MakeDefaultPoseAnimation()
{
	// make new model animation
	DSAnimData modelAnim;
	modelAnim.name = "default"; // set it externally from file name

	modelAnim.bones = PPNew DSBoneFrames[m_model->numBones];
	for (int i = 0; i < m_model->numBones; i++)
	{
		DSBoneFrames& boneFrame = modelAnim.bones[i];
		boneFrame.numFrames = 1;
		boneFrame.keyFrames = PPNew DSAnimFrame[1];
		boneFrame.keyFrames[0].angles = vec3_zero;
		boneFrame.keyFrames[0].position = vec3_zero;
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

	m_animPath = fnmPathStripName(filename);

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

void CMotionPackageGenerator::WriteAnimationPackage(const char* packageOutputFilename)
{
	CMemoryStream lumpDataStream(nullptr, VS_OPEN_WRITE, MIN_MOTIONPACKAGE_SIZE, PP_SL);

	lumpfilehdr_t header;
	header.ident = ANIMFILE_IDENT;
	header.version = ANIMFILE_VERSION;
	header.numLumps = 0;

	// separate m_animations on m_animationdescs and m_animframes
	ConvertAnimationsToWrite();

	CopyLumpToFile(&lumpDataStream, ANIMFILE_ANIMATIONS, (ubyte*)m_animationdescs.ptr(), m_animationdescs.numElem() * sizeof(animationdesc_t));
	header.numLumps++;

	// try to compress frame data
	const int framesLumpSize = m_animframes.numElem() * sizeof(animframe_t);

	unsigned long nCompressedFramesSize = framesLumpSize + 150;
	ubyte* pCompressedFrames = (ubyte*)PPAlloc(nCompressedFramesSize);

	memset(pCompressedFrames, 0, nCompressedFramesSize);

	int compressStatus = Z_ERRNO;

	// do not compress animation frames if option found
	if(g_cmdLine->FindArgument("-nocompress") == -1)
		compressStatus = compress2(pCompressedFrames, &nCompressedFramesSize, (ubyte*)m_animframes.ptr(), framesLumpSize, 9);

	if(compressStatus == Z_OK)
	{
		MsgWarning("Successfully compressed frame data from %d to %d bytes\n", framesLumpSize, nCompressedFramesSize);

		// write decompression data size
		CopyLumpToFile(&lumpDataStream, ANIMFILE_UNCOMPRESSEDFRAMESIZE, (ubyte*)&framesLumpSize, sizeof(int));
		header.numLumps++;

		// write compressed frame data (decompress when loading MOP file)
		CopyLumpToFile(&lumpDataStream, ANIMFILE_COMPRESSEDFRAMES, pCompressedFrames, nCompressedFramesSize);
		header.numLumps++;

		PPFree(pCompressedFrames);
	}
	else
	{
		// write uncompressed frame data
		CopyLumpToFile(&lumpDataStream, ANIMFILE_ANIMATIONFRAMES, (ubyte*)m_animframes.ptr(), framesLumpSize);
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

	Msg("Total written bytes: %" PRId64 "\n", file->Tell());
}
