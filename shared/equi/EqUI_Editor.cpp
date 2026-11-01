//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: EqUI editor
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "utils/KeyValues.h"
#include "materialsystem1/IMaterialSystem.h"

#include "EqUI_Manager.h"
#include "EqUI_Panel.h"
#include "EqUI_EditorProps.h"
#include "EqUI_Editor.h"

#ifdef EQUI_EDITOR
#include <imgui.h>

namespace equi::Editor
{

static constexpr const char* s_resourcesDir = "resources";
static constexpr const int s_maxIncludeDepth = 16;
static constexpr const int s_minZoomPercent = 10;
static constexpr const int s_maxZoomPercent = 400;

struct PreviewSize
{
	const char*	name;
	IVector2D	size;
};

static const PreviewSize s_previewSizes[] = {
	{ "Fit to window",	IVector2D(0, 0) },
	{ "640x480",		IVector2D(640, 480) },
	{ "800x600",		IVector2D(800, 600) },
	{ "1280x720",		IVector2D(1280, 720) },
	{ "1920x1080",		IVector2D(1920, 1080) },
};

static constexpr const ImU32 s_selectionColor = IM_COL32(255, 160, 0, 255);
static constexpr const ImU32 s_hoverColor = IM_COL32(255, 255, 255, 160);
static constexpr const ImU32 s_boundsColor = IM_COL32(0, 200, 255, 90);
static const ImVec4 s_editorOnlyTextColor(1.0f, 0.6f, 0.2f, 1.0f);
static const ImVec4 s_includeTextColor(0.55f, 0.75f, 1.0f, 1.0f);

//-------------------------------------------------------------------------------------------------

// layout file: the edited one or one included with 'child file'
struct LayoutDoc : RefCountedObject<LayoutDoc>
{
	EqString	filePath;
	KVSection	root;
	bool		modified{ false };
	bool		valid{ true };		// include file that failed to load is kept to not load it again
};
using LayoutDocPtr = CRefPtr<LayoutDoc>;

// in outliner order, it is reversed order of the sections (IUIControl::AddChild prepends children)
enum EMoveChild
{
	MOVE_CHILD_UP,
	MOVE_CHILD_DOWN,
	MOVE_CHILD_TO_BEGINNING,
	MOVE_CHILD_TO_END,
};

// section of a control, its parent section and the file it is written in
struct SectionRef
{
	KVSection*	sec{ nullptr };
	KVSection*	parent{ nullptr };
	LayoutDoc*	doc{ nullptr };			// not owned, documents are kept until a layout is opened
	bool		editorOnlyInclude{ false };	// control or its parent comes from an editor-only 'child file'
};

// Layout is edited as KeyValues documents, preview controls are recreated from them after every change.
// Controls are matched with their sections by the path of child names from the panel.
class CUIEditor
{
public:
	void			Draw(bool& open);
	bool			LoadDocument(const char* filePath);
	void			Shutdown();

private:
	void			DestroyPreviewPanel();
	void			NewDocument();
	void			ResetDocumentState();
	bool			SaveDocument(LayoutDoc& doc, const char* filePath);
	void			SaveIncludes();
	bool			IsModified() const;
	void			OnDocumentChanged(LayoutDoc* doc);
	void			RebuildPreview();
	void			UpdateKVText();

	void			ClearIncludes();
	bool			ReloadIncludeDoc(LayoutDoc& doc);
	LayoutDoc*		GetIncludeDoc(const char* filePath);
	LayoutDoc*		GetKVDoc();

	KVSection&		GetPanelSection();
	void			FindChildSection(KVSection& sec, LayoutDoc* doc, const char* name, SectionRef& result, int depth, bool editorOnlyInclude);
	SectionRef		FindSectionByPath(const char* path);
	EqString		GetControlPath(const IUIControl* control) const;
	IUIControl*		GetSelectedControl() const;
	void			Select(const char* path);
	bool			IsEditorHidden(const char* path) const;
	void			SetEditorHidden(const char* path, bool hidden);

	void			AddChildSection(const SectionRef& parentRef, const EqString& parentPath, const char* className, bool editorOnly);
	void			RemoveChildSection(const EqString& path);
	void			MoveChildSection(const EqString& path, EMoveChild move);
	void			CollectLocalFonts(const EqString& path, Array<EqString>& outNames);
	void			CollectSectionFonts(const SectionRef& ref, Array<EqString>& outNames);

	void			DrawMenuBar();
	void			DrawOpenPopup();
	void			DrawSaveAsPopup();
	void			DrawAddFilePopup();
	bool			DrawResFileList(const char* selectedPath, EqString& outActivated);
	void			RequestResFileList();
	void			AddIncludeSection(const EqString& path, const char* filePath, bool editorOnly);
	void			DrawOutliner();
	void			DrawOutlinerNode(IUIControl* control);
	void			DrawAddChildMenu(const SectionRef& ref, const EqString& path, bool editorOnly);
	void			DrawPreviewTab();
	void			DrawLayoutPopup();
	void			MoveSelectedControl(const IVector2D& move);
	void			DrawPreviewOverlay(const ImVec2& origin, float zoom, IUIControl* hovered);
	void			DrawPreviewRulers(const ImVec2& origin, const IVector2D& size, float zoom, const ImVec2& rulerSize);
	void			RenderPreview(const IVector2D& size);
	void			DrawKeyValuesTab();
	void			DrawInspector();
	void			DrawIncludes(const SectionRef& ref);

	LayoutDocPtr	m_mainDoc;
	Array<LayoutDocPtr>	m_includeDocs{ PP_SL };

	// preview controls are not added to the manager, they are rendered to own target
	Panel*			m_previewRoot{ nullptr };
	Panel*			m_panel{ nullptr };
	ITexturePtr		m_previewTarget;
	int				m_previewSizeIdx{ 0 };
	int				m_zoomPercent{ 100 };	// fixed resolutions only
	ColorRGBA		m_previewClearColor{ 0.2f, 0.2f, 0.2f, 1.0f };
	bool			m_rebuildPending{ true };
	bool			m_showBounds{ false };
	bool			m_stretchPanel{ true };
	bool			m_rootElement{ false };	// layout is rendered without a parent, like the game HUD

	// empty path is the panel itself
	EqString		m_selection;
	bool			m_hasSelection{ false };

	// control of the layout popup opened in the preview
	EqString		m_layoutPopupPath;

	// outliner item under the mouse in the current frame, outliner is drawn before the preview
	EqString		m_outlinerHoverPath;
	bool			m_hasOutlinerHover{ false };

	// controls hidden in the preview only, not written to the document
	Array<EqString>	m_editorHiddenPaths{ PP_SL };

	// KeyValues tab, -1 is the main document, otherwise include index
	Array<char>		m_kvText{ PP_SL };
	int				m_kvDocIdx{ -1 };
	bool			m_kvTextDirty{ true };
	bool			m_kvTextEdited{ false };
	bool			m_selectKVTab{ false };

