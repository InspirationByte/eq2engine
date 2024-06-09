//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "dsm_loader.h"
#include "egf/model.h"

struct KVSection;
struct animCaBoneFrames_t;

class CMotionPackageGenerator
{
public:
	~CMotionPackageGenerator();
	bool CompileScript(const char* filename);
	void WriteAnimationPackage(const char* packageOutputFilename);

	// Setups ESA bones for conversion
	void SetupESABones(SharedModel::DSModel* pModel, animCaBoneFrames_t* bones);

private:

	void Cleanup();

	// Finds and returns sequence index by name
	int	GetSequenceIndex(const char* name);

	// Finds and returns animation index by name
	int	GetAnimationIndex(const char* name);

	// Finds and returns pose controller index by name
	int	GetPoseControllerIndex(const char* name);

	// Shifts animation start
	//void ShiftAnimationFrames(studioBoneFrame_t* bone, int new_start_frame);
	void TranslateAnimationFrames(SharedModel::DSBoneFrames* bone, const Vector3D& offset);

	// Subtracts the animation frames
	// IT ONLY SUBTRACTS BY FIRST FRAME OF otherbone
	void SubtractAnimationFrames(SharedModel::DSBoneFrames* bone, SharedModel::DSBoneFrames* otherbone);

	// Advances every frame position on reversed velocity
	// Helps with motion capture issues.
	void VelocityBackTransform(SharedModel::DSBoneFrames* bone, const Vector3D& velocity);

	// Fills empty frames of animation
	// and interpolates non-empty to them.
	void InterpolateBoneAnimationFrames(SharedModel::DSBoneFrames* bone);

	// Crops animated bones
	void CropAnimationBoneFrames(SharedModel::DSBoneFrames* pBone, int newStart, int newEnd);

	// Crops animation
	void CropAnimationDimensions(SharedModel::DSAnimData* pAnim, int newStart, int newEnd);

	// Reverse animation
	void ReverseAnimation(SharedModel::DSAnimData* pAnim);

	// Scales bone animation length
	void RemapBoneFrames(SharedModel::DSBoneFrames* pBone, int newLength);

	// Scales animation length
	void RemapAnimationLength(SharedModel::DSAnimData* pAnim, int newLength);

	// Loads all animations from FBX
	void LoadFBXAnimations(const KVSection* section);

	// Loads animation from file
	int LoadAnimationFromESA(const char* filename);

	// duplicates the animation for further processing. Returns new index
	int DuplicateAnimationByIndex(int animIndex);

	// Loads animation from key-values parameters and applies.
	void LoadAnimation(const KVSection* section);

	// Parses animation list from script
	bool ParseAnimations(const KVSection* section);

	// Parses pose parameters from script
	void ParsePoseparameters(const KVSection* section);

	// Loads sequence parameters
	void LoadSequence(const KVSection* section, const char* seq_name);

	// Parses sequence list
	void ParseSequences(const KVSection* section);

	// Converts animations to writible format
	void ConvertAnimationsToWrite();

	// Makes standard pose.
	void MakeDefaultPoseAnimation();

	studioHdr_t*					m_model{ nullptr };
	Array<SharedModel::DSAnimData>	m_animations{ PP_SL };
	Array<sequencedesc_t>			m_sequences{ PP_SL };
	Array<sequenceevent_t>			m_events{ PP_SL };
	Array<posecontroller_t>			m_posecontrollers{ PP_SL };
	Array<animationdesc_t>			m_animationdescs{ PP_SL };
	Array<animframe_t>				m_animframes{ PP_SL };

	EqString						m_animPath{ "./" };
};