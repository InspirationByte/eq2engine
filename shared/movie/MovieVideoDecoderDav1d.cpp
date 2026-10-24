//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: AV1 video decoder using dav1d
//////////////////////////////////////////////////////////////////////////////////

#include <dav1d/dav1d.h>
#include "core/core_common.h"
#include "MovieVideoDecoder.h"

struct CMovieVideoFrameDav1d : public MovieVideoFrame
{
	~CMovieVideoFrameDav1d()
	{
		dav1d_picture_unref(&m_picture);
	}

	Dav1dPicture	m_picture{};
};

class CMovieVideoDecoderDav1d : public IMovieVideoDecoder
{
public:
	~CMovieVideoDecoderDav1d();

	bool				Init(const IVector2D& videoSize) override;
	void				Shutdown() override;

	EMovieDecodeResult	SendPacket(const MoviePacket& packet) override;
	EMovieDecodeResult	ReceiveFrame(MovieVideoFramePtr& frame) override;

	void				Flush() override;

private:
	Dav1dContext*		m_ctx{ nullptr };
	Dav1dData			m_pendingData{};
};

CMovieVideoDecoderDav1d::~CMovieVideoDecoderDav1d()
{
	Shutdown();
}

bool CMovieVideoDecoderDav1d::Init(const IVector2D& videoSize)
{
	Shutdown();

	Dav1dSettings settings;
	dav1d_default_settings(&settings);

	// thread count and frame delay are picked by dav1d from the number of CPU cores
	if (dav1d_open(&m_ctx, &settings) < 0)
	{
		m_ctx = nullptr;
		MsgError("Unable to initialize dav1d\n");
		return false;
	}

	DevMsg(DEVMSG_CORE, "dav1d %s initialized for %dx%d video\n", dav1d_version(), videoSize.x, videoSize.y);
	return true;
}

void CMovieVideoDecoderDav1d::Shutdown()
{
	dav1d_data_unref(&m_pendingData);

	if (m_ctx)
		dav1d_close(&m_ctx);
	m_ctx = nullptr;
}

EMovieDecodeResult CMovieVideoDecoderDav1d::SendPacket(const MoviePacket& packet)
{
	if (!m_ctx)
		return MOVIE_DECODE_ERROR;

	// data stays pending if decoder did not consume it in the previous call
	if (!m_pendingData.sz)
	{
		uint8_t* data = dav1d_data_create(&m_pendingData, packet.data.numElem());
		if (!data)
			return MOVIE_DECODE_ERROR;

		memcpy(data, packet.data.ptr(), packet.data.numElem());
		m_pendingData.m.timestamp = packet.ptsNs;
	}

	const int r = dav1d_send_data(m_ctx, &m_pendingData);
	if (r == DAV1D_ERR(EAGAIN))
		return MOVIE_DECODE_AGAIN;

	if (r < 0)
	{
		dav1d_data_unref(&m_pendingData);
		return MOVIE_DECODE_ERROR;
	}

	return m_pendingData.sz ? MOVIE_DECODE_AGAIN : MOVIE_DECODE_OK;
}

EMovieDecodeResult CMovieVideoDecoderDav1d::ReceiveFrame(MovieVideoFramePtr& frame)
{
	if (!m_ctx)
		return MOVIE_DECODE_ERROR;

	CRefPtr<CMovieVideoFrameDav1d> newFrame = CRefPtr_new(CMovieVideoFrameDav1d);
	Dav1dPicture& picture = newFrame->m_picture;

	const int r = dav1d_get_picture(m_ctx, &picture);
	if (r == DAV1D_ERR(EAGAIN))
		return MOVIE_DECODE_AGAIN;

	if (r < 0)
		return MOVIE_DECODE_ERROR;

	if (picture.p.bpc != 8 || picture.p.layout != DAV1D_PIXEL_LAYOUT_I420)
	{
		MsgError("Unsupported AV1 picture format (%d bit, layout %d), only 8 bit 4:2:0 is supported\n", picture.p.bpc, picture.p.layout);
		return MOVIE_DECODE_ERROR;
	}

	for (int i = 0; i < 3; ++i)
		newFrame->planes[i] = reinterpret_cast<const ubyte*>(picture.data[i]);

	// chroma planes share the same stride
	newFrame->strides[0] = static_cast<int>(picture.stride[0]);
	newFrame->strides[1] = static_cast<int>(picture.stride[1]);
	newFrame->strides[2] = static_cast<int>(picture.stride[1]);

	newFrame->width = picture.p.w;
	newFrame->height = picture.p.h;
	newFrame->ptsNs = picture.m.timestamp;

	if (picture.seq_hdr)
	{
		newFrame->colorMatrix = static_cast<EMovieColorMatrix>(picture.seq_hdr->mtrx);
		newFrame->fullRange = picture.seq_hdr->color_range != 0;
	}

	frame = MovieVideoFramePtr(newFrame.Ptr());
	return MOVIE_DECODE_OK;
}

void CMovieVideoDecoderDav1d::Flush()
{
	dav1d_data_unref(&m_pendingData);

	if (m_ctx)
		dav1d_flush(m_ctx);
}

IMovieVideoDecoder* CreateMovieVideoDecoderDav1d()
{
	return PPNew CMovieVideoDecoderDav1d();
}