	// file popups
	Array<EqString>	m_resFiles{ PP_SL };
	char			m_pathInput[256]{};
	char			m_fileFilter[64]{};
	EqString		m_addFileTargetPath;	// control that gets 'child file'
	bool			m_addFileEditorOnly{ false };
	bool			m_openPopupRequest{ false };
	bool			m_saveAsPopupRequest{ false };
	bool			m_addFilePopupRequest{ false };
};

static CStaticAutoPtr<CUIEditor> s_uiEditor;

//-------------------------------------------------------------------------------------------------

static void ScanResFiles(const EqString& dirPath, Array<EqString>& outFiles)
{
	CFileSystemFind fsFind(EqString::Format("%s/*", dirPath), SP_MOD);
	while (fsFind.Next())
	{
		const EqStringRef name = fsFind.GetPath();
		if (name[0] == '.')
			continue;

		const EqString filePath = EqString::Format("%s/%s", dirPath, name);
		if (fsFind.IsDirectory())
		{
			ScanResFiles(filePath, outFiles);
			continue;
		}

		if (fnmPathExtractExt(name).CompareCaseIns("res"))
			continue;

		// same files may come from several search paths
		const int foundIdx = arrayFindIndexF(outFiles, [&](const EqString& file) {
			return !file.CompareCaseIns(filePath);
		});

		if (foundIdx == -1)
			outFiles.append(filePath);
	}
}

static void RemoveEditorOnlySections(KVSection& sec)
{
	for (int i = sec.KeyCount() - 1; i >= 0; --i)
	{
		KVSection& key = sec.KeyAt(i);
		if (!CString::CompareCaseIns(key.GetName(), "child") && IsEditorOnly(key))
			sec.RemoveSection(&key);
		else if (key.KeyCount() > 0)
			RemoveEditorOnlySections(key);
	}
}

static bool IsIncludeSection(const KVSection& childSec)
{
	return childSec.ValueCount() > 1 && !CString::CompareCaseIns(childSec.ValueAt(0).GetString(), "file");
}

static const char* GetChildSectionName(const KVSection& childSec)
{
	return childSec.ValueCount() > 1 ? childSec.ValueAt(1).GetString() : "";
}

static void AddFontNames(const KVSection& sec, Array<EqString>& outNames)
{
	const KVSection* fontsSec = sec.FindSection("fonts", KV_FLAG_SECTION);
	if (!fontsSec)
		return;

	for (const KVSection& fontSec : fontsSec->Keys())
	{
		const int foundIdx = arrayFindIndexF(outNames, [&](const EqString& name) {
			return !name.CompareCaseIns(fontSec.GetName());
		});

		if (foundIdx == -1)
			outNames.append(fontSec.GetName());
	}
}

// containers and controls with selfVisible off draw nothing
static bool IsDrawnControl(const IUIControl* control)
{
	return control->IsSelfVisible()
		&& CString::CompareCaseIns(control->GetClassname(), "Container")
		&& CString::CompareCaseIns(control->GetClassname(), "HudElement");
}

// Unlike IUIControl::HitTest, children outside of the parent rectangle are tested, and controls
// that draw nothing are picked only when no drawn control is under the point.
static void EditorHitTest(IUIControl* control, const IVector2D& point, IUIControl*& drawnHit, IUIControl*& anyHit)
{
	if (!control->IsVisible())
		return;

	// first children are drawn on top
	for (auto it = control->GetFirstChild(); !it.atEnd() && !drawnHit; ++it)
		EditorHitTest(*it, point, drawnHit, anyHit);

	if (drawnHit || !control->GetClientRectangle().Contains(point))
		return;

	if (IsDrawnControl(control))
		drawnHit = control;
	else if (!anyHit)
		anyHit = control;
}

static void SetIntPair(KVSection& sec, const char* name, int x, int y)
{
	KVSection& key = sec.CreateSection(name);
	key.AddValue(x);
	key.AddValue(y);
}

// path of child names without the last one
static EqString GetParentPath(const EqString& path)
{
	const char* separator = strrchr(path.ToCString(), '.');
	if (!separator)
		return EqString();
	return EqString(path.ToCString(), static_cast<int>(separator - path.ToCString()));
}

static EqString JoinPath(const EqString& parentPath, const char* name)
{
	if (parentPath.Length() == 0)
		return EqString(name);
	return EqString::Format("%s.%s", parentPath, name);
}

static int KVTextResizeCb(ImGuiInputTextCallbackData* data)
{
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
	{
		Array<char>* text = static_cast<Array<char>*>(data->UserData);
		text->setNum(data->BufTextLen + 1, false);
		data->Buf = text->ptr();
	}
	return 0;
}

//-------------------------------------------------------------------------------------------------

void CUIEditor::DestroyPreviewPanel()
{
	// panel is owned by the preview root unless it is the root element
	if (m_panel && !m_panel->GetParent())
		delete m_panel;
	m_panel = nullptr;

	if (m_previewRoot)
		m_previewRoot->ClearChilds(true);
}

void CUIEditor::Shutdown()
{
	DestroyPreviewPanel();
	SAFE_DELETE(m_previewRoot);
	ClearIncludes();
	m_mainDoc = nullptr;
	m_previewTarget = nullptr;
	m_rebuildPending = true;
}

void CUIEditor::ResetDocumentState()
{
	ClearIncludes();

	// layouts without the panel section are loaded into a root element (HUD), others are added as panels
	m_rootElement = m_mainDoc->root.FindSection("panel", KV_FLAG_SECTION) == nullptr;

	m_mainDoc->modified = false;
	m_rebuildPending = true;
	m_kvDocIdx = -1;
	m_kvTextDirty = true;
	m_kvTextEdited = false;
	m_editorHiddenPaths.clear();
	Select("");
}

void CUIEditor::NewDocument()
{
	m_mainDoc = CRefPtr_new(LayoutDoc);

	KVSection& panelSec = m_mainDoc->root.CreateSection("panel", "NewPanel");
	SetIntPair(panelSec, "position", 0, 0);
	SetIntPair(panelSec, "size", 640, 480);

	ResetDocumentState();
}

bool CUIEditor::LoadDocument(const char* filePath)
{
	IFileStreamPtr file = g_fileSystem->Open(filePath, FS_OPEN_READ, SP_MOD);
	LayoutDocPtr doc = CRefPtr_new(LayoutDoc);
	if (!file || !KeyValues::Parse(file, doc->root))
	{
		MsgError("EqUI editor: can't load '%s'\n", filePath);
		return false;
	}

	doc->filePath = filePath;
	m_mainDoc = doc;

	ResetDocumentState();
	return true;
}

bool CUIEditor::SaveDocument(LayoutDoc& doc, const char* filePath)
{
	KVSection document;
	doc.root.CopyTo(document);
	RemoveEditorOnlySections(document);

	IFileStreamPtr file = g_fileSystem->Open(filePath, FS_OPEN_WRITE, SP_MOD);
	if (!file)
	{
		MsgError("EqUI editor: can't write '%s'\n", filePath);
		return false;
	}

	KeyValues::WriteText(file, document);

	doc.filePath = filePath;
	doc.modified = false;
	Msg("EqUI editor: saved '%s'\n", filePath);
	return true;
}

void CUIEditor::SaveIncludes()
{
	for (const LayoutDocPtr& doc : m_includeDocs)
	{
		if (doc->modified)
			SaveDocument(*doc, EqString(doc->filePath));
	}
}

bool CUIEditor::IsModified() const
{
	if (m_mainDoc->modified)
		return true;

	for (const LayoutDocPtr& doc : m_includeDocs)
	{
		if (doc->modified)
			return true;
	}
	return false;
}

void CUIEditor::OnDocumentChanged(LayoutDoc* doc)
{
	if (doc)
		doc->modified = true;

	m_rebuildPending = true;
	m_kvTextDirty = true;
}

void CUIEditor::RebuildPreview()
{
	m_rebuildPending = false;

	if (!m_previewRoot)
	{
		// stands for the manager root panel which covers the view frame
		m_previewRoot = Manager->Create<Panel>();
		m_previewRoot->SetName("equi_editor_root");
		m_previewRoot->SetSelfVisible(false);
	}

	DestroyPreviewPanel();

	m_panel = Manager->Create<Panel>();
	if (!m_rootElement)
		m_previewRoot->AddChild(m_panel);

	// included files come from the editor so their changes are shown before saving
	Manager->SetLayoutFileLoader([this](const char* fileName, KVSection& outSec) {
		const LayoutDoc* doc = GetIncludeDoc(fileName);
		if (!doc)
			return false;

		doc->root.CopyTo(outSec);
		return true;
	});

	m_panel->InitFromKeyValues(m_mainDoc->root);

	Manager->SetLayoutFileLoader(nullptr);

	// window panels are hidden after parsing
	m_panel->Show();

	// root elements are containers in the game, they don't draw themselves
	if (m_rootElement)
		m_panel->SetSelfVisible(false);

	for (const EqString& path : m_editorHiddenPaths)
	{
		IUIControl* control = path.Length() ? m_panel->Get(path) : m_panel;
		if (control)
			control->Hide();
	}
}

void CUIEditor::UpdateKVText()
{
	m_kvTextDirty = false;

	CMemoryStream stream(PP_SL);
	stream.Open(FS_OPEN_WRITE, nullptr, 4096);
	KeyValues::WriteText(&stream, GetKVDoc()->root);

	const int textLength = static_cast<int>(stream.Tell());
	m_kvText.setNum(textLength + 1, false);
	memcpy(m_kvText.ptr(), stream.GetBasePointer(), textLength);
	m_kvText[textLength] = 0;
}

//-------------------------------------------------------------------------------------------------

void CUIEditor::ClearIncludes()
{
	m_includeDocs.clear();
}

// discards changes of the file, the file that failed to load is loaded again
bool CUIEditor::ReloadIncludeDoc(LayoutDoc& doc)
{
	IFileStreamPtr file = g_fileSystem->Open(doc.filePath, FS_OPEN_READ, SP_MOD);
	KVSection root;
	if (!file || !KeyValues::Parse(file, root))
	{
		MsgError("EqUI editor: can't reload '%s'\n", doc.filePath.ToCString());
		return false;
	}

	doc.root.Clear();
	root.CopyTo(doc.root);
	doc.modified = false;
	doc.valid = true;

	if (GetKVDoc() == &doc)
		m_kvTextEdited = false;

	m_rebuildPending = true;
	m_kvTextDirty = true;
	return true;
}

LayoutDoc* CUIEditor::GetIncludeDoc(const char* filePath)
{
	for (const LayoutDocPtr& doc : m_includeDocs)
	{
		if (!doc->filePath.CompareCaseIns(filePath))
			return doc->valid ? doc.Ptr() : nullptr;
	}

	LayoutDocPtr doc = CRefPtr_new(LayoutDoc);
	doc->filePath = filePath;
	m_includeDocs.append(doc);

	// same as CUIManager::LoadLayoutFile
	IFileStreamPtr file = g_fileSystem->Open(filePath, FS_OPEN_READ, SP_MOD);
	if (!file || !KeyValues::Parse(file, doc->root))
	{
		MsgWarning("EqUI editor: can't load included file '%s'\n", filePath);
		doc->valid = false;
		return nullptr;
	}

	return doc.Ptr();
}

LayoutDoc* CUIEditor::GetKVDoc()
{
	if (m_includeDocs.inRange(m_kvDocIdx) && m_includeDocs[m_kvDocIdx]->valid)
		return m_includeDocs[m_kvDocIdx].Ptr();
	return m_mainDoc.Ptr();
}

KVSection& CUIEditor::GetPanelSection()
{
	// same as Panel::InitFromKeyValues
	KVSection* panelSec = m_mainDoc->root.FindSection("panel", KV_FLAG_SECTION);
	return panelSec ? *panelSec : m_mainDoc->root;
}

// same order as IUIControl::InitChildItems, later section with the same name overrides the control
void CUIEditor::FindChildSection(KVSection& sec, LayoutDoc* doc, const char* name, SectionRef& result, int depth, bool editorOnlyInclude)
{
	for (KVSection& childSec : sec.Keys("child"))
	{
		if (IsIncludeSection(childSec))
		{
			LayoutDoc* includeDoc = depth < s_maxIncludeDepth ? GetIncludeDoc(GetChildSectionName(childSec)) : nullptr;
			if (includeDoc)
				FindChildSection(includeDoc->root, includeDoc, name, result, depth + 1, editorOnlyInclude || IsEditorOnly(childSec));
			continue;
		}

		if (!CString::Compare(GetChildSectionName(childSec), name))
			result = SectionRef{ &childSec, &sec, doc, editorOnlyInclude };
	}
}

SectionRef CUIEditor::FindSectionByPath(const char* path)
{
	SectionRef ref{ &GetPanelSection(), nullptr, m_mainDoc.Ptr() };

	EqString name;
	for (const char* c = path; ; ++c)
	{
		if (*c && *c != '.')
		{
			name.Append(*c);
			continue;
		}

		if (name.Length())
		{
			SectionRef childRef;
			FindChildSection(*ref.sec, ref.doc, name, childRef, 0, false);
			if (!childRef.sec)
				return SectionRef();

			// children of controls from editor-only files are removed with them
			childRef.editorOnlyInclude = childRef.editorOnlyInclude || ref.editorOnlyInclude;
			ref = childRef;
			name.Clear();
		}

		if (!*c)
			break;
	}
	return ref;
}

EqString CUIEditor::GetControlPath(const IUIControl* control) const
{
	EqString path;
	for (; control && control != m_panel; control = control->GetParent())
	{
		if (path.Length())
			path = EqString::Format("%s.%s", control->GetName(), path);
		else
			path = control->GetName();
	}
	return path;
}

IUIControl* CUIEditor::GetSelectedControl() const
{
	if (!m_hasSelection || !m_panel)
		return nullptr;

	if (m_selection.Length() == 0)
		return m_panel;

	return m_panel->Get(m_selection);
}

void CUIEditor::Select(const char* path)
{
	m_selection = path;
	m_hasSelection = true;
}

bool CUIEditor::IsEditorHidden(const char* path) const
{
	return arrayFindIndex(m_editorHiddenPaths, EqString(path)) != -1;
}

void CUIEditor::SetEditorHidden(const char* path, bool hidden)
{
	const int pathIdx = arrayFindIndex(m_editorHiddenPaths, EqString(path));
	if (hidden && pathIdx == -1)
		m_editorHiddenPaths.append(path);
	else if (!hidden && pathIdx != -1)
		m_editorHiddenPaths.fastRemoveIndex(pathIdx);

	// rebuild restores visibility from the layout
	m_rebuildPending = true;
}

void CUIEditor::AddChildSection(const SectionRef& parentRef, const EqString& parentPath, const char* className, bool editorOnly)
{
	KVSection& parentSec = *parentRef.sec;

	EqString name;
	for (int i = 1; ; ++i)
	{
		name = EqString::Format("%s%d", className, i);

		bool nameUsed = false;
		for (const KVSection& key : parentSec.Keys("child"))
			nameUsed = nameUsed || !CString::Compare(GetChildSectionName(key), name);

		if (!nameUsed)
			break;
	}

	KVSection& childSec = parentSec.CreateSection("child", className);
	childSec.AddValue(name);
	SetIntPair(childSec, "position", 10, 10);
	SetIntPair(childSec, "size", 100, 30);

	if (!CString::CompareCaseIns(className, "label") || !CString::CompareCaseIns(className, "button"))
		childSec.SetKey("label", name);

	if (editorOnly)
		childSec.SetKey(EDITOR_ONLY_KEY, 1);

	Select(JoinPath(parentPath, name));
	OnDocumentChanged(parentRef.doc);
}

void CUIEditor::RemoveChildSection(const EqString& path)
{
	const SectionRef ref = FindSectionByPath(path);
	if (!ref.sec || !ref.parent)
		return;

	ref.parent->RemoveSection(ref.sec);

	// selection inside of the removed control
	const EqString childPrefix = EqString::Format("%s.", path);
	if (m_hasSelection && (m_selection == path || !strncmp(m_selection, childPrefix, childPrefix.Length())))
		Select(GetParentPath(path));

	OnDocumentChanged(ref.doc);
}

void CUIEditor::MoveChildSection(const EqString& path, EMoveChild move)
{
	const SectionRef ref = FindSectionByPath(path);
	if (!ref.sec || !ref.parent)
		return;

	Array<KVSection*>& keys = ref.parent->keys;
	const int idx = arrayFindIndex(keys, ref.sec);
	if (idx == -1)
		return;

	// only child keys are passed, other keys stay in place
	const auto findChildKey = [&](int from, int step) {
		for (int i = from + step; i >= 0 && i < keys.numElem(); i += step)
		{
			if (!CString::CompareCaseIns(keys[i]->GetName(), "child"))
				return i;
		}
		return -1;
	};

	switch (move)
	{
		case MOVE_CHILD_UP:
		case MOVE_CHILD_DOWN:
		{
			const int otherIdx = findChildKey(idx, move == MOVE_CHILD_UP ? 1 : -1);
			if (otherIdx == -1)
				return;

			keys[idx] = keys[otherIdx];
			keys[otherIdx] = ref.sec;
			break;
		}
		case MOVE_CHILD_TO_BEGINNING:
		{
			if (findChildKey(idx, 1) == -1)
				return;

			keys.removeIndex(idx);
			keys.append(ref.sec);
			break;
		}
		case MOVE_CHILD_TO_END:
		{
			const int firstIdx = findChildKey(-1, 1);
			if (firstIdx == idx)
				return;

			keys.removeIndex(idx);
			keys.insert(ref.sec, firstIdx);
			break;
		}
	}

	OnDocumentChanged(ref.doc);
}

void CUIEditor::CollectSectionFonts(const SectionRef& ref, Array<EqString>& outNames)
{
	if (!ref.sec)
		return;

	AddFontNames(*ref.sec, outNames);

	// included files add their fonts to the control that includes them
	for (const KVSection& childSec : ref.sec->Keys("child"))
	{
		if (!IsIncludeSection(childSec))
			continue;

		if (const LayoutDoc* includeDoc = GetIncludeDoc(GetChildSectionName(childSec)))
			AddFontNames(includeDoc->root, outNames);
	}
}

// named fonts are searched in the control and its parents
void CUIEditor::CollectLocalFonts(const EqString& path, Array<EqString>& outNames)
{
	CollectSectionFonts(FindSectionByPath(""), outNames);

	for (int i = 1; i <= path.Length(); ++i)
	{
		if (i == path.Length() || path[i] == '.')
			CollectSectionFonts(FindSectionByPath(EqString(path.ToCString(), i)), outNames);
	}

	arraySort(outNames, [](const EqString& a, const EqString& b) {
		return a.CompareCaseIns(b);
	});
}

//-------------------------------------------------------------------------------------------------

void CUIEditor::Draw(bool& open)
{
	if (!open)
		return;

	if (m_rebuildPending)
	{
		if (!m_mainDoc)
			NewDocument();
		RebuildPreview();
	}

	if (m_kvTextDirty && !m_kvTextEdited)
		UpdateKVText();

	const char* fileName = m_mainDoc->filePath.Length() ? m_mainDoc->filePath.ToCString() : "untitled";
	const EqString title = EqString::Format("EqUI Editor - %s%s###equiEditor", fileName, IsModified() ? "*" : "");

	ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(title, &open, ImGuiWindowFlags_MenuBar))
	{
		ImGui::End();
		return;
	}

