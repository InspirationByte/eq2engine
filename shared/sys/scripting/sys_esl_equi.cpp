#include "core/core_common.h"
#include "core/ILocalize.h"
#include "utils/KeyValues.h"

#include "font/IFontCache.h"
#include "equi/IEqUI_Control.h"
#include "equi/EqUI_Manager.h"
#include "equi/EqUI_Panel.h"
#include "equi/EqUI_Image.h"
#include "equi/EqUI_ProgressBar.h"

#include "sys_esl.h"
#include "sys_esl_equi.h"

EQSCRIPT_TYPE_BEGIN(EqWString)
EQSCRIPT_TYPE_END

//
// Font system
//

EQSCRIPT_TYPE_BEGIN( ITextLayoutBuilder )
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN( FontStyleParam )
	EQSCRIPT_BIND_VAR( layoutBuilder )

	EQSCRIPT_BIND_VAR( align )
	EQSCRIPT_BIND_VAR( styleFlag )

	EQSCRIPT_BIND_VAR( textColor)

	EQSCRIPT_BIND_VAR( shadowOffset )
	EQSCRIPT_BIND_VAR( shadowWeight )

	EQSCRIPT_BIND_VAR( shadowColor )
	EQSCRIPT_BIND_VAR( shadowAlpha )

	EQSCRIPT_BIND_VAR( scale )
	EQSCRIPT_BIND_VAR( textWeight)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN( IEqFont )
	EQSCRIPT_BIND_FUNC( GetLineHeight )
	EQSCRIPT_BIND_FUNC( GetBaselineOffs )
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN( IEqFontCache )
	EQSCRIPT_BIND_FUNC( GetFont )
EQSCRIPT_TYPE_END

//
// UI panel
//
static const EqWString& S_UIFormatString(const esl::ScriptState& state, int startArg, EqStringRef fmt)
{
	const int n = lua_gettop(state);

	const EqWString fmtTok = LocalizedString(fmt);
	const wchar_t* start = nullptr;

	// FIXME: reduce allocations more?
	static EqWString fmtSpan;
	static EqWString formattedStr;
	static EqWString temp;
	formattedStr.Empty();
	formattedStr.Resize(fmtTok.Length() * 2);

	int luaArgIdx = startArg;
	int trailingPercent = 0;

	// here's a neat trick: split format string between all % tokens
	// and format individual values, then append the string
	for (const wchar_t* cur = fmtTok; ; ++cur)
	{
		// skip escape sequences
		if (*cur == L'%')
			++trailingPercent;
		else
			trailingPercent = 0;

		if (trailingPercent == 2)
		{
			start = cur;
			continue;
		}

		if (*cur == L'%' || *cur == L'\0')
		{
			if (!start)
			{
				trailingPercent = 0;
				formattedStr.Append(fmtTok, cur - fmtTok);
				start = cur;
			}
			else
			{
				const wchar_t* end = cur;

				// make a span of the string and try formatting it
				fmtSpan.Assign(start, end - start);

				if (luaArgIdx <= n) // Ensure we don't exceed Lua arguments
				{
					const int type = lua_type(state, luaArgIdx);

					// Get the Lua argument based on index
					if (type == LUA_TUSERDATA)
					{
						esl::Object<EqWString> wstrObj(state, luaArgIdx);
						EqWString& str = wstrObj.Get();
						formattedStr.AppendFmt(fmtSpan, str);
					}
					else if (type == LUA_TSTRING)
					{
						const char* str = lua_tostring(state, luaArgIdx);
						if (*str == '#')
							temp = LocalizedString(str);
						else
							AnsiUnicodeConverter(temp, str);

						formattedStr.AppendFmt(fmtSpan, temp);
					}
					else if (lua_isinteger(state, luaArgIdx))
					{
						const uint64 num = lua_tointeger(state, luaArgIdx);
						formattedStr.AppendFmt(fmtSpan, num);
					}
					else if (lua_isnumber(state, luaArgIdx))
					{
						const float num = lua_tonumber(state, luaArgIdx);
						formattedStr.AppendFmt(fmtSpan, num);
					}
					else
					{
						// Unsupported argument type, put raw format string
						formattedStr.Append(fmtSpan);
					}
				}
				else
				{
					// No more arguments, append the raw format span
					formattedStr.Append(fmtSpan);
				}

				// goto next
				start = cur;
				++luaArgIdx;
			}
		}

		if (*cur == L'\0')
		{
			// add remaining
			formattedStr.Append(start);
			break;
		}
	}

	return formattedStr;
}

