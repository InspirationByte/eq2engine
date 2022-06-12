//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech engine model animation package format desc
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define ANIMFILE_IDENT				MCHAR4('A','N','M','C')
#define ANIMFILE_VERSION			5

// use tag ANIMCA_VERSION change

#define MAX_BLEND_WIDTH			16	// 16 animations for blending.
#define MAX_EVENTS_PER_SEQ		16	// 16 events per one sequence
#define MAX_SEQUENCE_BLENDS		4	// 4 blend layers

#define SEQ_DEFAULT_TRANSITION_TIME	(0.15f)

enum EAnimLumps
{
	ANIMFILE_ANIMATIONS = 0,		// animation headers
	ANIMFILE_SEQUENCES,				// sequence headers
	ANIMFILE_EVENTS,				// event datas
	ANIMFILE_POSECONTROLLERS,		// pose controllers
	ANIMFILE_ANIMATIONFRAMES,		// uncompressed animation frames
	ANIMFILE_UNCOMPRESSEDFRAMESIZE,	// compressed frame size info
	ANIMFILE_COMPRESSEDFRAMES,		// compressed animation frames
	ANIMFILE_LUMPS,
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

// MOP file header
struct animpackagehdr_s
{
	int ident;
	int version;
	int numLumps;
};
ALIGNED_TYPE(animpackagehdr_s, 4) animpackagehdr_t;

struct animpackagelump_s
{
	int				type; // ANIMCA_* type
	int				size; // size excluding this structure
};
ALIGNED_TYPE(animpackagelump_s, 4) animpackagelump_t;

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

	float	framerate;			// framerate
	float	transitiontime;	// transition time to this animation, in seconds

	// used pose controller - one per sequence
	int8	posecontroller;

	// sequence layers that used for blending.
	int8	numSequenceBlends;
	int8	sequenceblends[MAX_SEQUENCE_BLENDS];

	// animation (weights)
	int8	numAnimations;
	int8	animations[MAX_BLEND_WIDTH];

	// events
	int		numEvents;
	int		events[MAX_EVENTS_PER_SEQ];

	// sequence flags
	int		flags;
};
ALIGNED_TYPE(sequencedesc_s, 4) sequencedesc_t;

// sequence events
struct sequenceevent_s
{
	float frame;

	char command[44];
	char parameter[44];
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

