//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <egf/model.h>
#include <egf/motionpackage.h>

struct KVSection;
struct animCaBoneFrames_t;

namespace SharedModel {
	struct dsmmodel_t;
}

class CMotionPackageGenerator
{
public:
	~CMotionPackageGenerator();
	bool CompileScript(const char* filename);
	void WriteAnimationPackage(const char* packageOutputFilename);

	// Setups ESA bones for conversion
	void SetupESABones(SharedModel::dsmmodel_t* pModel, animCaBoneFrames_t* bones);

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
	void TranslateAnimationFrames(studioBoneFrame_t* bone, Vector3D& offset);

	// Subtracts the animation frames
	// IT ONLY SUBTRACTS BY FIRST FRAME OF otherbone
	void SubtractAnimationFrames(studioBoneFrame_t* bone, studioBoneFrame_t* otherbone);

	// Advances every frame position on reversed velocity
	// Helps with motion capture issues.
	void VelocityBackTransform(studioBoneFrame_t* bone, Vector3D& velocity);

	// Fills empty frames of animation
	// and interpolates non-empty to them.
	void InterpolateBoneAnimationFrames(studioBoneFrame_t* bone);

	// Crops animated bones
	void CropAnimationBoneFrames(studioBoneFrame_t* pBone, int newStart, int newFrames);

	// Crops animation
	void CropAnimationDimensions(studioAnimation_t* pAnim, int newStart, int newFrames);

	// Reverse animated bones
	void ReverseAnimationBoneFrames(studioBoneFrame_t* pBone);

	// Reverse animation
	void ReverseAnimation(studioAnimation_t* pAnim);

	// Scales bone animation length
	void RemapBoneFrames(studioBoneFrame_t* pBone, int newLength);

	// Scales animation length
	void RemapAnimationLength(studioAnimation_t* pAnim, int newLength);

	// Loads animation from file
	int LoadAnimationFromESA(const char* filename);

	// Loads animation from key-values parameters and applies.
	void LoadAnimation(KVSection* section);

	// Parses animation list from script
	bool ParseAnimations(KVSection* section);

	// Parses pose parameters from script
	void ParsePoseparameters(KVSection* section);

	// Loads sequence parameters
	void LoadSequence(KVSection* section, const char* seq_name);

	// Parses sequence list
	void ParseSequences(KVSection* section);

	// Converts animations to writible format
	void ConvertAnimationsToWrite();

	// Makes standard pose.
	void MakeDefaultPoseAnimation();

	studiohdr_t*				m_model{ nullptr };
	Array<studioAnimation_t>	m_animations{ PP_SL };
	Array<sequencedesc_t>		m_sequences{ PP_SL };
	Array<sequenceevent_t>		m_events{ PP_SL };
	Array<posecontroller_t>		m_posecontrollers{ PP_SL };
	Array<animationdesc_t>		m_animationdescs{ PP_SL };
	Array<animframe_t>			m_animframes{ PP_SL };
	EqString					m_animPath{ "./" };
};