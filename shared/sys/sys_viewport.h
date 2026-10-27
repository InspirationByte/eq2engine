//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: Host viewport - app state rendering inside an ImGui window
//////////////////////////////////////////////////////////////////////////////////

#pragma once

// viewport is never visible in Retail and Profile builds, states render to the backbuffer
#if !defined(_RETAIL) && !defined(_PROFILE)
#define HOST_VIEWPORT_ENABLED
#endif

class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

// Render target shown in an ImGui window. App state returns it from GetViewport(),
// host then sends mouse and keyboard input to the state in viewport coordinates.
class CHostViewport
{
public:
	CHostViewport(const char* name);
	~CHostViewport();

	// draws viewport in the current ImGui window; zero size component takes the available content region
	void				DrawImGui(const Vector2D& size = vec2_zero);
	void				ReleaseTargets();

	// drawn in the last ImGui frame
	bool				IsVisible() const;
	bool				IsHovered() const;
	bool				IsFocused() const;
	bool				IsMouseOver(const IVector2D& windowPos) const;

	// fall back to the backbuffer and window size when not visible
	ITexturePtr			GetColorTarget() const;
	ITexturePtr			GetDepthTarget() const;
	IVector2D			GetSize() const;

	// rectangle in window coordinates
	const IAARectangle&	GetScreenRect() const { return m_screenRect; }
	IVector2D			ToViewport(const IVector2D& windowPos) const;
	IVector2D			ToWindow(const IVector2D& viewportPos) const;

	// mouse buttons (MOU_B* bits) pressed inside the viewport
	bool				IsMouseCaptured() const { return m_capturedButtons != 0; }
	void				SetMouseButtonCaptured(int buttonBits, bool captured);
	void				ResetMouseCapture() { m_capturedButtons = 0; }

private:
	void				UpdateTargets(const IVector2D& size);

	EqString			m_name;
	ITexturePtr			m_colorTarget;
	ITexturePtr			m_depthTarget;
	IAARectangle		m_screenRect{ 0, 0, 0, 0 };
	int					m_drawFrame{ -1 };
	int					m_capturedButtons{ 0 };
	bool				m_hovered{ false };
	bool				m_focused{ false };
};
