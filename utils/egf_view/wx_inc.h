//-=-=-=-=-=-=-=-=-=-= Copyright (C) Damage Byte L.L.C 2009-2012 =-=-=-=-=-=-=-=-=-=-=-
//
// Description: wxWidgets includes
//
//-=-=-=-D-=-A-=-M-=-A-=-G-=-E-=-=-B-=-Y-=-T-=-E-=-=-S-=-T-=-U-=-D-=-I-=-O-=-S-=-=-=-=-

#ifndef WX_INC_H
#define WX_INC_H

#pragma warning(disable: 4003)

// for all others, include the necessary headers
#ifndef WX_PRECOMP
	#include "wx/app.h"
	#include "wx/log.h"
	#include "wx/frame.h"
	#include "wx/menu.h"

	#include "wx/button.h"
	#include "wx/checkbox.h"
	#include "wx/combobox.h"
	#include "wx/listbox.h"
	#include "wx/listctrl.h"
	#include "wx/statbox.h"
	#include "wx/stattext.h"
	#include "wx/textctrl.h"
	#include "wx/msgdlg.h"
	#include "wx/toolbar.h"
	#include "wx/minifram.h"
	#include "wx/mdi.h"
	#include "wx/docview.h"
	#include "wx/splitter.h"
	#include "wx/sashwin.h"
	#include "wx/filedlg.h"
	#include "wx/aui/aui.h"
	#include "wx/dialog.h"
	#include "wx/spinctrl.h"
	#include "wx/slider.h"
	#include "wx/radiobox.h"
	#include "wx/radiobut.h"
	#include "wx/dc.h"
	#include "wx/pen.h"
	#include "wx/brush.h"
	#include "wx/dcclient.h"
	#include "wx/dataview.h"
#endif

#include "wx/sysopt.h"
#include "wx/bookctrl.h"
#include "wx/sizer.h"
#include "wx/colordlg.h"
#include "wx/fontdlg.h"
#include "wx/textdlg.h"

#endif // WX_INC_H