static void S_IUIControl_SetLabelFormat(const esl::ScriptState& state, equi::IUIControl& control, EqStringRef label)
{
	const int n = lua_gettop(state);
	if (n > 2)
	{
		control.SetLabelText(S_UIFormatString(state, 3, label));
		return;
	}
	control.SetLabel(label);
}

static void S_IUIControl_SetLabelText(equi::IUIControl& control, const EqWString& label)
{
	control.SetLabelText(label);
}

static void S_IUIControl_ForEachChild(equi::IUIControl& control, const esl::LuaFunctionRef& func)
{
	using EachChildFunc = esl::runtime::FunctionCall<bool, equi::IUIControl*>;
	control.ForEachChild([&](equi::IUIControl* child) -> bool {
		auto result = EachChildFunc::Invoke(func, child);
		LUA_CHECK_CALL(result, "UIControl ForEachChild");
		return *result;
	});
}

EQSCRIPT_TYPE_BEGIN(equi::IUIControl)
	EQSCRIPT_BIND_FUNC( InitFromKeyValues )

	EQSCRIPT_BIND_VAR_EX_GET_SET(name, GetName, SetName)
	EQSCRIPT_BIND_VAR_EX_GET_SET(label, GetLabel, SetLabel)
	EQSCRIPT_BIND_VAR_EX_GET_SET(visible, IsVisible, SetVisible)
	EQSCRIPT_BIND_VAR_EX_GET_SET(selfVisible, IsSelfVisible, SetSelfVisible)
	EQSCRIPT_BIND_VAR_EX_GET_SET(clipChilds, SetClipsChilds, SetClipsChilds)
	EQSCRIPT_BIND_VAR_EX_GET_SET(clipTransform, HasClipTransform, SetClipTransform)
	EQSCRIPT_BIND_VAR_EX_GET_SET(enabled, IsEnabled, Enable)
	EQSCRIPT_BIND_VAR_EX_GET_SET(size, GetSize, SetSize)
	EQSCRIPT_BIND_VAR_EX_GET_SET(position, GetPosition, SetPosition)
	EQSCRIPT_BIND_VAR_EX_GET_SET(anchors, GetAnchors, SetAnchors)
	EQSCRIPT_BIND_VAR_EX_GET_SET(alignment, GetAlignment, SetAlignment)
	EQSCRIPT_BIND_VAR_EX_GET_SET(scaling, GetScaling, SetScaling)
	EQSCRIPT_BIND_VAR_EX_GET_SET(rectangle, GetRectangle, SetRectangle)
	EQSCRIPT_BIND_VAR_EX_GET_SET(fontScale, GetFontScale, SetFontScale)
	EQSCRIPT_BIND_VAR_EX_GET_SET(textAlignment, GetTextAlignment, SetTextAlignment)

	EQSCRIPT_BIND_FUNC( GetName )
	EQSCRIPT_BIND_FUNC( SetName )

	EQSCRIPT_BIND_FUNC( GetLabel )
	EQSCRIPT_BIND_STATIC_FUNC("SetLabel", S_IUIControl_SetLabelFormat)
	EQSCRIPT_BIND_STATIC_FUNC("SetLabelText", S_IUIControl_SetLabelText)

	EQSCRIPT_BIND_FUNC( Show )
	EQSCRIPT_BIND_FUNC( Hide )

	EQSCRIPT_BIND_FUNC( SetVisible )
	EQSCRIPT_BIND_FUNC( IsVisible )

	EQSCRIPT_BIND_FUNC( SetSelfVisible )
	EQSCRIPT_BIND_FUNC( IsSelfVisible )

	EQSCRIPT_BIND_FUNC( SetClipsChilds )
	EQSCRIPT_BIND_FUNC( IsClipsChilds ) 

	EQSCRIPT_BIND_FUNC( SetClipTransform )
	EQSCRIPT_BIND_FUNC( HasClipTransform )

	EQSCRIPT_BIND_FUNC( Enable )
	EQSCRIPT_BIND_FUNC( IsEnabled )

	EQSCRIPT_BIND_FUNC( SetSize )
	EQSCRIPT_BIND_FUNC( SetPosition )

	EQSCRIPT_BIND_FUNC( GetSize )
	EQSCRIPT_BIND_FUNC( GetPosition )

	EQSCRIPT_BIND_FUNC( SetAnchors )
	EQSCRIPT_BIND_FUNC( GetAnchors ) 

	EQSCRIPT_BIND_FUNC( SetAlignment )
	EQSCRIPT_BIND_FUNC( GetAlignment )

	EQSCRIPT_BIND_FUNC( SetScaling )
	EQSCRIPT_BIND_FUNC( GetScaling )

	EQSCRIPT_BIND_FUNC( SetRectangle )
	EQSCRIPT_BIND_FUNC( GetRectangle )

	EQSCRIPT_BIND_FUNC( GetClientRectangle )

	EQSCRIPT_BIND_FUNC( SetFontScale )
	EQSCRIPT_BIND_FUNC( GetFontScale )

	EQSCRIPT_BIND_FUNC( SetTextColor )
	EQSCRIPT_BIND_FUNC( GetTextColor )

	EQSCRIPT_BIND_FUNC( GetTextAlignment )
	EQSCRIPT_BIND_FUNC( SetTextAlignment )

	EQSCRIPT_BIND_FUNC( SetTransform )

	EQSCRIPT_BIND_FUNC( CalcScaling )

	EQSCRIPT_BIND_FUNC( GetSizeDiff )
	EQSCRIPT_BIND_FUNC( GetSizeDiffPerc )

	// child controls
	EQSCRIPT_BIND_FUNC( AddChild )
	EQSCRIPT_BIND_FUNC( RemoveChild )
	EQSCRIPT_BIND_FUNC( FindChild )
	EQSCRIPT_BIND_FUNC( FindChildRecursive )
	EQSCRIPT_BIND_FUNC( Get )
	EQSCRIPT_BIND_FUNC( ClearChilds )
	EQSCRIPT_BIND_FUNC( ClearAll )
	EQSCRIPT_BIND_STATIC_FUNC("ForEachChild", S_IUIControl_ForEachChild)

	EQSCRIPT_BIND_FUNC( GetParent )

	EQSCRIPT_BIND_FUNC( GetFont )
	EQSCRIPT_BIND_FUNC( GetClassname )

	EQSCRIPT_BIND_FUNC( GetCalcFontStyle )

	EQSCRIPT_BIND_FUNC( HitTest )
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(equi::Panel)

	EQSCRIPT_BIND_VAR_EX_GET_SET(color, GetColor, SetColor)
	EQSCRIPT_BIND_VAR_EX_GET_SET(selectionColor, GetSelectionColor, SetSelectionColor)

	EQSCRIPT_BIND_FUNC( SetColor )
	EQSCRIPT_BIND_FUNC( GetColor )

	EQSCRIPT_BIND_FUNC( SetSelectionColor )
	EQSCRIPT_BIND_FUNC( GetSelectionColor )

	EQSCRIPT_BIND_FUNC( CenterOnScreen )
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(equi::Container)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(equi::Image)
	EQSCRIPT_BIND_VAR_EX_GET_SET(color, GetColor, SetColor)
	EQSCRIPT_BIND_VAR_EX_GET_SET(uvRegion, GetUVRegion, SetUVRegion)

	EQSCRIPT_BIND_FUNC(SetMaterial)
	EQSCRIPT_BIND_FUNC(SetAtlasImage)

	EQSCRIPT_BIND_FUNC(GetMaterialName)
	EQSCRIPT_BIND_FUNC(GetAtlasImageName)

	EQSCRIPT_BIND_FUNC(SetColor)
	EQSCRIPT_BIND_FUNC(GetColor)

	EQSCRIPT_BIND_FUNC(SetUVRegion)
	EQSCRIPT_BIND_FUNC(GetUVRegion)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(equi::ProgressBar)

	EQSCRIPT_BIND_VAR_EX_GET_SET(value, GetValue, SetValue)
	EQSCRIPT_BIND_VAR_EX_GET_SET(color, GetColor, SetColor)

	EQSCRIPT_BIND_FUNC(SetValue)
	EQSCRIPT_BIND_FUNC(GetValue)

	EQSCRIPT_BIND_FUNC(SetColor)
	EQSCRIPT_BIND_FUNC(GetColor)
