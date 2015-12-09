//**************** Copyright (C) Parallel Prevision, L.L.C 2012 *****************
//
// Description: Edtior render window
//
//***************************************************************************

#include "edit_renderwindow.h"

#include "in_keys_ident.h"

class CConsoleWindow;

mxWindow *g_pConsoleWindow = NULL;

#define IDB_SEND_COMMAND 11001

class CConsoleWindow : public mxWindow
{
public:
	CConsoleWindow(mxWindow *parent, int x, int y, int w, int h, const char *label = 0, int style = 0) : mxWindow(parent,x,y,w,h,label,style)
	{
		console_messages = new mxListBox(this, 0,0,w,h,-1, mxListBox::Normal);
		this->setEnabled(true);
		this->setVisible(true);

		console_edit_frame = new mxLineEdit(this, 0, h - 5, w - 100, h);
		send_command_button = new mxButton(this, w-90,h-5,w-5,h,"Send", IDB_SEND_COMMAND);

		OnResize();
	}

	void OnResize()
	{
		RECT wRect;
		GetClientRect((HWND)this->getHandle(), &wRect);

		console_messages->setBounds(0,0,wRect.right,wRect.bottom - 25);

		console_edit_frame->setBounds(0,wRect.bottom-20,wRect.right-55,wRect.bottom);
		send_command_button->setBounds(wRect.right-50,wRect.bottom-20,50, 20);
	}

	void ExecuteCommand()
	{
		GetCommandAccessor()->SetCommandBuffer((char*)console_edit_frame->getLabel());
		GetCommandAccessor()->ExecuteCommandBuffer();
		GetCommandAccessor()->ClearCommandBuffer();
	}

	int handleEvent(mxEvent *event)
	{
		switch(event->event)
		{
			case mxEvent::KeyDown:
			{
				switch(event->key)
				{
					case KEY_ENTER:
					{
						ExecuteCommand();
					}
					break;
				}
				break;
			}
			case mxEvent::Size:
			{
				if(console_messages)
				{
					OnResize();
				}
				break;
			}
			case mxEvent::Action:
			{
				switch(event->action)
				{
					case IDB_SEND_COMMAND:
					{
						ExecuteCommand();
					}
					break;
				}
				break;
			}
		}

		return 0;
	}
	
	mxListBox*	console_messages;
	mxLineEdit* console_edit_frame;
	mxButton*	send_command_button;
};

void EngineSpewFunc(SpewType_t type,char* pMsg);

void CreateConsoleWindow(mxWindow *parent)
{
	g_pConsoleWindow = new CConsoleWindow(parent, 40,40,512,256,"Editor Console", mxWindow::Normal);

	g_pConsoleWindow->setVisible(true);
	g_pConsoleWindow->setEnabled(true);

	mxSetWindowExStyle(g_pConsoleWindow, WS_EX_TOPMOST, true);
	mxSetWindowExStyle(g_pConsoleWindow, WS_EX_TOOLWINDOW, true);

	/*
	mxSetWindowStyle(g_pConsoleWindow, WS_CHILD, false);
	mxSetWindowStyle(g_pConsoleWindow, WS_CHILDWINDOW, true);
	mxSetWindowStyle(g_pConsoleWindow, WS_CLIPCHILDREN, true);
	mxSetWindowStyle(g_pConsoleWindow, WS_CLIPSIBLINGS, true);
	mxSetWindowStyle(g_pConsoleWindow, WS_OVERLAPPEDWINDOW, true);
	*/
	
}

void InstallSpewFunction()
{
	SetSpewFunction(EngineSpewFunc);
}

void EngineSpewFunc(SpewType_t type,char* pMsg)
{
	printf("%s", pMsg );
	OutputDebugString(pMsg);

	DkList<DkStr> temp;
	xstrsplit(pMsg,"\n",temp);

	for(int i = 0; i < temp.numElem(); i++)
	{
		if(g_pConsoleWindow)
		{
			((CConsoleWindow*)g_pConsoleWindow)->console_messages->add(temp[i].getData());
		}
	}
}