	DrawMenuBar();
	DrawOpenPopup();
	DrawSaveAsPopup();
	DrawAddFilePopup();

	const float layoutHeight = ImGui::GetContentRegionAvail().y;
	constexpr ImGuiTableFlags layoutFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
	if (ImGui::BeginTable("##equiEditorLayout", 3, layoutFlags))
	{
		ImGui::TableSetupColumn("Outliner", ImGuiTableColumnFlags_WidthFixed, 260.0f);
		ImGui::TableSetupColumn("View", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthFixed, 360.0f);
		ImGui::TableNextRow();

		ImGui::TableNextColumn();
		if (ImGui::BeginChild("##outliner", ImVec2(0, layoutHeight)))
			DrawOutliner();
		ImGui::EndChild();

		ImGui::TableNextColumn();
		if (ImGui::BeginChild("##view", ImVec2(0, layoutHeight)))
		{
			if (ImGui::BeginTabBar("##viewTabs"))
			{
				if (ImGui::BeginTabItem("Preview"))
				{
					DrawPreviewTab();
					ImGui::EndTabItem();
				}

				const ImGuiTabItemFlags kvTabFlags = m_selectKVTab ? ImGuiTabItemFlags_SetSelected : 0;
				m_selectKVTab = false;
				if (ImGui::BeginTabItem("KeyValues", nullptr, kvTabFlags))
				{
					DrawKeyValuesTab();
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::EndChild();

		ImGui::TableNextColumn();
		if (ImGui::BeginChild("##inspector", ImVec2(0, layoutHeight)))
			DrawInspector();
		ImGui::EndChild();

		ImGui::EndTable();
	}

	ImGui::End();
}

void CUIEditor::DrawMenuBar()
{
	if (!ImGui::BeginMenuBar())
		return;

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New"))
			NewDocument();

		if (ImGui::MenuItem("Open..."))
			m_openPopupRequest = true;

		if (ImGui::MenuItem("Reload", nullptr, false, m_mainDoc->filePath.Length() > 0))
			LoadDocument(EqString(m_mainDoc->filePath));

		if (ImGui::MenuItem("Reload included files", nullptr, false, m_includeDocs.numElem() > 0))
		{
			for (const LayoutDocPtr& doc : m_includeDocs)
				ReloadIncludeDoc(*doc);
		}

		ImGui::Separator();

		// included files are saved together with the layout
		if (ImGui::MenuItem("Save", nullptr, false, IsModified()))
		{
			if (m_mainDoc->filePath.Length())
			{
				if (m_mainDoc->modified)
					SaveDocument(*m_mainDoc, EqString(m_mainDoc->filePath));
				SaveIncludes();
			}
			else
			{
				m_saveAsPopupRequest = true;
			}
		}

		if (ImGui::MenuItem("Save As..."))
			m_saveAsPopupRequest = true;

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("View"))
	{
		ImGui::MenuItem("Show control bounds", nullptr, &m_showBounds);
		ImGui::ColorEdit4("Background", &m_previewClearColor.x, ImGuiColorEditFlags_NoInputs);
		ImGui::EndMenu();
	}

	ImGui::EndMenuBar();
}

void CUIEditor::RequestResFileList()
{
	m_resFiles.clear();
	ScanResFiles(s_resourcesDir, m_resFiles);
	arraySort(m_resFiles, [](const EqString& a, const EqString& b) {
		return a.CompareCaseIns(b);
	});
}

// filtered list of .res files with the path input, returns true when a file is double-clicked
bool CUIEditor::DrawResFileList(const char* selectedPath, EqString& outActivated)
{
	bool activated = false;

	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputTextWithHint("##filter", "Filter", m_fileFilter, sizeof(m_fileFilter));

	const float bottomHeight = ImGui::GetFrameHeightWithSpacing() * 2.0f;
	if (ImGui::BeginChild("##files", ImVec2(0, -bottomHeight), true))
	{
		for (const EqString& file : m_resFiles)
		{
			if (m_fileFilter[0] && file.Find(m_fileFilter) == -1)
				continue;

			if (ImGui::Selectable(file, !file.CompareCaseIns(selectedPath), ImGuiSelectableFlags_AllowDoubleClick))
			{
				strncpy(m_pathInput, file.ToCString(), sizeof(m_pathInput) - 1);
				if (ImGui::IsMouseDoubleClicked(0))
				{
					outActivated = file;
					activated = true;
				}
			}
		}
	}
	ImGui::EndChild();

	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputText("##path", m_pathInput, sizeof(m_pathInput));

	return activated;
}

void CUIEditor::DrawOpenPopup()
{
	if (m_openPopupRequest)
	{
		m_openPopupRequest = false;
		RequestResFileList();

		strncpy(m_pathInput, m_mainDoc->filePath.ToCString(), sizeof(m_pathInput) - 1);
		ImGui::OpenPopup("Open layout");
	}

	ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Appearing);
	if (!ImGui::BeginPopupModal("Open layout"))
		return;

	EqString activatedFile;
	if (DrawResFileList(m_pathInput, activatedFile) && LoadDocument(activatedFile))
		ImGui::CloseCurrentPopup();

	if (ImGui::Button("Open") && LoadDocument(m_pathInput))
		ImGui::CloseCurrentPopup();

	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
		ImGui::CloseCurrentPopup();

	ImGui::EndPopup();
}

void CUIEditor::DrawSaveAsPopup()
{
	if (m_saveAsPopupRequest)
	{
		m_saveAsPopupRequest = false;
		strncpy(m_pathInput, m_mainDoc->filePath.Length() ? m_mainDoc->filePath.ToCString() : "resources/", sizeof(m_pathInput) - 1);
		ImGui::OpenPopup("Save layout");
	}

	if (!ImGui::BeginPopupModal("Save layout", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	ImGui::TextUnformatted("File path (SP_MOD):");
	ImGui::SetNextItemWidth(400.0f);
	ImGui::InputText("##path", m_pathInput, sizeof(m_pathInput));

	if (ImGui::Button("Save") && SaveDocument(*m_mainDoc, m_pathInput))
	{
		SaveIncludes();
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
		ImGui::CloseCurrentPopup();

	ImGui::EndPopup();
}

void CUIEditor::DrawAddFilePopup()
{
	if (m_addFilePopupRequest)
	{
		m_addFilePopupRequest = false;
		RequestResFileList();

		m_pathInput[0] = 0;
		ImGui::OpenPopup("Add child file");
	}

	ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Appearing);
	if (!ImGui::BeginPopupModal("Add child file"))
		return;

	ImGui::Text("Add to: %s", m_addFileTargetPath.Length() ? m_addFileTargetPath.ToCString() : "(panel)");
	ImGui::Checkbox("Editor only", &m_addFileEditorOnly);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Controls of the file are shown in the preview only, the entry is removed on save");

	EqString activatedFile;
	if (DrawResFileList(m_pathInput, activatedFile))
	{
		AddIncludeSection(m_addFileTargetPath, activatedFile, m_addFileEditorOnly);
		ImGui::CloseCurrentPopup();
	}

	ImGui::BeginDisabled(m_pathInput[0] == 0);
	if (ImGui::Button("Add"))
	{
		AddIncludeSection(m_addFileTargetPath, m_pathInput, m_addFileEditorOnly);
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
		ImGui::CloseCurrentPopup();

	ImGui::EndPopup();
}

void CUIEditor::AddIncludeSection(const EqString& path, const char* filePath, bool editorOnly)
{
	const SectionRef ref = FindSectionByPath(path);
	if (!ref.sec)
		return;

	// file that includes itself would be loaded recursively by IUIControl::InitChildItems
	if (!m_mainDoc->filePath.CompareCaseIns(filePath) || !ref.doc->filePath.CompareCaseIns(filePath))
	{
		MsgError("EqUI editor: '%s' can't include itself\n", filePath);
		return;
	}

	KVSection& includeSec = ref.sec->CreateSection("child", "file");
	includeSec.AddValue(filePath);

	if (editorOnly)
		includeSec.SetKey(EDITOR_ONLY_KEY, 1);

	OnDocumentChanged(ref.doc);
}

//-------------------------------------------------------------------------------------------------

void CUIEditor::DrawOutliner()
{
	ImGui::TextUnformatted("Outliner");
	ImGui::Separator();

	m_hasOutlinerHover = false;

	if (m_panel)
		DrawOutlinerNode(m_panel);
}

void CUIEditor::DrawOutlinerNode(IUIControl* control)
{
	const EqString path = GetControlPath(control);
	const SectionRef ref = FindSectionByPath(path);
	const bool isPanel = control == m_panel;
	const bool sectionEditorOnly = ref.sec && IsEditorOnly(*ref.sec);
	const bool editorOnly = sectionEditorOnly || ref.editorOnlyInclude;
	const bool fromInclude = ref.doc && ref.doc != m_mainDoc.Ptr();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (isPanel)
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	if (control->GetFirstChild().atEnd())
		flags |= ImGuiTreeNodeFlags_Leaf;
	if (m_hasSelection && m_selection == path)
		flags |= ImGuiTreeNodeFlags_Selected;

	// controls without a section are created by the control itself, e.g. window controls
	const bool hasTextColor = editorOnly || fromInclude || !ref.sec;
	if (editorOnly)
		ImGui::PushStyleColor(ImGuiCol_Text, s_editorOnlyTextColor);
	else if (fromInclude)
		ImGui::PushStyleColor(ImGuiCol_Text, s_includeTextColor);
	else if (!ref.sec)
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

	bool editorVisible = !IsEditorHidden(path);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
	if (ImGui::Checkbox(EqString::Format("##editorVisible%s", path), &editorVisible))
		SetEditorHidden(path, !editorVisible);
	ImGui::PopStyleVar();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Show in the preview, not saved to the layout");

	ImGui::SameLine();

	// ids are paths so the tree state is kept when the preview is rebuilt
	const bool nodeOpen = ImGui::TreeNodeEx(EqString::Format("#%s", path), flags, "%s [%s]%s%s%s",
		control->GetName(), control->GetClassname(),
		editorOnly ? " (editor only)" : "",
		fromInclude ? " - " : "", fromInclude ? fnmPathExtractName(ref.doc->filePath).ToCString() : "");

	if (hasTextColor)
		ImGui::PopStyleColor();

	if (ImGui::IsItemHovered())
	{
		m_outlinerHoverPath = path;
		m_hasOutlinerHover = true;
	}

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		Select(path);

	if (ref.sec && ImGui::BeginPopupContextItem())
	{
		if (ImGui::BeginMenu("Add child"))
		{
			DrawAddChildMenu(ref, path, false);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Add editor-only child"))
		{
			DrawAddChildMenu(ref, path, true);
			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Add child file..."))
		{
			m_addFileTargetPath = path;
			m_addFilePopupRequest = true;
		}

		if (fromInclude && ImGui::MenuItem(EqString::Format("Reload %s", fnmPathExtractName(ref.doc->filePath).ToCString())))
			ReloadIncludeDoc(*ref.doc);

		ImGui::Separator();
		if (ImGui::BeginMenu("Layout"))
		{
			if (DrawLayoutProperties(*ref.sec))
				OnDocumentChanged(ref.doc);
			ImGui::EndMenu();
		}

		if (!isPanel)
		{
			ImGui::Separator();

			static const char* s_moveItems[] = { "Move up", "Move down", "Move to beginning", "Move to end" };
			for (int i = 0; i < static_cast<int>(elementsOf(s_moveItems)); ++i)
			{
				if (ImGui::MenuItem(s_moveItems[i]))
					MoveChildSection(path, static_cast<EMoveChild>(i));

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Upper controls are drawn on top and written later in the file");
			}

			ImGui::Separator();

			bool editorOnlyValue = sectionEditorOnly;
			if (ImGui::MenuItem("Editor only", nullptr, &editorOnlyValue))
			{
				if (editorOnlyValue)
					ref.sec->SetKey(EDITOR_ONLY_KEY, 1);
				else
					ref.sec->RemoveSectionByName(EDITOR_ONLY_KEY);
				OnDocumentChanged(ref.doc);
			}

			if (ImGui::MenuItem("Delete"))
				RemoveChildSection(path);
		}
		ImGui::EndPopup();
	}

	if (!nodeOpen)
		return;

	for (auto it = control->GetFirstChild(); !it.atEnd(); ++it)
		DrawOutlinerNode(*it);

	ImGui::TreePop();
}

void CUIEditor::DrawAddChildMenu(const SectionRef& ref, const EqString& path, bool editorOnly)
{
	// engine and game controls registered in the manager
	static Array<EqStringRef> classNames{ PP_SL };
	classNames.clear();

	Manager->GetControlClassNames(classNames);
	arraySort(classNames, [](const char* a, const char* b) {
		return CString::CompareCaseIns(a, b);
	});

	for (EqStringRef className : classNames)
	{
		if (ImGui::MenuItem(className))
			AddChildSection(ref, path, className, editorOnly);
	}
}

//-------------------------------------------------------------------------------------------------

void CUIEditor::RenderPreview(const IVector2D& size)
{
	if (!m_previewTarget)
	{
		// filtered for zoom
		const SamplerStateParams sampler(TEXFILTER_LINEAR, TEXADDRESS_CLAMP);
		m_previewTarget = g_renderAPI->CreateRenderTarget(
			Builder<TextureDesc>()
			.Name("_equi_editor_preview")
			.Size(size.x, size.y)
			.Format(g_matSystem->GetCurrentBackbuffer()->GetFormat())
			.Sampler(sampler)
			.End()
		);
	}
	else if (m_previewTarget->GetWidth() != size.x || m_previewTarget->GetHeight() != size.y)
	{
		g_renderAPI->ResizeRenderTarget(m_previewTarget, { size.x, size.y, 1 });
	}

	// screen overlay panels are resized by CUIManager::Render, HUD layouts are resized by the game
	if (m_stretchPanel || KV_GetValueBool(GetPanelSection().FindSection("screenoverlay"), 0, false))
	{
		m_panel->SetPosition(0);
		m_panel->SetSize(size);
	}

	// parsed size is the base size, so root does not scale its children like the manager root
	if (m_previewRoot->GetSize() != size)
	{
		KVSection rootSec;
		SetIntPair(rootSec, "position", 0, 0);
		SetIntPair(rootSec, "size", size.x, size.y);
		m_previewRoot->Parse(rootSec);
	}

	// editor is drawn in the middle of the host frame
	Matrix4x4 prevProj, prevView, prevWorld, prevWorld2;
	g_matSystem->GetMatrix(MATRIXMODE_PROJECTION, prevProj);
	g_matSystem->GetMatrix(MATRIXMODE_VIEW, prevView);
	g_matSystem->GetMatrix(MATRIXMODE_WORLD, prevWorld);
	g_matSystem->GetMatrix(MATRIXMODE_WORLD2, prevWorld2);

	g_matSystem->Setup2D(size.x, size.y);

	IGPURenderPassRecorderPtr rendPassRecorder = g_renderAPI->BeginRenderPass(
		Builder<RenderPassDesc>()
		.ColorTarget(m_previewTarget, true, m_previewClearColor)
		.End()
	);

	ControlRenderContext ctrlRenderContext{ rendPassRecorder };
	if (m_rootElement)
		m_panel->Render(1, ctrlRenderContext);
	else
		m_previewRoot->Render(1, ctrlRenderContext);

	g_matSystem->QueueCommandBuffer(rendPassRecorder->End());

	g_matSystem->SetMatrix(MATRIXMODE_PROJECTION, prevProj);
	g_matSystem->SetMatrix(MATRIXMODE_VIEW, prevView);
	g_matSystem->SetMatrix(MATRIXMODE_WORLD, prevWorld);
	g_matSystem->SetMatrix(MATRIXMODE_WORLD2, prevWorld2);
}

void CUIEditor::DrawPreviewTab()
{
	ImGui::SetNextItemWidth(150.0f);
	if (ImGui::BeginCombo("Size", s_previewSizes[m_previewSizeIdx].name))
	{
		for (int i = 0; i < static_cast<int>(elementsOf(s_previewSizes)); ++i)
		{
			if (ImGui::Selectable(s_previewSizes[i].name, i == m_previewSizeIdx))
				m_previewSizeIdx = i;
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Root element", &m_rootElement))
		m_rebuildPending = true;
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Render the layout without a parent and without its own background, like the game HUD.\nOff: the layout is a panel added to the manager root panel.");

	ImGui::SameLine();
	ImGui::Checkbox("Stretch panel", &m_stretchPanel);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Resize the panel to the preview size, children are scaled from the layout size");

	ImGui::SameLine();
	ImGui::Checkbox("Bounds", &m_showBounds);

	IVector2D size = s_previewSizes[m_previewSizeIdx].size;
	const bool fitToWindow = size.x == 0;

	ImGui::SameLine();
	ImGui::BeginDisabled(fitToWindow);
	ImGui::SetNextItemWidth(150.0f);
	ImGui::SliderInt("Zoom", &m_zoomPercent, s_minZoomPercent, s_maxZoomPercent, "%d%%");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Ctrl + mouse wheel over the preview, double-click resets to 100%%");
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		m_zoomPercent = 100;
	ImGui::EndDisabled();

	// mouse wheel with Ctrl zooms instead of scrolling
	const ImGuiWindowFlags areaFlags = ImGuiWindowFlags_HorizontalScrollbar | (ImGui::GetIO().KeyCtrl ? ImGuiWindowFlags_NoScrollWithMouse : 0);
	if (!ImGui::BeginChild("##previewArea", ImVec2(0, 0), false, areaFlags))
	{
		ImGui::EndChild();
		return;
	}

	const ImVec2 availSize = ImGui::GetContentRegionAvail();
	if (fitToWindow)
		size = IVector2D(max(static_cast<int>(availSize.x), 1), max(static_cast<int>(availSize.y), 1));

	const float zoom = fitToWindow ? 1.0f : m_zoomPercent / 100.0f;
	const ImVec2 displaySize(floorf(size.x * zoom), floorf(size.y * zoom));

	if (m_panel)
	{
		RenderPreview(size);

		ImVec2 origin = ImGui::GetCursorScreenPos();
		ImVec2 rulerSize(0.0f, 0.0f);

		// fixed resolution is centered in the area, rulers are on the top and left sides
		if (!fitToWindow)
		{
			rulerSize = ImVec2(ImGui::CalcTextSize("0000").x + 6.0f, ImGui::GetFontSize() + 4.0f);

			const ImVec2 blockSize(rulerSize.x + displaySize.x, rulerSize.y + displaySize.y);
			const ImVec2 offset(max((availSize.x - blockSize.x) * 0.5f, 0.0f), max((availSize.y - blockSize.y) * 0.5f, 0.0f));
			origin = ImVec2(floorf(origin.x + offset.x + rulerSize.x), floorf(origin.y + offset.y + rulerSize.y));
			ImGui::SetCursorScreenPos(origin);
		}

		ImGui::Image(reinterpret_cast<ImTextureID>(m_previewTarget.Ptr()), displaySize);
		const bool previewHovered = ImGui::IsItemHovered();
		const bool previewClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

		const ImGuiIO& io = ImGui::GetIO();
		if (previewHovered && !fitToWindow && io.KeyCtrl && io.MouseWheel != 0.0f)
			m_zoomPercent = clamp(m_zoomPercent + static_cast<int>(io.MouseWheel) * 10, s_minZoomPercent, s_maxZoomPercent);

		IUIControl* hovered = nullptr;
		if (previewHovered)
		{
			const ImVec2 mousePos = ImGui::GetMousePos();
			const IVector2D point(static_cast<int>((mousePos.x - origin.x) / zoom), static_cast<int>((mousePos.y - origin.y) / zoom));

			IUIControl* drawnHit = nullptr;
			IUIControl* anyHit = nullptr;
			EditorHitTest(m_panel, point, drawnHit, anyHit);
			hovered = drawnHit ? drawnHit : anyHit;
		}

		if (hovered && previewClicked)
			Select(GetControlPath(hovered));

		if (hovered && ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			Select(GetControlPath(hovered));
			m_layoutPopupPath = m_selection;
			ImGui::OpenPopup("##controlLayout");
		}

		if (hovered && !ImGui::IsPopupOpen("##controlLayout"))
			ImGui::SetTooltip("%s [%s]", hovered->GetName(), hovered->GetClassname());

		DrawLayoutPopup();

		// arrows move the selected control, Shift moves by 10
		if (ImGui::IsWindowFocused() && !io.WantTextInput)
		{
			const int step = io.KeyShift ? 10 : 1;
			const IVector2D move(
				(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)) ? step : 0) - (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)) ? step : 0),
				(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)) ? step : 0) - (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)) ? step : 0));

			if (move.x || move.y)
				MoveSelectedControl(move);
		}

		// control under the mouse in the outliner is highlighted when visible
		if (!hovered && m_hasOutlinerHover)
		{
			IUIControl* outlinerHovered = m_outlinerHoverPath.Length() ? m_panel->Get(m_outlinerHoverPath) : m_panel;
			if (outlinerHovered && outlinerHovered->IsVisible())
				hovered = outlinerHovered;
		}

		DrawPreviewOverlay(origin, zoom, hovered);

		if (!fitToWindow)
			DrawPreviewRulers(origin, size, zoom, rulerSize);
	}

