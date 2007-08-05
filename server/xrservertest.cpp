/**
 * Xremote - server-side (Xtest extension)
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "../xremote.h"
#include "xrservercon.h"
#include "xrserver.h"
#include "xrservertest.h"


XRSERVERTEST::XRSERVERTEST(const string &localHost, in_port_t localPort, const string &displayName) : XRSERVER(localHost, localPort, displayName), supported(false) {
	int eventBase = 0;
	int errorBase = 0;
	int majorVersion = 0;
	int minorVersion = 0;

	if (XTestQueryExtension(
				this->display,
				&eventBase,
				&errorBase,
				&majorVersion,
				&minorVersion
			) == True) {
		this->supported = true;
	} else {
		this->supported = false;
	}
}

XRSERVERTEST::XRSERVERTEST(const XRSERVERTEST &ref) : XRSERVER((const XRSERVER &)ref), supported(ref.supported) {
}

XRSERVERTEST::~XRSERVERTEST() {
}


bool XRSERVERTEST::isSupported() const {
	return this->supported;
}


bool XRSERVERTEST::processPtrEvent(PXRPTREVENT event) {
	if (!this->supported) {
		return false;
	}

	// set the pointer at the right position
	XRSCREEN screen(this->getCurrentScreen());
	Window wnd;
	int x = 0;
	int y = 0;
	int w = screen.getWidth();
	int h = screen.getHeight();

	if (screen.getPointer(&wnd, &x, &y)) {
		if ((x + event->getX()) < 0) {
			// move to previous screen if any
			x += event->getX();
			y += event->getY();
			if (y < 0) {
				y = 0;
			} else if (y >= h) {
				y = h - 1;
			}
			if (screen.getIndex() > 0) {
				screen = this->getScreen(screen.getIndex() - 1);
				return screen.setPointer(screen.getWidth() - x, y);
			}
			return this->sendEnd(y, XRNOTIFY_LEFT);
		} else if ((x + event->getX()) >= w) {
			// move to next screen if any
			x += event->getX();
			y += event->getY();
			if (y < 0) {
				y = 0;
			} else if (y >= h) {
				y = h - 1;
			}
			if (screen.getIndex() < (this->getScreenCount() - 1)) {
				screen = this->getScreen(screen.getIndex() + 1);
				return screen.setPointer(x - w, y);
			}
			return this->sendEnd(y, XRNOTIFY_RIGHT);
		}
	}

//	printf("ptr: x=%d, y=%d\n", event->getX(), event->getY());
	XTestFakeRelativeMotionEvent(
		this->display,
		event->getX(),
		event->getY(),
		CurrentTime
	);
	this->flush();
	return true;
}

bool XRSERVERTEST::processButtonEvent(PXRPTREVENT event, bool down) {
	if (!this->supported) {
		return false;
	}
	if (down) {
//		printf("button: id=%d, down\n", event->getButton());
		XTestFakeButtonEvent(
			this->display,
			event->getButton(),
			True,
			CurrentTime
		);
	} else {
//		printf("button: id=%d, up\n", event->getButton());
		XTestFakeButtonEvent(
			this->display,
			event->getButton(),
			False,
			CurrentTime
		);
	}
	this->flush();
	return true;
}

bool XRSERVERTEST::processKbdEvent(PXRKBDEVENT event, bool down) {
	if (!this->supported) {
		return false;
	}
	if (down) {
//		printf("kbd: keycode=%d, down\n", event->getKeyCode());
		XTestFakeKeyEvent(
			this->display,
			event->getKeyCode(),
			True,
			CurrentTime
		);
	} else {
//		printf("kbd: keycode=%d, up\n", event->getKeyCode());
		XTestFakeKeyEvent(
			this->display,
			event->getKeyCode(),
			False,
			CurrentTime
		);
	}
	this->flush();
	return true;
}
