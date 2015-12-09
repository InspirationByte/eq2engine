//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: 
//
//****************************************************************************

#ifndef _DROPDOWNLIST_H_
#define _DROPDOWNLIST_H_

#include "Panel.h"

class DropDownList;
class DropDownListener {
public:
	virtual ~DropDownListener(){}

	virtual void onDropDownChanged(DropDownList *dropDownList) = 0;
};

class DropDownList : public Panel {
public:
	DropDownList(const float x, const float y, const float w, const float h);
	virtual ~DropDownList();

	int addItem(const char *str);
	int addItemUnique(const char *str);
	void selectItem(const int item);
	int getItem(const char *str) const;
	const char *getText(const int index) const { return items[index]; }
	const char *getSelectedText() const { return items[selectedItem]; }
	int getSelectedItem() const { return selectedItem; }
	void sort();
	void clear();


	void setListener(DropDownListener *listener){ dropDownListener = listener; }

	bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	bool onMouseWheel(const int x, const int y, const int scroll);
	bool onKey(const unsigned int key, const bool pressed);
	void onFocus(const bool focus);

	void draw(IFont* defaultFont);

protected:
	DkList <char *> items;
	int selectedItem;

	DropDownListener *dropDownListener;

	bool isDroppedDown;
};

#endif // _DROPDOWNLIST_H_