	ImGui::EndChild();
}

void CUIEditor::DrawLayoutPopup()
{
	if (!ImGui::BeginPopup("##controlLayout"))
		return;

	// path is kept because the controls are recreated after every change
	const SectionRef ref = FindSectionByPath(m_layoutPopupPath);
	IUIControl* control = m_layoutPopupPath.Length() ? m_panel->Get(m_layoutPopupPath) : m_panel;

	if (control)
		ImGui::Text("%s [%s]", control->GetName(), control->GetClassname());

	ImGui::Separator();

	if (ref.sec)
	{
		if (DrawLayoutProperties(*ref.sec))
			OnDocumentChanged(ref.doc);
	}
	else
	{
		ImGui::TextDisabled("Control has no layout section");
	}

	ImGui::EndPopup();
}

// position is the offset from the right and bottom sides with these alignments
void CUIEditor::MoveSelectedControl(const IVector2D& move)
{
	const IUIControl* control = GetSelectedControl();
	const SectionRef ref = FindSectionByPath(m_selection);
	if (!control || !ref.sec)
		return;

	const int alignment = control->GetAlignment();
	const IVector2D offset(
		(alignment & UI_ALIGN_RIGHT) ? -move.x : move.x,
		(alignment & UI_ALIGN_BOTTOM) ? -move.y : move.y);

	KVSection* key = ref.sec->FindSection("position");
	IVector2D position(0);
	if (key)
		key->GetValues(position);
	else
		key = &ref.sec->CreateSection("position");

	position += offset;
	key->ClearValues();
	key->AddValue(position.x);
	key->AddValue(position.y);

	OnDocumentChanged(ref.doc);
}

