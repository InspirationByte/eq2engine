// dear imgui: Renderer Backend for Equilibrium Engine

#include "core/core_common.h"
#include "imgui_impl_matsystem.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"
#include "imaging/ImageLoader.h"

// DirectX data
struct ImGui_ImplMatSystem_Data
{
	ITexture*					FontTexture;

	ImGui_ImplMatSystem_Data() { memset(this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplMatSystem_Data* ImGui_ImplMatSystem_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_ImplMatSystem_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}


// Functions
static void ImGui_ImplMatSystem_SetupRenderState(ImDrawData* draw_data)
{
	ImGui_ImplMatSystem_Data* bd = ImGui_ImplMatSystem_GetBackendData();

	//materials->Setup2D(draw_data->DisplaySize.x, draw_data->DisplaySize.y);

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	materials->SetAmbientColor(color_white);
	materials->SetDepthStates(false, false);
	materials->SetBlendingStates(blending);
	materials->SetRasterizerStates(CULL_NONE, FILL_SOLID, true, true);
}

// Render function.
void ImGui_ImplMatSystem_RenderDrawData(ImDrawData* draw_data)
{
	// Avoid rendering when minimized
	if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
		return;

	// Setup desired DX state
	ImGui_ImplMatSystem_SetupRenderState(draw_data);

	IDynamicMesh* pMatsysMesh = materials->GetDynamicMesh();

	CMeshBuilder mb(pMatsysMesh);
	float halfPixelOfs = 0.0f;
	if (g_pShaderAPI->GetShaderAPIClass() == SHADERAPI_DIRECT3D9)
		halfPixelOfs = 0.5f;

	// Copy vertices to matsystem mesh
	ImVec2 clip_off = draw_data->DisplayPos;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];

		g_pShaderAPI->SetTexture(nullptr, nullptr, 0);
		materials->BindMaterial(materials->GetDefaultMaterial());

		mb.Begin(PRIM_TRIANGLES);

		for (int i = 0; i < cmd_list->VtxBuffer.Size; i++)
		{
			const ImDrawVert& vtx_src = cmd_list->VtxBuffer.Data[i];
			MColor color(vtx_src.col);

			mb.Position2f(vtx_src.pos.x + halfPixelOfs, vtx_src.pos.y + halfPixelOfs);
			mb.TexCoord2f(vtx_src.uv.x, vtx_src.uv.y);
			mb.Color4fv(color);
			mb.AdvanceVertex();
		}

		ImDrawIdx* idx_dst;
		void* vtx_dst;
		if (pMatsysMesh->AllocateGeom(0, cmd_list->IdxBuffer.Size, &vtx_dst, &idx_dst, false) != -1)
		{
			memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		}

		mb.End(false);

		// render command buffers now
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

			if (pcmd->UserCallback != nullptr)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					ImGui_ImplMatSystem_SetupRenderState(draw_data);
				else
					pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
				ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
				if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
					continue;

				// Apply Scissor/clipping rectangle, Bind texture, Draw
				ITexture* texture = (ITexture*)pcmd->GetTexID();
				IRectangle scissor((int)clip_min.x, (int)clip_min.y, (int)clip_max.x, (int)clip_max.y);
				g_pShaderAPI->SetScissorRectangle(scissor);
				g_pShaderAPI->SetTexture(texture, nullptr, 0);

				materials->BindMaterial(materials->GetDefaultMaterial());
				g_pShaderAPI->Reset(STATE_RESET_VBO);
				pMatsysMesh->Render(pcmd->IdxOffset, pcmd->ElemCount);
			}
		}
	}
}

bool ImGui_ImplMatSystem_Init()
{
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

	// Setup backend capabilities flags
	ImGui_ImplMatSystem_Data* bd = IM_NEW(ImGui_ImplMatSystem_Data)();
	io.BackendRendererUserData = (void*)bd;
	io.BackendRendererName = "imgui_impl_matsystem";
	//io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

	return true;
}

void ImGui_ImplMatSystem_Shutdown()
{
	ImGui_ImplMatSystem_Data* bd = ImGui_ImplMatSystem_GetBackendData();
	IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
	ImGuiIO& io = ImGui::GetIO();

	ImGui_ImplMatSystem_InvalidateDeviceObjects();
	io.BackendRendererName = nullptr;
	io.BackendRendererUserData = nullptr;
	IM_DELETE(bd);
}

static bool ImGui_ImplMatSystem_CreateFontsTexture()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplMatSystem_Data* bd = ImGui_ImplMatSystem_GetBackendData();
	unsigned char* pixels;
	int width, height, bytes_per_pixel;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

	CImage image;
	image.SetName("_imfont");
	ubyte* pImage = image.Create(FORMAT_RGBA8, width, height, 1, 1);
	memcpy(pImage, pixels, GetBytesPerPixel(FORMAT_RGBA8) * width * height);

	FixedArray<CImage*, 1> imgs(PP_SL);
	imgs.append(&image);
	
	SamplerStateParam_t params = g_pShaderAPI->MakeSamplerState(TEXFILTER_NEAREST, TEXADDRESS_CLAMP, TEXADDRESS_CLAMP, TEXADDRESS_CLAMP);
	bd->FontTexture = g_pShaderAPI->CreateTexture(imgs, params);

	ASSERT(bd->FontTexture);
	if (!bd->FontTexture)
		return false;

	// Store our identifier
	io.Fonts->SetTexID((ImTextureID)bd->FontTexture);

	return true;
}

bool ImGui_ImplMatSystem_CreateDeviceObjects()
{
	ImGui_ImplMatSystem_Data* bd = ImGui_ImplMatSystem_GetBackendData();
	if (!bd)
		return false;
	if (!ImGui_ImplMatSystem_CreateFontsTexture())
		return false;
	return true;
}

void ImGui_ImplMatSystem_InvalidateDeviceObjects()
{
	ImGui_ImplMatSystem_Data* bd = ImGui_ImplMatSystem_GetBackendData();
	if (!bd)
		return;
	if (bd->FontTexture) { g_pShaderAPI->FreeTexture(bd->FontTexture); bd->FontTexture = nullptr; ImGui::GetIO().Fonts->SetTexID(nullptr); } // We copied bd->pFontTextureView to io.Fonts->TexID so let's clear that as well.
}

void ImGui_ImplMatSystem_NewFrame()
{
	ImGui_ImplMatSystem_Data* bd = ImGui_ImplMatSystem_GetBackendData();
	IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplMatSystem_Init()?");

	if (!bd->FontTexture)
		ImGui_ImplMatSystem_CreateDeviceObjects();
}
