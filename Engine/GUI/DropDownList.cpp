//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: 
//
//****************************************************************************

#include "DropDownList.h"

DropDownList::DropDownList(const float x, const float y, const float w, const float h){
	setPosition(x, y);
	setSize(w, h);

	color = Vector4D(1, 1, 0, 0.65f);

	dropDownListener = NULL;

	selectedItem = -1;
	isDroppedDown = false;
}

DropDownList::~DropDownList(){
	clear();
}

int DropDownList::addItem(const char *str){
	char *string = new char[strlen(str) + 1];
	strcpy(string, str);

	return items.append(string);
}

int DropDownList::addItemUnique(const char *str)
{
	for (int i = 0; i < items.numElem(); i++)
	{
		if (strcmp(str, items[i]) == 0){
			return i;
		}
	}
	return addItem(str);
}

void DropDownList::selectItem(const int item){
	selectedItem = item;
	if (dropDownListener) dropDownListener->onDropDownChanged(this);
}

int DropDownList::getItem(const char *str) const {
	for (int i = 0; i < items.numElem(); i++){
		if (strcmp(str, items[i]) == 0){
			return i;
		}
	}

	return -1;
}

void DropDownList::clear()
{
	for (int i = 0; i < items.numElem(); i++)
	{
		delete items[i];
	}
	items.clear();
}

int comp(char * const &elem0, char * const &elem1)
{
	return strcmp(elem0, elem1);
}

void DropDownList::sort()
{
/*
	char *curr = NULL;
	if (selectedItem >= 0) curr = items[selectedItem];

	items.sort(comp);

	if (selectedItem >= 0)
	{
		for (int i = 0; i < items.getCount(); i++)
		{
			if (strcmp(curr, items[i]) == 0)
			{
				selectedItem = i;
				break;
			}
		}
	}
	*/
}

bool DropDownList::onMouseButton(const int x, const int y, const MouseButton button, const bool pressed){
	if (button == MOUSE_LEFT){
		if (pressed){
			if (x > xPos + width - height && x < xPos + width && y > yPos && y < yPos + height){
				if (y < yPos + 0.5f * height){
					if (selectedItem > 0) selectItem(selectedItem - 1);
				} else {
					if (selectedItem + 1 < (int) items.numElem()) selectItem(selectedItem + 1);
				}
			} else {
				if (isDroppedDown){
					int item = int((y - yPos) / height + selectedItem);
					if (item >= 0 && item < (int) items.numElem()) selectItem(item);
				}
				isDroppedDown = !isDroppedDown;
			}
		}
		capture = isDroppedDown;

		return true;
	}

	return false;
}

bool DropDownList::onMouseWheel(const int x, const int y, const int scroll){
	selectedItem -= scroll;

	int count = items.numElem();

	if (selectedItem >= count){
		selectedItem = count - 1;
	} else if (selectedItem < 0){
		selectedItem = 0;
	}

	return true;
}

bool DropDownList::onKey(const unsigned int key, const bool pressed){
	if (pressed){
		switch (key){
		case KEY_UP:
			if (selectedItem > 0) selectItem(selectedItem - 1);
			return true;
		case KEY_DOWN:
			if (selectedItem + 1 < (int) items.numElem()) selectItem(selectedItem + 1);
			return true;
		case KEY_ENTER:
		case KEY_SPACE:
			isDroppedDown = !isDroppedDown;
			return true;
		case KEY_ESCAPE:
			if (!isDroppedDown) return false;
			capture = isDroppedDown = false;
			return true;
		}
	}

	return false;
}

void DropDownList::onFocus(const bool focus){
	capture = isDroppedDown = false;
}

