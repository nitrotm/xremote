/**
 * Xremote - X "InputOnly" window with input grab
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "xremote.h"
#include "xrscreen.h"
#include "xrdisplay.h"
#include "xrwindow.h"


XRWINDOW::XRWINDOW(const string &displayName) : XRDISPLAY(displayName), grab(false), window(None), cursor(None), atomId(1) {
	this->XA_CLIPBOARD = XInternAtom(this->display, "CLIPBOARD", False);
}

XRWINDOW::XRWINDOW(const XRWINDOW &ref) : XRDISPLAY((const XRDISPLAY &)ref), grab(false), window(None), cursor(None), atomId(1), XA_CLIPBOARD(ref.XA_CLIPBOARD) {
}

XRWINDOW::~XRWINDOW() {
	this->ungrabInput();
	this->destroyCursor();
	this->destroyWindow();
}


bool XRWINDOW::createWindow(int x, int y, int width, int height) {
	if (!this->destroyWindow()) {
		return false;
	}

	// get screen
	XRSCREEN screen(this->getCurrentScreen());

	// build attributes
	XSetWindowAttributes attrs;

	attrs.win_gravity = NorthWestGravity;
	attrs.event_mask = PointerMotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
	attrs.do_not_propagate_mask = PointerMotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
	attrs.override_redirect = True;
	attrs.cursor = None;

	// create window
	this->window = XCreateWindow(
		this->display,
		screen.getRootWindow(),
		x,
		y,
		width,
		height,
		0,
		0,
		InputOnly,
		screen.getDefaultVisual(),
		CWWinGravity | CWEventMask | CWDontPropagate | CWOverrideRedirect | CWCursor,
		&attrs
	);

	this->flush();
	return true;
}

bool XRWINDOW::destroyWindow() {
	if (this->window != None) {
		XDestroyWindow(this->display, this->window);
		this->window = None;
	}

	this->flush();
	return true;
}


bool XRWINDOW::createCursor() {
	// create cursor source bitmap
	Pixmap cursorSource = XCreatePixmap(this->display, this->window, 1, 1, 1);
	GC cursorSourceGC = XCreateGC(this->display, cursorSource, 0, NULL);

	XSetBackground(this->display, cursorSourceGC, 0);
	XFillRectangle(this->display, cursorSource, cursorSourceGC, 0, 0, 10, 10);
	XFreeGC(this->display, cursorSourceGC);

	// create cursor from pixmap
	XColor fg = { 0, 0xFFFF, 0xFFFF, 0xFFFF, 0, 0 };
	XColor bg = { 0, 0x0000, 0x0000, 0x0000, 0, 0 };

	this->cursor = XCreatePixmapCursor(this->display, cursorSource, cursorSource, &fg, &bg, 0, 0);

	// free pixmap
	XFreePixmap(this->display, cursorSource);

	this->flush();
	return true;
}

bool XRWINDOW::destroyCursor() {
	if (this->cursor != None) {
		XFreeCursor(this->display, this->cursor);
		this->cursor = None;
	}

	this->flush();
	return true;
}


bool XRWINDOW::grabInput() {
	if (this->window == None) {
		return false;
	}
	if (!this->grab) {
		XMapWindow(this->display, this->window);
		XSetInputFocus(this->display, this->window, RevertToPointerRoot, CurrentTime);
		if (XGrabPointer(
				this->display,
				this->window,
				False,
				PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
				GrabModeAsync,
				GrabModeAsync,
				None,
				this->cursor,
				CurrentTime
			) != GrabSuccess) {
			this->flush();
			return false;
		}
		if (XGrabKeyboard(this->display, this->window, False, GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess) {
			XUngrabPointer(this->display, CurrentTime);
			this->flush();
			return false;
		}
		this->flush();

		// remove all event on the queue
		while (XPending(this->display)) {
			XEvent xev;

			XNextEvent(this->display, &xev);
		}
		this->grab = true;
	}
	return true;
}

bool XRWINDOW::ungrabInput() {
	if (this->window == None) {
		return false;
	}
	if (this->grab) {
		// remove all event on the queue
		while (XPending(this->display)) {
			XEvent xev;

			XNextEvent(this->display, &xev);
		}

		XUngrabKeyboard(this->display, CurrentTime);
		XUngrabPointer(this->display, CurrentTime);
		XUnmapWindow(this->display, this->window);

		this->flush();
		this->grab = false;
	}
	return true;
}

bool XRWINDOW::isGrabbing() {
	return this->grab;
}


bool XRWINDOW::centerPointerOnScreen() {
	return this->getCurrentScreen().centerPointer();
}

bool XRWINDOW::pollXEvents() {
	while (XPending(this->display)) {
		XEvent xev;

		XNextEvent(this->display, &xev);
		if (!this->processXEvent(xev)) {
			return false;
		}
	}
	return true;
}

bool XRWINDOW::processXEvent(const XEvent &xev) {
	return false;
}


Atom XRWINDOW::getClipboardAtom() const {
	return this->XA_CLIPBOARD;
}

void XRWINDOW::askSelection(Atom selection) {
	char propertyName[256];

	sprintf(propertyName, "XREMOTE_CLIPBOARD_%d", this->atomId++);
	XConvertSelection(
		this->display,
		selection,
		XA_STRING,
		XInternAtom(display, propertyName, False),
		this->window,
		CurrentTime
	);

	this->flush();
}

string XRWINDOW::getSelection(Atom selection, Atom property) const {
	if (property != None) {
		Atom type = None;
		int format = 0;
		unsigned long nb = 0;
		unsigned long left = 0;
		unsigned char *data = NULL;

		XGetWindowProperty(
			this->display,
			this->window,
			property,
			0,
			1024,
			True,
			XA_STRING,
			&type,
			&format,
			&nb,
			&left,
			&data
		);
		XDeleteProperty(this->display, this->window, property);

		this->flush();

		if (type == XA_STRING && format == 8 && nb > 0 && data != NULL) {
			string buffer((char*)data);

			XFree(data);
			return buffer;
		} else if (data != NULL) {
			XFree(data);
		} else if (nb > 0) {
//			printf("error get property (type=%d, format=%d, nb=%d)\n", (int)type, format, (int)nb);
		}
	}
	return "";
}

void XRWINDOW::setSelection(Atom selection, const string &text) {
	if (selection == XA_PRIMARY) {
		this->primaryTextSelection = text;
	} else if (selection == XA_SECONDARY) {
		this->secondaryTextSelection = text;
	} else if (selection == this->XA_CLIPBOARD) {
		this->clipboardTextSelection = text;
	} else {
		return;
	}
	XSetSelectionOwner(this->display, selection, this->window, CurrentTime);

	this->flush();
}

void XRWINDOW::sendSelectionNotify(const XEvent &xev) {
	Window requestor = xev.xselectionrequest.requestor;
	Atom selection = xev.xselectionrequest.selection;
	Atom target = xev.xselectionrequest.target;
	Atom property = xev.xselectionrequest.property;
	string text;

	if (selection == XA_PRIMARY) {
		text = this->primaryTextSelection;
	} else if (selection == XA_SECONDARY) {
		text = this->secondaryTextSelection;
	} else if (selection == this->XA_CLIPBOARD) {
		text = this->clipboardTextSelection;
	}
	if (text.length() > 0) {
		if (property == None) {
			property = XInternAtom(this->display, "XR_STRING", False);
		}
		if (target == XA_STRING) {
			XChangeProperty(
				this->display,
				requestor,
				property,
				target,
				8,
				PropModeReplace,
				(unsigned char*)text.c_str(),
				text.length()
			);
		} else {
			char *targetName = XGetAtomName(this->display, target);
	
			if (strcmp(targetName, "COMPOUND_TEXT") == 0) {
				XTextProperty prop;
				char *list[1];
	
				list[0] = new char[text.length() + 1];
				strcpy(list[0], text.c_str());
	
				if (XmbTextListToTextProperty(this->display, list, 1, XCompoundTextStyle, &prop) >= 0) {
					XSetTextProperty(this->display, requestor, &prop, property);
				} else {
//					printf("special target (%s) not convertible\n", targetName);
					property = None;
				}
				delete [] list[0];
			} else {
//				printf("unknown target (%s)\n", targetName);
				property = None;
			}
			XFree(targetName);
		}
	} else {
		property = None;
	}

	XEvent xev2;

	memset(&xev2, 0, sizeof(XEvent));
	xev2.xselection.type = SelectionNotify;
	xev2.xselection.display = this->display;
	xev2.xselection.requestor = requestor;
	xev2.xselection.selection = selection;
	xev2.xselection.target = target;
	xev2.xselection.property = property;
	xev2.xselection.time = xev.xselectionrequest.time;
	XSendEvent(this->display, requestor, False, 0, &xev2);

	this->flush();
}

void XRWINDOW::clearSelection(Atom selection) {
	if (selection == XA_PRIMARY) {
		this->primaryTextSelection = "";
	} else if (selection == XA_SECONDARY) {
		this->secondaryTextSelection = "";
	} else if (selection == this->XA_CLIPBOARD) {
		this->clipboardTextSelection = "";
	}
}
