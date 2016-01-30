//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor previewable object
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORPREVIEWABLE_H
#define EDITORPREVIEWABLE_H

class ITexture;

class CEditorPreviewable
{
public:
	CEditorPreviewable();
	virtual ~CEditorPreviewable();

	virtual void	RefreshPreview() = 0;
	void			SetDirtyPreview();
	bool			IsDirtyPreview() const;

	bool			CreatePreview(int texSize);

	ITexture*		GetPreview() const;
protected:
	ITexture*	m_preview;
	bool		m_dirtyPreview;
};

#endif // EDITORPREVIEWABLE_H