//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: EqUI editor - control property inspector
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "utils/KeyValues.h"

#include "EqUI_Editor.h"
#include "EqUI_EditorProps.h"

#ifdef EQUI_EDITOR
#include <imgui.h>

namespace equi::Editor
{

enum EPropType
{
	PROP_BOOL,
	PROP_INT2,
	PROP_FLOAT,
	PROP_FLOAT2,
	PROP_COLOR,
	PROP_STRING,
	PROP_VALUES,	// raw list of values, e.g. font "Roboto" 20 bold
	PROP_FLAGS,		// any combination of option tokens
	PROP_ENUM,		// one of option tokens
	PROP_FONT,		// local font name or font description values
	PROP_TRANSFORM,	// nested section: rotate, translate, elementScale
};

struct PropDesc
{
	const char*			key;
	EPropType			type;
	const char*			className;		// nullptr - property of every control
	const char* const*	options;
	int					numOptions;
	float				defValue[4];	// shown while key is not set
};

static const char* s_alignOptions[] = { "left", "hcenter", "right", "top", "vcenter", "bottom" };
static const char* s_anchorOptions[] = { "left", "top", "right", "bottom" };
static const char* s_scalingOptions[] = { "none", "inherit", "inherit_min", "inherit_max", "aspectw", "aspecth", "uniform", "aspect_min", "aspect_max" };

#define PROP_OPTIONS(arr)	arr, elementsOf(arr)

// keys parsed by IUIControl::Parse and the control classes, see equi_overview.md
static const PropDesc s_propDescs[] = {
	{ "position",		PROP_INT2,		nullptr, nullptr, 0, { 0, 0 } },
	{ "size",			PROP_INT2,		nullptr, nullptr, 0, { 64, 64 } },
	{ "visible",		PROP_BOOL,		nullptr, nullptr, 0, { 1 } },
	{ "selfVisible",	PROP_BOOL,		nullptr, nullptr, 0, { 1 } },
	{ "label",			PROP_STRING,	nullptr, nullptr, 0, {} },
	{ "align",			PROP_FLAGS,		nullptr, PROP_OPTIONS(s_alignOptions), {} },
	{ "anchors",		PROP_FLAGS,		nullptr, PROP_OPTIONS(s_anchorOptions), {} },
	{ "scaling",		PROP_ENUM,		nullptr, PROP_OPTIONS(s_scalingOptions), {} },
	{ "transform",		PROP_TRANSFORM,	nullptr, nullptr, 0, {} },
	{ "clipChilds",		PROP_BOOL,		nullptr, nullptr, 0, { 1 } },
	{ "clipTransform",	PROP_BOOL,		nullptr, nullptr, 0, { 1 } },
	{ "command",		PROP_VALUES,	nullptr, nullptr, 0, {} },
	{ "font",			PROP_FONT,		nullptr, nullptr, 0, {} },
	{ "fontScale",		PROP_FLOAT2,	nullptr, nullptr, 0, { 1, 1 } },
	{ "textColor",		PROP_COLOR,		nullptr, nullptr, 0, { 1, 1, 1, 1 } },
	{ "textAlign",		PROP_FLAGS,		nullptr, PROP_OPTIONS(s_alignOptions), {} },

	{ "color",			PROP_COLOR,		"panel", nullptr, 0, { 1, 1, 1, 1 } },
	{ "window",			PROP_BOOL,		"panel", nullptr, 0, { 0 } },
	{ "screenoverlay",	PROP_BOOL,		"panel", nullptr, 0, { 0 } },

	{ "file",			PROP_STRING,	"container", nullptr, 0, {} },

	{ "path",			PROP_STRING,	"image", nullptr, 0, {} },
	{ "atlas",			PROP_VALUES,	"image", nullptr, 0, {} },
	{ "uvLeftTop",		PROP_FLOAT2,	"image", nullptr, 0, { 0, 0 } },
	{ "uvRightBottom",	PROP_FLOAT2,	"image", nullptr, 0, { 1, 1 } },
	{ "color",			PROP_COLOR,		"image", nullptr, 0, { 1, 1, 1, 1 } },
	{ "flipX",			PROP_BOOL,		"image", nullptr, 0, { 0 } },
	{ "flipY",			PROP_BOOL,		"image", nullptr, 0, { 0 } },

	{ "image",			PROP_STRING,	"mask", nullptr, 0, {} },
	{ "color",			PROP_COLOR,		"mask", nullptr, 0, { 1, 1, 1, 1 } },

	{ "value",			PROP_FLOAT,		"progressBar", nullptr, 0, { 0 } },
	{ "color",			PROP_COLOR,		"progressBar", nullptr, 0, { 1, 1, 1, 1 } },
};

static constexpr const ImU32 s_unsetKeyColor = IM_COL32(128, 128, 128, 255);

//-------------------------------------------------------------------------------------------------

static bool IsPropOfClass(const PropDesc& desc, const char* className)
{
	return !desc.className || !CString::CompareCaseIns(desc.className, className);
}

static bool IsKnownKey(const char* key, const char* className)
{
	for (const PropDesc& desc : s_propDescs)
	{
		if (IsPropOfClass(desc, className) && !CString::CompareCaseIns(desc.key, key))
			return true;
	}
	return false;
}

// creates the key or removes its values
static KVSection& ResetKey(KVSection& sec, KVSection*& key, const char* name)
{
	if (!key)
		key = &sec.CreateSection(name);
	key->ClearValues();
	return *key;
}

static bool NeedsQuotes(const char* str)
{
	if (!*str)
		return true;

	for (const char* c = str; *c; ++c)
	{
		if (CType::IsSpace(*c) || strchr("{};\"", *c))
			return true;
	}
	return false;
}

static EqString ValuesToText(const KVSection* key)
{
	EqString text;
	if (!key)
		return text;

	for (int i = 0; i < key->ValueCount(); ++i)
	{
		const char* value = key->ValueAt(i).GetString();
		if (i > 0)
			text.Append(' ');

		if (NeedsQuotes(value))
			text.Append(EqString::Format("\"%s\"", value));
		else
			text.Append(value);
	}
	return text;
}

// values text is parsed with the KeyValues parser, so quoting rules are the same as in .res files
static bool TextToValues(KVSection& key, const char* text)
{
	const EqString keyText = EqString::Format("v %s;", text);

	KVSection parsed;
	if (!KeyValues::ParseText(keyText, keyText.Length(), parsed))
		return false;

	const KVSection* parsedKey = parsed.FindSection("v");
	if (!parsedKey)
		return false;

	key.ClearValues();
	for (int i = 0; i < parsedKey->ValueCount(); ++i)
		key.AddValue(parsedKey->ValueAt(i));

	return true;
}

static bool HasOption(const KVSection* key, const char* option)
{
	if (!key)
		return false;

	for (EqStringRef value : key->Values<EqStringRef>())
	{
		if (!value.CompareCaseIns(option))
			return true;

		// shortcuts accepted by the parser
		if (!value.CompareCaseIns("center") && (!CString::CompareCaseIns(option, "hcenter") || !CString::CompareCaseIns(option, "vcenter")))
			return true;
		if (!value.CompareCaseIns("all"))
			return true;
	}
	return false;
}

static void SetFloatValues(KVSection& sec, const char* name, const float* values, int count)
{
	KVSection* key = sec.FindSection(name);
	KVSection& newKey = ResetKey(sec, key, name);
	for (int i = 0; i < count; ++i)
		newKey.AddValue(values[i]);
}

// replaces values of the key with parsed text, the key is created if it is valid
static bool ApplyValuesText(KVSection& sec, KVSection*& key, const char* name, const char* text)
{
	KVSection* newKey = key ? key : &sec.CreateSection(name);
	if (!TextToValues(*newKey, text))
	{
		MsgError("EqUI editor: can't parse values of '%s'\n", name);
		if (!key)
			sec.RemoveSection(newKey);
		return false;
	}

	key = newKey;
	return true;
}

//-------------------------------------------------------------------------------------------------

static bool DrawTextInput(const char* id, const EqString& value, EqString& outValue)
{
	char buffer[512];
	strncpy(buffer, value.ToCString(), sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = 0;

	ImGui::SetNextItemWidth(-FLT_MIN);
	if (!ImGui::InputText(id, buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Press Enter to apply");
		return false;
	}

	outValue = buffer;
	return true;
}

// edits value of the key, creates it on change
static bool DrawPropertyValue(KVSection& sec, KVSection*& key, const PropDesc& desc, const PropertiesContext& ctx)
{
	ImGui::SetNextItemWidth(-FLT_MIN);

	switch (desc.type)
	{
		case PROP_BOOL:
		{
			bool value = KV_GetValueBool(key, 0, desc.defValue[0] > 0.0f);
			if (!ImGui::Checkbox("##value", &value))
				return false;

			ResetKey(sec, key, desc.key).AddValue(value ? 1 : 0);
			return true;
		}
		case PROP_INT2:
		{
			IVector2D value(desc.defValue[0], desc.defValue[1]);
			if (key)
				key->GetValues(value);

			if (!ImGui::DragInt2("##value", &value.x))
				return false;

			KVSection& newKey = ResetKey(sec, key, desc.key);
			newKey.AddValue(value.x);
			newKey.AddValue(value.y);
			return true;
		}
		case PROP_FLOAT:
		{
			float value = KV_GetValueFloat(key, 0, desc.defValue[0]);
			if (!ImGui::DragFloat("##value", &value, 0.01f))
				return false;

			ResetKey(sec, key, desc.key).AddValue(value);
			return true;
		}
		case PROP_FLOAT2:
		{
			Vector2D value = KV_GetVector2D(key, 0, Vector2D(desc.defValue[0], desc.defValue[1]));
			if (!ImGui::DragFloat2("##value", &value.x, 0.01f))
				return false;

			KVSection& newKey = ResetKey(sec, key, desc.key);
			newKey.AddValue(value.x);
			newKey.AddValue(value.y);
			return true;
		}
		case PROP_COLOR:
		{
			Vector4D value = KV_GetVector4D(key, 0, Vector4D(desc.defValue[0], desc.defValue[1], desc.defValue[2], desc.defValue[3]));
			if (!ImGui::ColorEdit4("##value", &value.x, ImGuiColorEditFlags_Float))
				return false;

			KVSection& newKey = ResetKey(sec, key, desc.key);
			for (int i = 0; i < 4; ++i)
				newKey.AddValue(value[i]);
			return true;
		}
		case PROP_STRING:
		{
			EqString value;
			if (!DrawTextInput("##value", KV_GetValueString(key, 0, ""), value))
				return false;

			ResetKey(sec, key, desc.key).AddValue(value.ToCString());
			return true;
		}
		case PROP_VALUES:
		{
			EqString value;
			if (!DrawTextInput("##value", ValuesToText(key), value))
				return false;

			return ApplyValuesText(sec, key, desc.key, value);
		}
		case PROP_FONT:
		{
			// single value is a name from 'fonts', several values describe the font itself
			bool changed = false;
			const EqString current = ValuesToText(key);

			if (ImGui::BeginCombo("##localFont", current.Length() ? current.ToCString() : "(not set)"))
			{
				if (ctx.localFonts.numElem() == 0)
					ImGui::TextDisabled("No 'fonts' in the layout");

				for (const EqString& fontName : ctx.localFonts)
				{
					const bool selected = key && key->ValueCount() == 1 && !fontName.CompareCaseIns(key->ValueAt(0).GetString());
					if (ImGui::Selectable(fontName, selected) && !selected)
					{
						ResetKey(sec, key, desc.key).AddValue(fontName.ToCString());
						changed = true;
					}
				}
				ImGui::EndCombo();
			}

			EqString value;
			if (DrawTextInput("##value", current, value))
				changed = ApplyValuesText(sec, key, desc.key, value) || changed;

			return changed;
		}
		case PROP_TRANSFORM:
		{
			// same keys and defaults as in IUIControl::Parse
			float rotate = KV_GetValueFloat(key ? key->FindSection("rotate") : nullptr, 0, 0.0f);
			Vector2D translate = KV_GetVector2D(key ? key->FindSection("translate") : nullptr, 0, vec2_zero);
			Vector2D elementScale = KV_GetVector2D(key ? key->FindSection("elementScale") : nullptr, 0, Vector2D(1.0f));

			// short labels to fit narrow columns and menus, key names are in the tooltips
			const auto beginRow = [&](const char* label, const char* tooltip) {
				ImGui::SetNextItemWidth(40);
				ImGui::TextUnformatted(label);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", tooltip);
				ImGui::SameLine(45);
				ImGui::SetNextItemWidth(-FLT_MIN);
			};

			beginRow("T", "translate");
			const bool translateChanged = ImGui::DragFloat2("##translate", &translate.x, 0.5f);

			beginRow("R", "rotate, degrees");
			const bool rotateChanged = ImGui::DragFloat("##rotate", &rotate, 0.5f, 0.0f, 0.0f, "%.1f");

			beginRow("S", "elementScale");
			const bool scaleChanged = ImGui::DragFloat2("##elementScale", &elementScale.x, 0.01f);

			if (!rotateChanged && !translateChanged && !scaleChanged)
				return false;

			// nested keys are kept, only the changed one is written
			if (!key)
				key = &sec.CreateSection(desc.key);

			if (rotateChanged)
				SetFloatValues(*key, "rotate", &rotate, 1);
			if (translateChanged)
				SetFloatValues(*key, "translate", &translate.x, 2);
			if (scaleChanged)
				SetFloatValues(*key, "elementScale", &elementScale.x, 2);

			return true;
		}
		case PROP_FLAGS:
		{
			const ImGuiStyle& style = ImGui::GetStyle();
			const float maxX = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;

			bool changed = false;
			uint optionBits = 0;
			for (int i = 0; i < desc.numOptions; ++i)
			{
				bool hasOption = HasOption(key, desc.options[i]);

				// options that don't fit go to the next line
				const float optionWidth = ImGui::GetFrameHeight() + style.ItemInnerSpacing.x + ImGui::CalcTextSize(desc.options[i]).x;
				if (i > 0 && ImGui::GetItemRectMax().x + style.ItemSpacing.x + optionWidth <= maxX)
					ImGui::SameLine();

				ImGui::PushID(i);
				changed = ImGui::Checkbox(desc.options[i], &hasOption) || changed;
				ImGui::PopID();

				if (hasOption)
					optionBits |= (1 << i);
			}

			if (!changed)
				return false;

			KVSection& newKey = ResetKey(sec, key, desc.key);
			for (int i = 0; i < desc.numOptions; ++i)
			{
				if (optionBits & (1 << i))
					newKey.AddValue(desc.options[i]);
			}
			return true;
		}
		case PROP_ENUM:
		{
			const char* current = KV_GetValueString(key, 0, desc.options[0]);
			if (!ImGui::BeginCombo("##value", current))
				return false;

			bool changed = false;
			for (int i = 0; i < desc.numOptions; ++i)
			{
				const bool selected = !CString::CompareCaseIns(current, desc.options[i]);
				if (ImGui::Selectable(desc.options[i], selected) && !selected)
				{
					ResetKey(sec, key, desc.key).AddValue(desc.options[i]);
					changed = true;
				}
			}
			ImGui::EndCombo();
			return changed;
		}
	}
	return false;
}

static void SetDefaultValue(KVSection& key, const PropDesc& desc, const PropertiesContext& ctx)
{
	switch (desc.type)
	{
		case PROP_BOOL:
			key.AddValue(desc.defValue[0] > 0.0f ? 1 : 0);
			break;
		case PROP_INT2:
			key.AddValue(static_cast<int>(desc.defValue[0]));
			key.AddValue(static_cast<int>(desc.defValue[1]));
			break;
		case PROP_FLOAT:
			key.AddValue(desc.defValue[0]);
			break;
		case PROP_FLOAT2:
			key.AddValue(desc.defValue[0]);
			key.AddValue(desc.defValue[1]);
			break;
		case PROP_COLOR:
			for (int i = 0; i < 4; ++i)
				key.AddValue(desc.defValue[i]);
			break;
		case PROP_STRING:
			key.AddValue("");
			break;
		case PROP_ENUM:
			key.AddValue(desc.options[0]);
			break;
		case PROP_FONT:
			key.AddValue(ctx.localFonts.numElem() ? ctx.localFonts[0].ToCString() : "default");
			break;
		case PROP_TRANSFORM:
		{
			const float rotate = 0.0f;
			const float translate[] = { 0.0f, 0.0f };
			const float elementScale[] = { 1.0f, 1.0f };
			SetFloatValues(key, "rotate", &rotate, 1);
			SetFloatValues(key, "translate", translate, 2);
			SetFloatValues(key, "elementScale", elementScale, 2);
			break;
		}
		default:
			break;
	}
}

// checkbox in the first column tells if the key is written to the layout
static bool DrawProperty(KVSection& sec, const PropDesc& desc, const PropertiesContext& ctx)
{
	KVSection* key = sec.FindSection(desc.key);
	bool changed = false;

	ImGui::PushID(desc.key);
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	bool isSet = key != nullptr;
	if (ImGui::Checkbox("##set", &isSet))
	{
		if (isSet)
		{
			key = &sec.CreateSection(desc.key);
			SetDefaultValue(*key, desc, ctx);
		}
		else
		{
			sec.RemoveSection(key);
			key = nullptr;
		}
		changed = true;
	}

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(isSet ? "Remove key from the layout" : "Write key to the layout");

	ImGui::SameLine();
	if (!key)
		ImGui::PushStyleColor(ImGuiCol_Text, s_unsetKeyColor);
	ImGui::TextUnformatted(desc.key);
	if (!key)
		ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	changed = DrawPropertyValue(sec, key, desc, ctx) || changed;

	ImGui::PopID();
	return changed;
}

// keys that the inspector does not know are edited as raw values
static bool DrawOtherKeys(KVSection& sec, const char* className)
{
	bool changed = false;
	int removeKeyIdx = -1;

	for (int i = 0; i < sec.KeyCount(); ++i)
	{
		KVSection& key = sec.KeyAt(i);
		if (!CString::CompareCaseIns(key.GetName(), "child") || !CString::CompareCaseIns(key.GetName(), EDITOR_ONLY_KEY))
			continue;

		if (IsKnownKey(key.GetName(), className))
			continue;

		ImGui::PushID(i);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		if (ImGui::SmallButton("x"))
			removeKeyIdx = i;

		ImGui::SameLine();
		ImGui::TextUnformatted(key.GetName());
		ImGui::TableNextColumn();

		if (key.KeyCount() > 0)
		{
			// nested sections like transform and fonts
			ImGui::TextDisabled("{ %d keys } - edit in KeyValues tab", key.KeyCount());
		}
		else
		{
			EqString value;
			if (DrawTextInput("##value", ValuesToText(&key), value))
			{
				if (TextToValues(key, value))
					changed = true;
				else
					MsgError("EqUI editor: can't parse values of '%s'\n", key.GetName());
			}
		}
		ImGui::PopID();
	}

	if (removeKeyIdx != -1)
	{
		sec.RemoveSection(&sec.KeyAt(removeKeyIdx));
		changed = true;
	}

	return changed;
}

// adds keys written as in .res file: key value1 value2
static bool DrawAddKey(KVSection& sec)
{
	EqString text;
	ImGui::TextUnformatted("Add key");
	if (!DrawTextInput("##addKey", EqString(), text) || text.Length() == 0)
		return false;

	const EqString keysText = EqString::Format("%s;", text.ToCString());

	KVSection parsed;
	if (!KeyValues::ParseText(keysText, keysText.Length(), parsed))
	{
		MsgError("EqUI editor: can't parse key '%s'\n", text.ToCString());
		return false;
	}

	for (const KVSection* parsedKey : parsed.keys)
	{
		KVSection* key = sec.FindSection(parsedKey->GetName());
		if (!key)
			key = &sec.CreateSection(parsedKey->GetName());
		parsedKey->CopyTo(*key);
	}
	return parsed.KeyCount() > 0;
}

static const PropDesc& GetCommonPropDesc(const char* key)
{
	for (const PropDesc& desc : s_propDescs)
	{
		if (!desc.className && !CString::CompareCaseIns(desc.key, key))
			return desc;
	}

	ASSERT_FAIL("unknown property %s", key);
	return s_propDescs[0];
}

// 3x3 grid, missing key is left top like in IUIControl
static bool DrawAlignmentGrid(KVSection& sec)
{
	static const char* s_hAlign[] = { "left", "hcenter", "right" };
	static const char* s_vAlign[] = { "top", "vcenter", "bottom" };

	KVSection* key = sec.FindSection("align");

	int curH = 0;
	int curV = 0;
	if (key)
	{
		curH = HasOption(key, "hcenter") ? 1 : (HasOption(key, "right") ? 2 : 0);
		curV = HasOption(key, "vcenter") ? 1 : (HasOption(key, "bottom") ? 2 : 0);
	}

	bool changed = false;
	for (int v = 0; v < 3; ++v)
	{
		for (int h = 0; h < 3; ++h)
		{
			if (h > 0)
				ImGui::SameLine();

			ImGui::PushID(v * 3 + h);
			if (ImGui::RadioButton("##align", h == curH && v == curV) && !(h == curH && v == curV))
			{
				KVSection& newKey = ResetKey(sec, key, "align");
				newKey.AddValue(s_hAlign[h]);
				newKey.AddValue(s_vAlign[v]);
				changed = true;
			}

			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s %s", s_hAlign[h], s_vAlign[v]);
			ImGui::PopID();
		}
	}
	return changed;
}

// checkboxes placed on the sides of a square
static bool DrawAnchorsCross(KVSection& sec)
{
	KVSection* key = sec.FindSection("anchors");

	bool sides[4];
	for (int i = 0; i < 4; ++i)
		sides[i] = HasOption(key, s_anchorOptions[i]);

	const float boxSize = ImGui::GetFrameHeight();
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const float startX = ImGui::GetCursorPosX();

	// left, top, right, bottom
	bool changed = false;
	const auto drawSide = [&](int side, float offsetX) {
		ImGui::SetCursorPosX(startX + offsetX);
		ImGui::PushID(side);
		changed = ImGui::Checkbox("##anchor", &sides[side]) || changed;
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", s_anchorOptions[side]);
		ImGui::PopID();
	};

	drawSide(1, boxSize + spacing);
	drawSide(0, 0.0f);
	ImGui::SameLine();
	drawSide(2, (boxSize + spacing) * 2.0f);
	drawSide(3, boxSize + spacing);

	if (!changed)
		return false;

	KVSection& newKey = ResetKey(sec, key, "anchors");
	for (int i = 0; i < 4; ++i)
	{
		if (sides[i])
			newKey.AddValue(s_anchorOptions[i]);
	}
	return true;
}

//-------------------------------------------------------------------------------------------------

bool IsEditorOnly(const KVSection& sec)
{
	return KV_GetValueBool(sec.FindSection(EDITOR_ONLY_KEY), 0, false);
}

bool DrawControlProperties(KVSection& sec, const char* className, const PropertiesContext& ctx)
{
	bool changed = false;

	constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
	if (ImGui::BeginTable("##properties", 2, tableFlags))
	{
		ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 130.0f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

		for (const PropDesc& desc : s_propDescs)
		{
			if (IsPropOfClass(desc, className))
				changed = DrawProperty(sec, desc, ctx) || changed;
		}

		changed = DrawOtherKeys(sec, className) || changed;
		ImGui::EndTable();
	}

	ImGui::Separator();
	changed = DrawAddKey(sec) || changed;

	return changed;
}

bool DrawLayoutProperties(KVSection& sec)
{
	static const Array<EqString> s_noFonts{ PP_SL };
	const PropertiesContext ctx{ s_noFonts };

	bool changed = false;

	constexpr ImGuiTableFlags pickerFlags = ImGuiTableFlags_SizingFixedFit;
	if (ImGui::BeginTable("##layoutPickers", 2, pickerFlags))
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Align");
		changed = DrawAlignmentGrid(sec) || changed;

		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Anchors");
		changed = DrawAnchorsCross(sec) || changed;

		ImGui::EndTable();
	}

	// popups and menus are sized by their content, values take the remaining width
	constexpr float labelWidth = 70.0f;
	constexpr float valueWidth = 180.0f;
	for (const char* keyName : { "position", "size", "scaling", "transform" })
	{
		const PropDesc& desc = GetCommonPropDesc(keyName);
		KVSection* key = sec.FindSection(desc.key);

		ImGui::PushID(keyName);
		ImGui::TextUnformatted(keyName);
		ImGui::SameLine(labelWidth);
		ImGui::BeginGroup();
		ImGui::Dummy(ImVec2(valueWidth, 0.0f));
		changed = DrawPropertyValue(sec, key, desc, ctx) || changed;
		ImGui::EndGroup();
		ImGui::PopID();
	}

	return changed;
}

}

#endif // EQUI_EDITOR