void CUIEditor::DrawPreviewRulers(const ImVec2& origin, const IVector2D& size, float zoom, const ImVec2& rulerSize)
{
	const ImVec2 displaySize(floorf(size.x * zoom), floorf(size.y * zoom));

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 backColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
	const ImU32 tickColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
	const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);

	drawList->AddRectFilled(ImVec2(origin.x - rulerSize.x, origin.y - rulerSize.y), ImVec2(origin.x + displaySize.x, origin.y), backColor);
	drawList->AddRectFilled(ImVec2(origin.x - rulerSize.x, origin.y), ImVec2(origin.x, origin.y + displaySize.y), backColor);

	// steps in preview pixels are chosen to keep ticks and labels apart on screen
	static const int s_rulerSteps[] = { 10, 50, 100, 200, 500, 1000 };
	const auto pickStep = [zoom](float minScreenDistance) {
		for (const int step : s_rulerSteps)
		{
			if (step * zoom >= minScreenDistance)
				return step;
		}
		return s_rulerSteps[elementsOf(s_rulerSteps) - 1];
	};

	const int tickStep = pickStep(5.0f);
	const int labelStep = max(pickStep(ImGui::CalcTextSize("0000").x + 8.0f), tickStep);

	const auto tickLength = [&](int pos, float rulerLength) {
		if (pos % labelStep == 0)
			return rulerLength;
		return pos % (tickStep * 5) == 0 ? rulerLength * 0.5f : rulerLength * 0.25f;
	};

	for (int x = 0; x <= size.x; x += tickStep)
	{
		const float tickX = origin.x + floorf(x * zoom);
		drawList->AddLine(ImVec2(tickX, origin.y), ImVec2(tickX, origin.y - tickLength(x, rulerSize.y)), tickColor);
		if (x % labelStep == 0)
			drawList->AddText(ImVec2(tickX + 2.0f, origin.y - rulerSize.y), textColor, EqString::Format("%d", x));
	}

	for (int y = 0; y <= size.y; y += tickStep)
	{
		const float tickY = origin.y + floorf(y * zoom);
		drawList->AddLine(ImVec2(origin.x, tickY), ImVec2(origin.x - tickLength(y, rulerSize.x), tickY), tickColor);
		if (y % labelStep == 0)
			drawList->AddText(ImVec2(origin.x - rulerSize.x + 2.0f, tickY + 1.0f), textColor, EqString::Format("%d", y));
	}

	// mouse position marks
	const ImVec2 mousePos = ImGui::GetMousePos();
	if (mousePos.x >= origin.x && mousePos.x <= origin.x + displaySize.x)
		drawList->AddLine(ImVec2(mousePos.x, origin.y), ImVec2(mousePos.x, origin.y - rulerSize.y), s_selectionColor);
	if (mousePos.y >= origin.y && mousePos.y <= origin.y + displaySize.y)
		drawList->AddLine(ImVec2(origin.x, mousePos.y), ImVec2(origin.x - rulerSize.x, mousePos.y), s_selectionColor);
}

