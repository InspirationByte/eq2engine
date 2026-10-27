//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: Host viewport - app state rendering inside an ImGui window
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "materialsystem1/IMaterialSystem.h"

#include "sys_host.h"
#include "sys_viewport.h"

// enabled by sys_viewport.h
#ifdef HOST_VIEWPORT_ENABLED
#include <imgui.h>
#endif // HOST_VIEWPORT_ENABLED

CHostViewport::CHostViewport(const char* name)
	: m_name(name)
{
}

CHostViewport::~CHostViewport()
{
	ReleaseTargets();
}

void CHostViewport::ReleaseTargets()
{
	m_colorTarget = nullptr;
	m_depthTarget = nullptr;
	m_drawFrame = -1;
	m_capturedButtons = 0;
	m_hovered = false;
	m_focused = false;
}

void CHostViewport::UpdateTargets(const IVector2D& size)
{
	// same formats as the backbuffer so state pipelines are the same
	if (!m_colorTarget)
	{
		const SamplerStateParams sampler(TEXFILTER_NEAREST, TEXADDRESS_CLAMP);
		m_colorTarget = g_renderAPI->CreateRenderTarget(
			Builder<TextureDesc>()
			.Name(EqString::Format("_hvp_%s", m_name))
			.Size(size.x, size.y)
			.Format(g_matSystem->GetCurrentBackbuffer()->GetFormat())
			.Sampler(sampler)
			.End()
		);

		m_depthTarget = g_renderAPI->CreateRenderTarget(
			Builder<TextureDesc>()
			.Name(EqString::Format("_hvp_%s_depth", m_name))
			.Size(size.x, size.y)
			.Format(g_matSystem->GetDefaultDepthBuffer()->GetFormat())
			.End()
		);
		return;
	}

	if (m_colorTarget->GetWidth() == size.x && m_colorTarget->GetHeight() == size.y)
		return;

	g_renderAPI->ResizeRenderTarget(m_colorTarget, { size.x, size.y, 1 });
	g_renderAPI->ResizeRenderTarget(m_depthTarget, { size.x, size.y, 1 });
}

void CHostViewport::DrawImGui(const Vector2D& size)
{
#ifdef HOST_VIEWPORT_ENABLED
	const ImVec2 availSize = ImGui::GetContentRegionAvail();
	const ImVec2 drawSize(size.x > 0.0f ? size.x : availSize.x, size.y > 0.0f ? size.y : availSize.y);
	const IVector2D targetSize(max(static_cast<int>(drawSize.x), 1), max(static_cast<int>(drawSize.y), 1));

	UpdateTargets(targetSize);

	const ImVec2 screenPos = ImGui::GetCursorScreenPos();
	const ImVec2 screenEnd(screenPos.x + targetSize.x, screenPos.y + targetSize.y);

	// button keeps window from being dragged and gives hover state
	ImGui::InvisibleButton(m_name, ImVec2(static_cast<float>(targetSize.x), static_cast<float>(targetSize.y)), ImGuiButtonFlags_MouseButtonMask_);
	m_hovered = ImGui::IsItemHovered() || ImGui::IsItemActive();
	m_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<ImTextureID>(m_colorTarget.Ptr()), screenPos, screenEnd);

	m_screenRect = IAARectangle(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), static_cast<int>(screenEnd.x), static_cast<int>(screenEnd.y));
	m_drawFrame = ImGui::GetFrameCount();
#endif // HOST_VIEWPORT_ENABLED
}

bool CHostViewport::IsVisible() const
{
#ifdef HOST_VIEWPORT_ENABLED
	return m_colorTarget && ImGui::GetCurrentContext() && m_drawFrame == ImGui::GetFrameCount();
#else
	return false;
#endif // HOST_VIEWPORT_ENABLED
}

bool CHostViewport::IsHovered() const
{
	return IsVisible() && m_hovered;
}

bool CHostViewport::IsFocused() const
{
	return IsVisible() && m_focused;
}

bool CHostViewport::IsMouseOver(const IVector2D& windowPos) const
{
	return IsHovered() && m_screenRect.Contains(windowPos);
}

ITexturePtr CHostViewport::GetColorTarget() const
{
	return IsVisible() ? m_colorTarget : g_matSystem->GetCurrentBackbuffer();
}

ITexturePtr CHostViewport::GetDepthTarget() const
{
	return IsVisible() ? m_depthTarget : g_matSystem->GetDefaultDepthBuffer();
}

IVector2D CHostViewport::GetSize() const
{
	return IsVisible() ? m_screenRect.GetSize() : g_pHost->GetWindowSize();
}

IVector2D CHostViewport::ToViewport(const IVector2D& windowPos) const
{
	return IsVisible() ? windowPos - m_screenRect.leftTop : windowPos;
}

IVector2D CHostViewport::ToWindow(const IVector2D& viewportPos) const
{
	return IsVisible() ? viewportPos + m_screenRect.leftTop : viewportPos;
}

void CHostViewport::SetMouseButtonCaptured(int buttonBits, bool captured)
{
	if (captured)
		m_capturedButtons |= buttonBits;
	else
		m_capturedButtons &= ~buttonBits;
}
