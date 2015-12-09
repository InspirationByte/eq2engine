//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: 
//
//****************************************************************************

#include "Dialog.h"

Dialog::Dialog(const float x, const float y, const float w, const float h, const bool modal, const bool hideOnClose)
{
	setPosition(x, y);
	setSize(w, h);

	borderWidth = 2;
	tabHeight = 32;

	color = Vector4D(0.2f, 0.2f, 0.2f, 1.0f);

	draging = false;
	closeModeHide = hideOnClose;
	showSelection = false;
	isModal = modal;
	currTab = 0;

	closeButton = new PushButton(x + w - 2 * borderWidth - 24, y + 2 * borderWidth, 24, 24, "X");
	closeButton->setListener(this);
}

Dialog::~Dialog(){
	for (int i = 0; i < tabs.numElem(); i++){
		DialogTab *tab = tabs[i];
		if (tab->Panels.goToFirst()){
			do {
				delete tab->Panels.getCurrent().Panel;
			} while (tab->Panels.goToNext());
		}
		delete tab->caption;
		delete tab;
	}

	delete closeButton;
}

int Dialog::addTab(const char *caption){
	DialogTab *tab = new DialogTab;
	tab->caption = new char[strlen(caption) + 1];
	strcpy(tab->caption, caption);
	return tabs.add(tab);
}

void Dialog::addPanel(const int tab, Panel *panel, const uint flags){
	WInfo wInfo;
	wInfo.Panel = panel;
	wInfo.x = panel->getX();
	wInfo.y = panel->getY();

	panel->setPosition(xPos + wInfo.x + 2 * borderWidth, yPos + wInfo.y + 2 * borderWidth + tabHeight);
	tabs[tab]->Panels.addFirst(wInfo);
}

bool Dialog::onMouseMove(const int x, const int y){
	if (currTab < tabs.getCount()){
		if (draging){
			float dx = float(x - sx);
			float dy = float(y - sy);

			xPos += dx;
			yPos += dy;

			for (uint i = 0; i < tabs.getCount(); i++){
				DialogTab *tab = tabs[i];

				if (tab->Panels.goToFirst()){
					do {
						WInfo wi = tab->Panels.getCurrent();
						wi.Panel->setPosition(xPos + wi.x + 2 * borderWidth, yPos + wi.y + 2 * borderWidth + tabHeight);
					} while (tab->Panels.goToNext());
				}
			}
			closeButton->setPosition(xPos + width - 2 * borderWidth - 24, yPos + 2 * borderWidth);

			sx = x;
			sy = y;
		} else {
			DialogTab *tab = tabs[currTab];

			if (tab->Panels.goToFirst()){
				do {
					Panel *pPanel = tab->Panels.getCurrent().Panel;
					if (pPanel->isEnabled() && pPanel->isVisible() && (pPanel->isInPanel(x, y) || pPanel->isCapturing())){
						if (pPanel->onMouseMove(x, y)){
							tab->Panels.moveCurrentToTop();
							capture = isModal || pPanel->isCapturing();
							return true;
						}
					}
				} while (tab->Panels.goToNext());
			}

		}
	}

	return true;
}

bool Dialog::onMouseButton(const int x, const int y, const MouseButton button, const bool pressed){
	showSelection = false;

	if (closeButton->isCapturing() || closeButton->isInPanel(x, y)){
		closeButton->onMouseButton(x, y, button, pressed);
		capture = true;
		return true;
	}

	if (currTab < tabs.getCount()){
		DialogTab *tab = tabs[currTab];

		if (tab->Panels.goToFirst()){
			do {
				Panel *Panel = tab->Panels.getCurrent().Panel;
				if (Panel->isEnabled() && Panel->isVisible() && (Panel->isInPanel(x, y) || Panel->isCapturing())){
					if (Panel->onMouseButton(x, y, button, pressed)){
						tab->Panels.moveCurrentToTop();
						capture = isModal || Panel->isCapturing();
						return true;
					}
				}
			} while (tab->Panels.goToNext());
		}
	}

	if (button == MOUSE_LEFT){
		capture = isModal || pressed;
		if (isInPanel(x, y)){
			draging = pressed;
			sx = x;
			sy = y;

			if (pressed){
				if (x > xPos + 2 * borderWidth && y > yPos + 2 * borderWidth && y < yPos + 2 * borderWidth + tabHeight){
					for (uint i = 0; i < tabs.getCount(); i++){
						if (x < tabs[i]->rightX){
							currTab = i;
							draging = false;
							break;
						}
					}
				}
			}
		}
	}

	return true;
}

bool Dialog::onMouseWheel(const int x, const int y, const int scroll){
	if (currTab < tabs.getCount()){
		DialogTab *tab = tabs[currTab];

		if (tab->Panels.goToFirst()){
			do {
				Panel *Panel = tab->Panels.getCurrent().Panel;
				if (Panel->isEnabled() && Panel->isVisible() && (Panel->isInPanel(x, y) || Panel->isCapturing())){
					if (Panel->onMouseWheel(x, y, scroll)){
						tab->Panels.moveCurrentToTop();
						capture = isModal || Panel->isCapturing();
						return true;
					}
				}
			} while (tab->Panels.goToNext());
		}
	}

	return true;
}

