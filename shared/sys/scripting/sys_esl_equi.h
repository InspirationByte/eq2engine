#pragma once

struct FontStyleParam;
class ITextLayoutBuilder;
class IEqFont;
class IEqFontCache;

template<typename CH> class EqTStr;
using EqWString = EqTStr<wchar_t>;

namespace equi
{
class IUIControl;
class Panel;
class Container;
class Image;
class ProgressBar;
class CUIManager;
}

// temporary
EQSCRIPT_BIND_TYPE_NO_PARENT(EqWString, "wstring", BY_VALUE)

EQSCRIPT_BIND_TYPE_NO_PARENT(ITextLayoutBuilder, "ITextLayoutBuilder", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(FontStyleParam, "FontStyleParam", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(IEqFont, "IEqFont", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(IEqFontCache, "IEqFontCache", BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(equi::CUIManager, "equi::CUIManager", BY_REF)

EQSCRIPT_BIND_TYPE_NO_PARENT(equi::IUIControl, "equi::IUIControl", WEAK_REF)
EQSCRIPT_BIND_TYPE_WITH_PARENT(equi::Panel, equi::IUIControl, "equi::Panel")
EQSCRIPT_BIND_TYPE_WITH_PARENT(equi::Container, equi::IUIControl, "equi::Container")
EQSCRIPT_BIND_TYPE_WITH_PARENT(equi::Image, equi::IUIControl, "equi::Image")
EQSCRIPT_BIND_TYPE_WITH_PARENT(equi::ProgressBar, equi::IUIControl, "equi::ProgressBar")

bool eslSysEquiInit(const esl::ScriptState& state);