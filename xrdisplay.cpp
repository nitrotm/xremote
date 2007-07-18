/**
 * Xremote - X display wrapper
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "xremote.h"


XRDISPLAY::XRDISPLAY(const string &displayName) : displayName(displayName), screenCount(0) {
	this->display = XOpenDisplay(this->displayName.c_str());
	if (this->display != NULL) {
		this->screenCount = XScreenCount(this->display);
		for (int i = 0; i < this->screenCount; i++) {
			this->screens.push_back(XRSCREEN(this->display, i));
		}
	}
}

XRDISPLAY::XRDISPLAY(Display *display) : display(display) {
	this->displayName = XDisplayString(this->display);
	this->screenCount = XScreenCount(this->display);
	for (int i = 0; i < this->screenCount; i++) {
		this->screens.push_back(XRSCREEN(this->display, i));
	}
}

XRDISPLAY::XRDISPLAY(const XRDISPLAY &ref) : display(ref.display), displayName(ref.displayName), screens(ref.screens), screenCount(ref.screenCount) {
}

XRDISPLAY::~XRDISPLAY() {
	if (this->display != NULL) {
		XCloseDisplay(this->display);
	}
}


string XRDISPLAY::getDisplayName() const {
	return this->displayName;
}


int XRDISPLAY::getScreenCount() const {
	return this->screenCount;
}

XRSCREEN XRDISPLAY::getScreen(int screenIndex) const {
	if (screenIndex >= 0 && screenIndex < this->screenCount) {
		return this->screens[screenIndex];
	}
	return XRSCREEN();
}

XRSCREEN XRDISPLAY::getCurrentScreen() const {
	for (int i = 0; i < this->screenCount; i++) {
		Window child = None;
		int x = 0;
		int y = 0;

		if (this->screens[i].getPointer(&child, &x, &y)) {
			return screens[i];
		}
	}
	return XRSCREEN();
}


void XRDISPLAY::flush() const {
	XFlush(
		this->display
	);
}

string XRDISPLAY::getCutBuffer() const {
	int size = 0;
	char *buffer = XFetchBytes(this->display, &size);
	string str(buffer);

	XFree(buffer);
	return str;
}

void XRDISPLAY::setCutBuffer(const string &value) {
	XStoreBytes(this->display, value.c_str(), value.length());
}