bool Dialog::onKey(const unsigned int key, const bool pressed){
	if (currTab < tabs.getCount()){
		DialogTab *tab = tabs[currTab];

		if (tab->Panels.goToFirst()){
			if (tab->Panels.getCurrent().Panel->onKey(key, pressed)) return true;
		}
		if (pressed){
			if (key == KEY_ESCAPE){
				close();
				return true;
			} else if (key == KEY_TAB){
				if (tab->Panels.goToFirst()){
					Panel *currTop = tab->Panels.getCurrent().Panel;

					tab->Panels.goToLast();
					do {
						Panel *Panel = tab->Panels.getCurrent().Panel;
						if (Panel->isEnabled()){
							tab->Panels.moveCurrentToTop();

							currTop->onFocus(false);
							Panel->onFocus(true);

							showSelection = true;
							break;
						}
					} while (tab->Panels.goToPrev());
				}
				return true;
			}
			showSelection = false;
		}
	}

	return false;
}

bool Dialog::onJoystickAxis(const int axis, const float value){
	if (currTab < tabs.getCount()){
		DialogTab *tab = tabs[currTab];

		if (tab->Panels.goToFirst()){
			if (tab->Panels.getCurrent().Panel->onJoystickAxis(axis, value)) return true;
		}
	}

	return false;
}

bool Dialog::onJoystickButton(const int button, const bool pressed){
	if (currTab < tabs.getCount()){
		DialogTab *tab = tabs[currTab];

		if (tab->Panels.goToFirst()){
			if (tab->Panels.getCurrent().Panel->onJoystickButton(button, pressed)) return true;
		}
	}

	return false;
}

void Dialog::onButtonClicked(PushButton *button){
	close();
}

void Dialog::draw(IRenderer *renderer, const FontID defaultFont, const SamplerStateID linearClamp, const BlendStateID blendSrcAlpha, const DepthStateID depthState){
	
	//drawSoftBorderQuad(renderer, linearClamp, blendSrcAlpha, depthState, xPos, yPos, xPos + width, yPos + height, borderWidth, 1, 1);

	renderer->reset();
	renderer->setBlendState(blendSrcAlpha);
	renderer->setDepthState(depthState);
	renderer->apply();

	renderer->drawFilledRectangle(Vector2D(xPos,yPos),Vector2D(xPos + width, yPos + height),color);
	drawShadedRectangle(renderer,Vector2D(xPos,yPos),Vector2D(xPos + width, yPos + height),color*2,color*0.3f);

	Vector4D black(0, 0, 0, 1);
	Vector4D blue(0.3f, 0.4f, 1.0f, 0.65f);

	float x = xPos + 2 * borderWidth;
	float y = yPos + 2 * borderWidth;

	for (uint i = 0; i < tabs.getCount(); i++)
	{
		float tabWidth = 0.75f * tabHeight;
		float cw = renderer->getTextWidth(defaultFont, tabs[i]->caption);
		float newX = x + tabWidth * cw + 6;

		if (i == currTab)
		{
			Vector2D quad[] = { MAKEQUAD(x, y, newX, y + tabHeight, 2) };
			renderer->drawPlain(PRIM_TRIANGLE_STRIP, quad, elementsOf(quad), blendSrcAlpha, depthState, &(color * 2));
		}

		Vector2D rect[] = { MAKERECT(x, y, newX, y + tabHeight, 1) };
		renderer->drawPlain(PRIM_TRIANGLE_STRIP, rect, elementsOf(rect), BS_NONE, depthState, &black);

		renderer->drawText(tabs[i]->caption, x + 3, y, tabWidth, tabHeight, defaultFont, linearClamp, blendSrcAlpha, depthState);

		tabs[i]->rightX = x = newX;
	}

	//Vector2D line[] = { MAKEQUAD(xPos + 2 * borderWidth, y + tabHeight - 1, xPos + width - 2 * borderWidth, y + tabHeight + 1, 0) };
	//renderer->drawPlain(PRIM_TRIANGLE_STRIP, line, elementsOf(line), BS_NONE, depthState, &black);

	closeButton->draw(renderer, defaultFont, linearClamp, blendSrcAlpha, depthState);

	if (currTab < tabs.getCount()){
		DialogTab *tab = tabs[currTab];

		if (tab->Panels.goToLast())
		{
			do 
			{
				Panel *Panel = tab->Panels.getCurrent().Panel;
				if (Panel->isVisible()) Panel->draw(renderer, defaultFont, linearClamp, blendSrcAlpha, depthState);
			} while (tab->Panels.goToPrev());
		}
		if (showSelection)
		{
			if (tab->Panels.goToFirst())
			{
				Panel *w = tab->Panels.getCurrent().Panel;

				float x = w->getX();
				float y = w->getY();
				Vector2D rect[] = { MAKERECT(x - 5, y - 5, x + w->getWidth() + 5, y + w->getHeight() + 5, 1) };
				renderer->drawPlain(PRIM_TRIANGLE_STRIP, rect, elementsOf(rect), BS_NONE, depthState, &black);
			}
		}
	}
}

void Dialog::close()
{
	if (closeModeHide)
	{
		visible = false;
		showSelection = false;
	} 
	else 
	{
		dead = true;
	}
	capture = false;
}