void CUIEditor::DrawPreviewOverlay(const ImVec2& origin, float zoom, IUIControl* hovered)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// control transforms are not applied to the rectangles
	const auto drawControlRect = [&](const IUIControl* control, ImU32 color, float thickness) {
		const IAARectangle rect = control->GetClientRectangle();
		drawList->AddRect(
			ImVec2(origin.x + floorf(rect.leftTop.x * zoom), origin.y + floorf(rect.leftTop.y * zoom)),
			ImVec2(origin.x + floorf(rect.rightBottom.x * zoom), origin.y + floorf(rect.rightBottom.y * zoom)),
			color, 0.0f, 0, thickness);
	};

	if (m_showBounds)
	{
		const EqFunction<bool(IUIControl*)> drawBounds = [&](IUIControl* control) {
			if (!control->IsVisible())
				return true;
			drawControlRect(control, s_boundsColor, 1.0f);
			control->ForEachChild(drawBounds);
			return true;
		};
		m_panel->ForEachChild(drawBounds);
	}

	if (hovered)
		drawControlRect(hovered, s_hoverColor, 1.0f);

	const IUIControl* selected = GetSelectedControl();
	if (!selected)
		return;

	drawControlRect(selected, s_selectionColor, 2.0f);

	// layout values as written in the file, above the rectangle or below it near the top edge
	const IVector2D position = selected->GetPosition();
	const IVector2D size = selected->GetSize();
	const EqString label = EqString::Format("pos X: %d Y: %d  size W: %d H: %d", position.x, position.y, size.x, size.y);

	const IAARectangle rect = selected->GetClientRectangle();
	const ImVec2 textSize = ImGui::CalcTextSize(label);
	const float padding = 2.0f;

	ImVec2 labelPos(origin.x + floorf(rect.leftTop.x * zoom), origin.y + floorf(rect.leftTop.y * zoom) - textSize.y - padding * 2.0f);
	if (labelPos.y < ImGui::GetWindowPos().y)
		labelPos.y = origin.y + floorf(rect.rightBottom.y * zoom) + padding * 2.0f;

	drawList->AddRectFilled(ImVec2(labelPos.x - padding, labelPos.y - padding), ImVec2(labelPos.x + textSize.x + padding, labelPos.y + textSize.y + padding), IM_COL32(0, 0, 0, 200));
	drawList->AddText(labelPos, s_selectionColor, label);
}

