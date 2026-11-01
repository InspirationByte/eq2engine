//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: EqUI editor - control property inspector
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct KVSection;

namespace equi::Editor
{
// key of the editor-only child sections, they are shown in the preview and removed on save
static constexpr const char* EDITOR_ONLY_KEY = "editorOnly";

struct PropertiesContext
{
	const Array<EqString>&	localFonts;		// names from 'fonts' sections of the layout available to the control
	EqStringRef				filter;
};

bool	IsEditorOnly(const KVSection& sec);

// draws property editors of the control layout section, returns true when the section is changed
bool	DrawControlProperties(KVSection& sec, const char* className, const PropertiesContext& ctx);

// alignment and anchors as visual pickers, position, size and scaling; returns true when the section is changed
bool	DrawLayoutProperties(KVSection& sec);
}
