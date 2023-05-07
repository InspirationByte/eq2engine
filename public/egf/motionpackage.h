//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech engine model animation package format desc
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define ANIMFILE_IDENT				MCHAR4('E','Q','M','P')
#define ANIMFILE_VERSION			6

// use tag ANIMCA_VERSION change

static constexpr const int MAX_BLEND_WIDTH		= 16;	// animations for blending.
static constexpr const int MAX_EVENTS_PER_SEQ	= 16;	// events per one sequence
static constexpr const int MAX_SEQUENCE_BLENDS	= 4;	// blend layers

#define SEQ_DEFAULT_TRANSITION_TIME	(0.15f)

enum EAnimLumps
{
	ANIMFILE_ANIMATIONS				= 0,		// animation headers
	ANIMFILE_SEQUENCES				= 1,		// sequence headers
	ANIMFILE_EVENTS					= 2,		// event datas
	ANIMFILE_POSECONTROLLERS		= 3,		// pose controllers
	ANIMFILE_ANIMATIONFRAMES		= 4,		// uncompressed animation frames
	ANIMFILE_UNCOMPRESSEDFRAMESIZE	= 5,		// compressed frame size info
	ANIMFILE_COMPRESSEDFRAMES		= 6,		// compressed animation frames

	ANIMFILE_LUMPS					= 7,
};


// sequence flags
enum EAnimSequenceFlags
{
	SEQFLAG_LOOP			= (1 << 1),	// this animation is looped
	SEQFLAG_NO_TRANSITION	= (1 << 2),	// has disabled blending transition
	SEQFLAG_AUTOPLAY		= (1 << 3),	// automatically plays if no animation plays
	SEQFLAG_SLOTBLEND		= (1 << 4),	// part blend to slot instead of interpolation
	//SEQFLAG_LERPPOSE		= (1 << 5)	// wrap weights

};

// animation descriptor
struct animationdesc_s
{
	char	name[44];

	int		firstFrame;	// first frame, first bone in ANIMCA_ANIMATIONFRAMES
	int		numFrames;	// NOTE: this is not a bone frame count, but you can compute it by multipling it on bone count.
};
ALIGNED_TYPE(animationdesc_s, 4) animationdesc_t;

// sequence descriptor
struct sequencedesc_s
{
	// basic parameters
	char	name[44];
	char	activity[44];

	int		flags;				// sequence flags

	float	framerate;			// framerate
	float	transitiontime;		// transition time to this animation, in seconds

	// used pose controller - one per sequence
	int8	posecontroller;
	int8	slot;

	// sequence layers that used for blending.
	int8	numSequenceBlends;
	int8	sequenceblends[MAX_SEQUENCE_BLENDS];

	// animation (weights)
	int8	numAnimations;
	int8	animations[MAX_BLEND_WIDTH];

	// events
	int8	numEvents;
	int8	events[MAX_EVENTS_PER_SEQ];
};
ALIGNED_TYPE(sequencedesc_s, 4) sequencedesc_t;

// sequence events
struct sequenceevent_s
{
	float	frame;

	char	command[44];
	char	parameter[44];
};
ALIGNED_TYPE(sequenceevent_s, 4) sequenceevent_t;

// animation frame for single bone.
struct animframe_s
{
	Vector3D vecBonePosition;
	Vector3D angBoneAngles;
};
ALIGNED_TYPE(animframe_s, 4) animframe_t;

// pose controllers
struct posecontroller_s
{
	char	name[44];

	// blending range
	float	blendRange[2];
};
ALIGNED_TYPE(posecontroller_s, 4) posecontroller_t;