EQSCRIPT_TYPE_END


//
// UI panel manager
//

EQSCRIPT_TYPE_BEGIN(equi::CUIManager)
	EQSCRIPT_BIND_FUNC( GetRootPanel )

	//EQSCRIPT_BIND_FUNC( RegisterFactory )

	EQSCRIPT_BIND_FUNC( CreateElement )

	EQSCRIPT_BIND_FUNC( AddPanel )
	EQSCRIPT_BIND_FUNC( DestroyPanel )
	EQSCRIPT_BIND_FUNC( FindPanel )

	EQSCRIPT_BIND_FUNC( BringToTop )
	EQSCRIPT_BIND_FUNC( GetTopPanel )

	//EQSCRIPT_BIND_FUNC( SetViewFrame )
	EQSCRIPT_BIND_FUNC( GetViewFrame )
	EQSCRIPT_BIND_FUNC( GetScreenSize )

	EQSCRIPT_BIND_FUNC( SetFocus )
	EQSCRIPT_BIND_FUNC( GetFocus )
	EQSCRIPT_BIND_FUNC( GetMouseOver )

	EQSCRIPT_BIND_FUNC( IsWindowsVisible )

	EQSCRIPT_BIND_FUNC( GetDefaultFont )
EQSCRIPT_TYPE_END

