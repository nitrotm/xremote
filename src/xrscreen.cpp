/**
 * Xremote - X screen wrapper
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "xremote.h"
#include "xrscreen.h"


XRSCREEN::XRSCREEN() : display(NULL), screen(NULL), index(0), width(0), height(0) {
}

XRSCREEN::XRSCREEN(Display *display, int index) : display(display), index(index) {
	this->screen = XScreenOfDisplay(this->display, this->index);
	this->width = XDisplayWidth(this->display, this->index);
	this->height = XDisplayHeight(this->display, this->index);
}

XRSCREEN::XRSCREEN(const XRSCREEN &ref) : display(ref.display), screen(ref.screen), index(ref.index), width(ref.width), height(ref.height) {
}

XRSCREEN::~XRSCREEN() {
}


int XRSCREEN::getIndex() const {
	return this->index;
}

int XRSCREEN::getWidth() const {
	return this->width;
}

int XRSCREEN::getHeight() const {
	return this->height;
}

Window XRSCREEN::getRootWindow() const {
	return XRootWindowOfScreen(this->screen);
}

Visual* XRSCREEN::getDefaultVisual() const {
	return XDefaultVisualOfScreen(this->screen);
}


bool XRSCREEN::centerPointer() {
	int cx = this->width / 2;
	int cy = this->height / 2;

	return this->setPointer(cx, cy);
}

bool XRSCREEN::getPointer(Window *child, int *x, int *y) const {
	unsigned int mask = 0;
	Window root = None;
	int wndX = 0;
	int wndY = 0;

	if (XQueryPointer(
						this->display,
						this->getRootWindow(),
						&root,
						child,
						x,
						y,
						&wndX,
						&wndY,
						&mask
					) == True) {
		return true;
	}
	return false;
}

bool XRSCREEN::setPointer(int x, int y) const {
	XWarpPointer(
		this->display,
		None,
		this->getRootWindow(),
		0,
		0,
		0,
		0,
		x,
		y
	);
	this->flush();
	return true;
}


void XRSCREEN::flush() const {
	XFlush(
		this->display
	);
}
