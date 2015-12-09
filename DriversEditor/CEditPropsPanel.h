#include "EditorHeader.h"

class CEditPropsPanel : public wxPanel 
{
public:
		
	CEditPropsPanel( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 548,145 ), long style = wxTAB_TRAVERSAL ); 
	~CEditPropsPanel();

	int				GetRotation();
	int				GetRadius();
	int				GetHeightfieldFlags();
	int				GetAddHeight();


	EEditMode		GetEditMode();

protected:
	enum
	{
		SETROT_0 = 1000,
		SETROT_270,
		SETROT_90,
		SETROT_180
	};
		
	wxRadioBox*		m_mode;

	wxCheckBox*		m_detached;
	wxCheckBox*		m_addWall;
	wxCheckBox*		m_wallCollide;
	wxCheckBox*		m_noCollide;

	wxSlider*		m_addheight;
	wxSlider*		m_radius;

	int				m_rotation;
		
	// Virtual event handlers, overide them in your derived class
	void OnClickRotation( wxCommandEvent& event );
};