static void S_LoadFontDescriptionFile(const char* fileName)
{
	g_fontCache->LoadFontDescriptionFile(fileName);
}

//---------------------------------------------------------------------------------------
// EqUI
//---------------------------------------------------------------------------------------

bool eslSysEquiInit(const esl::ScriptState& state)
{
	LUA_SET_GLOBAL_CONST(state, TEXT_STYLE_REGULAR);
	LUA_SET_GLOBAL_CONST(state, TEXT_STYLE_BOLD);
	LUA_SET_GLOBAL_CONST(state, TEXT_STYLE_ITALIC);

	LUA_SET_GLOBAL_CONST(state, TEXT_ALIGN_LEFT);
	LUA_SET_GLOBAL_CONST(state, TEXT_ALIGN_RIGHT);
	LUA_SET_GLOBAL_CONST(state, TEXT_ALIGN_HCENTER);

	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_BORDER_LEFT, UI_BORDER_LEFT);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_BORDER_TOP, UI_BORDER_TOP);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_BORDER_RIGHT, UI_BORDER_RIGHT);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_BORDER_BOTTOM, UI_BORDER_BOTTOM);

	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_ALIGN_LEFT, UI_ALIGN_LEFT);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_ALIGN_TOP, UI_ALIGN_TOP);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_ALIGN_RIGHT, UI_ALIGN_RIGHT);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_ALIGN_BOTTOM, UI_ALIGN_BOTTOM);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_ALIGN_HCENTER, UI_ALIGN_HCENTER);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_ALIGN_VCENTER, UI_ALIGN_VCENTER);

	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_SCALING_NONE, UI_SCALING_NONE);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_SCALING_INHERIT, UI_SCALING_INHERIT);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_SCALING_INHERIT_MIN, UI_SCALING_INHERIT_MIN);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_SCALING_INHERIT_MAX, UI_SCALING_INHERIT_MAX);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_SCALING_ASPECT_MIN, UI_SCALING_ASPECT_MIN);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_SCALING_ASPECT_MAX, UI_SCALING_ASPECT_MAX);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_SCALING_ASPECT_W, UI_SCALING_ASPECT_W);
	LUA_SET_GLOBAL_ENUMCONST_3(state, equi::UI_SCALING_ASPECT_H, UI_SCALING_ASPECT_H);

	state.RegisterClass<EqWString>();

	// EqUI
	state.RegisterClass<IEqFont>();
	state.RegisterClass<IEqFontCache>();

	state.RegisterClass<equi::IUIControl>();
	state.RegisterClass<equi::Panel>();
	state.RegisterClass<equi::Container>();
	state.RegisterClass<equi::Image>();
	state.RegisterClass<equi::ProgressBar>();
	state.RegisterClass<equi::CUIManager>();
	state.RegisterClassStatic<equi::CUIManager>("FormatString", EQSCRIPT_CFUNC(+[](const esl::ScriptState& state, EqStringRef fmt) { return S_UIFormatString(state, 2, fmt); }));
	state.RegisterClassStatic<equi::CUIManager>("IsValid", EQSCRIPT_CFUNC(+[](const equi::IUIControl* control) { return control != nullptr; }));

	esl::LuaTable equiCastFuncsTab = state.CreateTable();
	equiCastFuncsTab.Set("label", EQSCRIPT_CFUNC(+[](equi::IUIControl* ctrl) { return ctrl; }));
	equiCastFuncsTab.Set("panel", EQSCRIPT_CFUNC(equi::DynamicCast<equi::Panel>));
	equiCastFuncsTab.Set("container", EQSCRIPT_CFUNC(equi::DynamicCast<equi::Container>));
	equiCastFuncsTab.Set("image", EQSCRIPT_CFUNC(equi::DynamicCast<equi::Image>));
	equiCastFuncsTab.Set("progressBar", EQSCRIPT_CFUNC(equi::DynamicCast<equi::ProgressBar>));

	equi::CUIManager* equiManager = equi::Manager.GetInstancePtr();
	state.SetGlobal("equi_cast", equiCastFuncsTab);
	state.SetGlobal("equi", equiManager);

	// equi font system
	{
		esl::LuaTable fontsTable = state.CreateTable();
		fontsTable.Set("LoadFontDescriptionFile", EQSCRIPT_CFUNC(S_LoadFontDescriptionFile));
		state.SetGlobal("fonts", fontsTable);
	}

	return true;
}