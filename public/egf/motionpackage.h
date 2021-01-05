//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech engine model animation package format desc
//////////////////////////////////////////////////////////////////////////////////

#ifndef ANIMATIONFILE_H
#define ANIMATIONFILE_H

#include "platform/Platform.h"

#define ANIMCA_IDENT			(('C'<<24)+('M'<<16)+('N'<<8)+'A') // MOTN for MOTioN data
#define ANIMCA_VERSION			5

enum AnimCA_Lumps_e
{
	ANIMCA_ANIMATIONS = 0,			// animation headers
	ANIMCA_SEQUENCES,				// sequence headers
	ANIMCA_EVENTS,					// event datas
	ANIMCA_POSECONTROLLERS,			// pose controllers
	ANIMCA_ANIMATIONFRAMES,			// uncompressed animation frames
	ANIMCA_UNCOMPRESSEDFRAMESIZE,	// compressed frame size info
	ANIMCA_COMPRESSEDFRAMES,		// compressed animation frames

	ANIMCA_LUMPS,
};

// EQ format limits, change requries ANIMCA_VERSION changing

#define MAX_BLEND_WIDTH			16	// 16 animations for blending.
#define MAX_EVENTS_PER_SEQ		16	// 16 events per one sequence
#define MAX_SEQUENCE_BLENDS		4	// 4 blend layers

#define DEFAULT_TRANSITION_TIME	0.15f

// sequence flags
#define SEQFLAG_LOOP			(1 << 1)	// this animation is looped
#define SEQFLAG_NOTRANSITION	(1 << 2)	// has disabled blending transition
#define SEQFLAG_AUTOPLAY		(1 << 3)	// automatically plays if no animation plays
#define SEQFLAG_SLOTBLEND		(1 << 4)	// part blend to slot instead of interpolation
//#define SEQFLAG_LERPPOSE		(1 << 5)	// wrap weights

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

#endif //ANIMATIONFILE_H