void CUIEditor::DrawKeyValuesTab()
{
	LayoutDoc* kvDoc = GetKVDoc();

	// switching the file discards text changes
	ImGui::SetNextItemWidth(250.0f);
	if (ImGui::BeginCombo("File", kvDoc == m_mainDoc.Ptr() ? "(layout)" : kvDoc->filePath.ToCString()))
	{
		if (ImGui::Selectable("(layout)", kvDoc == m_mainDoc.Ptr()))
			m_kvDocIdx = -1;

		for (int i = 0; i < m_includeDocs.numElem(); ++i)
		{
			if (!m_includeDocs[i]->valid)
				continue;

			if (ImGui::Selectable(m_includeDocs[i]->filePath, kvDoc == m_includeDocs[i].Ptr()))
				m_kvDocIdx = i;
		}
		ImGui::EndCombo();

		if (GetKVDoc() != kvDoc)
		{
			m_kvTextEdited = false;
			m_kvTextDirty = true;
			UpdateKVText();
			kvDoc = GetKVDoc();
		}
	}

	ImGui::SameLine();
	ImGui::BeginDisabled(!m_kvTextEdited);
	if (ImGui::Button("Apply"))
	{
		KVSection document;
		if (KeyValues::ParseText(m_kvText.ptr(), CString::Length(m_kvText.ptr()), document, "EqUI editor"))
		{
			kvDoc->root.Clear();
			document.CopyTo(kvDoc->root);
			m_kvTextEdited = false;
			OnDocumentChanged(kvDoc);
		}
		else
		{
			MsgError("EqUI editor: can't parse layout text\n");
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Revert"))
	{
		m_kvTextEdited = false;
		m_kvTextDirty = true;
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	if (m_kvTextEdited && m_kvTextDirty)
		ImGui::TextColored(s_editorOnlyTextColor, "Layout was changed in the editor, Apply overwrites the changes");
	else
		ImGui::TextDisabled("Sections with '%s' are removed on save", EDITOR_ONLY_KEY);

	if (m_kvText.numElem() == 0)
		m_kvText.append(0);

	constexpr ImGuiInputTextFlags textFlags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize;
	if (ImGui::InputTextMultiline("##kvText", m_kvText.ptr(), m_kvText.numElem(), ImVec2(-FLT_MIN, -FLT_MIN), textFlags, KVTextResizeCb, &m_kvText))
		m_kvTextEdited = true;
}

//-------------------------------------------------------------------------------------------------

void CUIEditor::DrawInspector()
{
	ImGui::TextUnformatted("Inspector");
	ImGui::Separator();

	IUIControl* control = GetSelectedControl();
	if (!control)
	{
		ImGui::TextDisabled("Select a control in the outliner or the preview");
		return;
	}

	const SectionRef ref = FindSectionByPath(m_selection);
	KVSection* sec = ref.sec;
	const bool isPanel = control == m_panel;

	ImGui::Text("Class: %s", control->GetClassname());

	const IAARectangle rect = control->GetClientRectangle();
	ImGui::TextDisabled("Screen rect: %d %d - %d %d", rect.leftTop.x, rect.leftTop.y, rect.rightBottom.x, rect.rightBottom.y);

	if (!sec)
	{
		ImGui::Text("Name: %s", control->GetName());
		ImGui::TextWrapped("Control is created by its parent and has no layout section.");
		return;
	}

	if (ref.doc != m_mainDoc.Ptr())
	{
		ImGui::TextColored(s_includeTextColor, "File: %s%s", ref.doc->filePath, ref.doc->modified ? "*" : "");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Control is written in the included file, the file is saved with the layout");
	}

	if (ref.editorOnlyInclude)
		ImGui::TextColored(s_editorOnlyTextColor, "Included as editor-only, removed from the saved layout");

	if (!isPanel)
	{
		char nameBuffer[128];
		strncpy(nameBuffer, control->GetName(), sizeof(nameBuffer) - 1);
		nameBuffer[sizeof(nameBuffer) - 1] = 0;

		if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue) && nameBuffer[0])
		{
			sec->SetValue(nameBuffer, 1);

			Select(JoinPath(GetParentPath(m_selection), nameBuffer));
			OnDocumentChanged(ref.doc);
		}

		bool editorOnly = IsEditorOnly(*sec);
		if (ImGui::Checkbox("Editor only", &editorOnly))
		{
			if (editorOnly)
				sec->SetKey(EDITOR_ONLY_KEY, 1);
			else
				sec->RemoveSectionByName(EDITOR_ONLY_KEY);
			OnDocumentChanged(ref.doc);
		}

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Shown in the preview only, removed from the saved layout");
	}

	ImGui::Separator();

	// class is the first value of child sections
	const char* className = isPanel ? "panel" : sec->ValueAt(0).GetString();

	static Array<EqString> localFonts{ PP_SL };
	localFonts.clear();

	CollectLocalFonts(m_selection, localFonts);

	const PropertiesContext propsCtx{ localFonts };
	if (DrawControlProperties(*sec, className, propsCtx))
		OnDocumentChanged(ref.doc);

	DrawIncludes(ref);
}

// 'child file' entries of the control section
void CUIEditor::DrawIncludes(const SectionRef& ref)
{
	ImGui::Separator();
	ImGui::TextUnformatted("Included files");

	KVSection& sec = *ref.sec;
	int removeKeyIdx = -1;

	for (int i = 0; i < sec.KeyCount(); ++i)
	{
		KVSection& childSec = sec.KeyAt(i);
		if (CString::CompareCaseIns(childSec.GetName(), "child") || !IsIncludeSection(childSec))
			continue;

		ImGui::PushID(i);
		if (ImGui::SmallButton("x"))
			removeKeyIdx = i;

		ImGui::SameLine();
		const char* includePath = GetChildSectionName(childSec);
		const int includeIdx = arrayFindIndexF(m_includeDocs, [&](const LayoutDocPtr& doc) {
			return !doc->filePath.CompareCaseIns(includePath);
		});

		bool includeEditorOnly = IsEditorOnly(childSec);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
		if (ImGui::Checkbox("##editorOnly", &includeEditorOnly))
		{
			if (includeEditorOnly)
				childSec.SetKey(EDITOR_ONLY_KEY, 1);
			else
				childSec.RemoveSectionByName(EDITOR_ONLY_KEY);
			OnDocumentChanged(ref.doc);
		}
		ImGui::PopStyleVar();

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Editor only: shown in the preview, removed on save");

		ImGui::SameLine();
		ImGui::BeginDisabled(includeIdx == -1 || !m_includeDocs[includeIdx]->valid);
		if (ImGui::SmallButton("KV"))
		{
			m_kvDocIdx = includeIdx;
			m_kvTextEdited = false;
			m_kvTextDirty = true;
			m_selectKVTab = true;
		}
		ImGui::EndDisabled();

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Show the file in the KeyValues tab");

		ImGui::SameLine();
		ImGui::BeginDisabled(includeIdx == -1);
		if (ImGui::SmallButton("R"))
			ReloadIncludeDoc(*m_includeDocs[includeIdx]);
		ImGui::EndDisabled();

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Reload the file from disk, its unsaved changes are lost");

		ImGui::SameLine();
		char pathBuffer[256];
		strncpy(pathBuffer, includePath, sizeof(pathBuffer) - 1);
		pathBuffer[sizeof(pathBuffer) - 1] = 0;

		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::InputText("##includePath", pathBuffer, sizeof(pathBuffer), ImGuiInputTextFlags_EnterReturnsTrue) && pathBuffer[0])
		{
			childSec.SetValue(pathBuffer, 1);
			OnDocumentChanged(ref.doc);
		}
		ImGui::PopID();
	}

	if (removeKeyIdx != -1)
	{
		sec.RemoveSection(&sec.KeyAt(removeKeyIdx));
		OnDocumentChanged(ref.doc);
	}

	char newPath[256]{};
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::InputTextWithHint("##addInclude", "Add file, e.g. resources/ui_partial/menu_fonts.res", newPath, sizeof(newPath), ImGuiInputTextFlags_EnterReturnsTrue) && newPath[0])
		AddIncludeSection(m_selection, newPath, false);
}

}

#endif // EQUI_EDITOR

//-------------------------------------------------------------------------------------------------

void equiEditorUIDraw(bool& open)
{
#ifdef EQUI_EDITOR
	equi::Editor::s_uiEditor->Draw(open);
#endif
}

bool equiEditorOpenFile(const char* filePath)
{
#ifdef EQUI_EDITOR
	return equi::Editor::s_uiEditor->LoadDocument(filePath);
#else
	return false;
#endif
}

void equiEditorShutdown()
{
#ifdef EQUI_EDITOR
	equi::Editor::s_uiEditor->Shutdown();
#endif
}
