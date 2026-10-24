//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: Movie player video decoder interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "MovieDemuxWebM.h"

// matrix coefficients as defined by ISO/IEC 23091-4
enum EMovieColorMatrix : int
{
	MOVIE_COLOR_MATRIX_BT709 = 1,
	MOVIE_COLOR_MATRIX_UNSPECIFIED = 2,
	MOVIE_COLOR_MATRIX_BT470BG = 5,
	MOVIE_COLOR_MATRIX_BT601 = 6,
	MOVIE_COLOR_MATRIX_BT2020_NCL = 9,
};

enum EMovieDecodeResult : int
{
	MOVIE_DECODE_ERROR = -1,
	MOVIE_DECODE_AGAIN = 0,
	MOVIE_DECODE_OK,
};

// decoded 8 bit 4:2:0 frame, planes stay valid while the frame is referenced
struct MovieVideoFrame : public RefCountedObject<MovieVideoFrame>
{
	const ubyte*		planes[3]{ nullptr, nullptr, nullptr };
	int					strides[3]{ 0, 0, 0 };
	int					width{ 0 };
	int					height{ 0 };
	int64				ptsNs{ 0 };
	EMovieColorMatrix	colorMatrix{ MOVIE_COLOR_MATRIX_BT709 };
	bool				fullRange{ false };
};
using MovieVideoFramePtr = CRefPtr<MovieVideoFrame>;

class IMovieVideoDecoder
{
public:
	virtual ~IMovieVideoDecoder() = default;

	virtual bool				Init(const IVector2D& videoSize) = 0;
	virtual void				Shutdown() = 0;

	// MOVIE_DECODE_AGAIN means that frames must be received before sending the same packet again
	virtual EMovieDecodeResult	SendPacket(const MoviePacket& packet) = 0;

	// MOVIE_DECODE_AGAIN means that decoder needs more packets
	virtual EMovieDecodeResult	ReceiveFrame(MovieVideoFramePtr& frame) = 0;

	// drops pending packets and frames
	virtual void				Flush() = 0;
};

IMovieVideoDecoder*	CreateMovieVideoDecoderDav1d();