void DropDownList::draw(IFont* defaultFont)
{
	Vector4D col = enabled? color : Vector4D(color.xyz() * 0.5f, 1);
	Vector4D black(0, 0, 0, 1);

	Vertex2D_t quad[] = { MAKETEXQUAD(xPos, yPos, xPos + width, yPos + height, 2) };

	g_pShaderAPI->DrawSetColor(col);
	g_pShaderAPI->DrawPrimitives2D(PRIM_TRIANGLE_STRIP, quad, elementsOf(quad));

	Vertex2D_t rect[] = { MAKETEXQUAD(xPos, yPos, xPos + width, yPos + height, 2) };

	g_pShaderAPI->DrawSetColor(black);
	g_pShaderAPI->DrawPrimitives2D(PRIM_TRIANGLE_STRIP, rect, elementsOf(rect));

	Vertex2D_t line0[] = { MAKETEXQUAD(xPos + width - height, yPos + 2, xPos + width - height + 2, yPos + height - 2, 0) };

	g_pShaderAPI->DrawSetColor(black);
	g_pShaderAPI->DrawPrimitives2D(PRIM_TRIANGLE_STRIP, line0, elementsOf(line0));

	Vertex2D_t line1[] = { MAKETEXQUAD(xPos + width - height + 1, yPos + 0.5f * height - 1, xPos + width - 2, yPos + 0.5f * height + 1, 0) };
	
	g_pShaderAPI->DrawSetColor(black);
	g_pShaderAPI->DrawPrimitives2D(PRIM_TRIANGLE_STRIP, line1, elementsOf(line1));

	Vertex2D_t triangles[] = {
		Vertex2D_t(Vector2D(xPos + width - 0.5f * height, yPos + 0.1f * height),vec2_zero),
		Vertex2D_t(Vector2D(xPos + width - 0.2f * height, yPos + 0.4f * height),vec2_zero),
		Vertex2D_t(Vector2D(xPos + width - 0.8f * height, yPos + 0.4f * height),vec2_zero),
		Vertex2D_t(Vector2D(xPos + width - 0.5f * height, yPos + 0.9f * height),vec2_zero),
		Vertex2D_t(Vector2D(xPos + width - 0.8f * height, yPos + 0.6f * height),vec2_zero),
		Vertex2D_t(Vector2D(xPos + width - 0.2f * height, yPos + 0.6f * height),vec2_zero),
	};

	g_pShaderAPI->DrawSetColor(black);
	g_pShaderAPI->DrawPrimitives2D(PRIM_TRIANGLES, triangles, elementsOf(triangles));

	float textWidth = 0.75f * height;
	float w = width - 1.3f * height;
	if (selectedItem >= 0)
	{
		float tw = defaultFont->GetStringLength(items[selectedItem],strlen(items[selectedItem]));
		float maxW = w / tw;
		if (textWidth > maxW) textWidth = maxW;

		defaultFont->DrawText(items[selectedItem], xPos + 0.15f * height, yPos, textWidth, height);
	}

	if (isDroppedDown){
		Vertex2D_t quad[] = { MAKETEXQUAD(xPos, yPos - selectedItem * height, xPos + width - height + 2, yPos + (items.numElem() - selectedItem) * height, 2) };
		g_pShaderAPI->DrawSetColor(col);
		g_pShaderAPI->DrawPrimitives2D(PRIM_TRIANGLE_STRIP, quad, elementsOf(quad));

		Vertex2D_t rect[] = { MAKETEXRECT(xPos, yPos - selectedItem * height, xPos + width - height + 2, yPos + (items.numElem() - selectedItem) * height, 2) };
		g_pShaderAPI->DrawSetColor(black);
		g_pShaderAPI->DrawPrimitives2D(PRIM_TRIANGLE_STRIP, rect, elementsOf(rect));

		for (int i = 0; i < items.numElem(); i++){
			float tw = defaultFont->GetStringLength(items[i],strlen(items[i]));
			float maxW = w / tw;
			if (textWidth > maxW) textWidth = maxW;

			defaultFont->DrawText(items[i], xPos + 0.15f * height, yPos + (int(i) - selectedItem) * height, textWidth, height);
		}		
	}

}
