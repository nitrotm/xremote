/**
 * Xremote - client-side
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "xremote.h"


XRCLIENT::XRCLIENT(const string &localHost, in_port_t localPort, const string &remoteHost, in_port_t remotePort, const string &displayName) : XRNET(localHost, localPort, remoteHost, remotePort, displayName), allows(false), alive(time(NULL)) {
}

XRCLIENT::XRCLIENT(const XRCLIENT &ref) : XRNET((const XRNET &)ref), allows(false), alive(time(NULL)) {
}

XRCLIENT::~XRCLIENT() {
}


bool XRCLIENT::main() {
	XRLOCKER locker(&this->lock);
	bool		 code = true;

	// create socket
	if (!this->createSocket()) {
		printf("xremote: error in createSocket()\n");
		code = false;
	}

	// create window
	if (!this->createWindow(0, 0, 1, 1)) {
		printf("xremote: error in createWindow()\n");
		code = false;
	}

	// create cursor
	if (!this->createCursor()) {
		printf("xremote: error in createCursor()\n");
		code = false;
	}

	// event loop
//	printf("xremote: client main loop begin\n");
	this->allows = true;
	while (this->allows && code) {
		// receive all event from network
		if (!this->receiveAll()) {
			printf("xremote: error in receiveAll()\n");
			code = false;
		}

		// fetch next X events
		if (!this->pollXEvents()) {
			printf("xremote: error in pollXEvents()\n");
			code = false;
		}

		// send all event to network
		if (!this->sendAll()) {
			printf("xremote: error in sendAll()\n");
			code = false;
		}

		// send alive messages
		if (this->isGrabbing()) {
			if ((time(NULL) - this->alive) >= XREMOTE_DEAD_CHECK) {
				this->send(
					new XRNOTIFYEVENT(
						XREVENT_END,
						0,
						this->localPort,
						this->remoteAddress,
						this->remotePort,
						XRNOTIFY_NONE
					)
				);

				if (!this->end(0, 0)) {
					printf("xremote: error in end()\n");
					code = false;
				}
				this->centerPointerOnScreen();
			} else if ((time(NULL) - this->alive) >= XREMOTE_ALIVE_CHECK) {
				this->send(
					new XRNOTIFYEVENT(
						XREVENT_ALIVE,
						0,
						this->localPort,
						this->remoteAddress,
						this->remotePort,
						XRNOTIFY_NONE
					)
				);
			}
		} else {
			// check the pointer at the border of display
			XRSCREEN firstScreen(this->getScreen(0));
			XRSCREEN lastScreen(this->getScreen(this->getScreenCount() - 1));
			Window wnd = None;
			int flags = XRNOTIFY_NONE;
			int x = 0;
			int y = 0;

			if (firstScreen.getPointer(&wnd, &x, &y) && x <= 0) {
				flags = XRNOTIFY_LEFT;
			} else if (lastScreen.getPointer(&wnd, &x, &y) && x >= (lastScreen.getWidth() - 1)) {
				flags = XRNOTIFY_RIGHT;
			} else {
				locker.usleep(10000);
			}
			if (flags != XRNOTIFY_NONE) {
				this->send(
					new XRNOTIFYEVENT(
						XREVENT_BEGIN,
						y,
						this->localPort,
						this->remoteAddress,
						this->remotePort,
						flags
					)
				);
			}
		}
	}
	this->allows = false;
//	printf("xremote: client main loop end\n");

	// stop grabbing inputs
	if (!this->ungrabInput()) {
		printf("xremote: error in ungrabInput()\n");
		code = false;
	}

	// destroy cursor
	if (!this->destroyCursor()) {
		printf("xremote: error in destroyCursor()\n");
		code = false;
	}

	// destroy window
	if (!this->destroyWindow()) {
		printf("xremote: error in destroyWindow()\n");
		code = false;
	}

	// destroy socket
	if (!this->destroySocket()) {
		printf("xremote: error in destroySocket()\n");
		code = false;
	}
	return code;
}

void XRCLIENT::shutdown() {
	XRLOCKER locker(&this->lock);

	this->allows = false;
}


bool XRCLIENT::onReceive(PXREVENT event) {
	XRLOCKER locker(&this->lock);

	if (event->getRemoteAddress() != this->remoteAddress || event->getRemotePort() != this->remotePort) {
		printf("xremote: invalid server address (%s)\n", event->getRemoteHost().c_str());
		return false;
	}
	this->alive = time(NULL);
	if ((event->getType() & XREVENT_BEGIN) == XREVENT_BEGIN) {
		PXRNOTIFYEVENT notifyev = (PXRNOTIFYEVENT)event;

		if ((notifyev->getFlags() & XRNOTIFY_REPLY) != XRNOTIFY_REPLY) {
			return this->end(notifyev->getY(), notifyev->getFlags());
		} else {
			if (!this->begin(notifyev->getY(), notifyev->getFlags())) {
				this->send(
					new XRNOTIFYEVENT(
						XREVENT_END,
						notifyev->getY(),
						this->localPort,
						this->remoteAddress,
						this->remotePort,
						XRNOTIFY_NONE
					)
				);
				return this->end(notifyev->getY(), notifyev->getFlags());
			}
		}
	} else if (this->isGrabbing()) {
		if ((event->getType() & XREVENT_END) == XREVENT_END) {
			PXRNOTIFYEVENT notifyev = (PXRNOTIFYEVENT)event;

			if ((notifyev->getFlags() & XRNOTIFY_REPLY) != XRNOTIFY_REPLY) {
				this->send(
					new XRNOTIFYEVENT(
						(const XRNOTIFYEVENT &)*event,
						notifyev->getFlags() | XRNOTIFY_REPLY
					)
				);
			}
			return this->end(notifyev->getY(), notifyev->getFlags());
		} else if ((event->getType() & XREVENT_ALIVE) == XREVENT_ALIVE) {
			PXRNOTIFYEVENT notifyev = (PXRNOTIFYEVENT)event;

			if ((notifyev->getFlags() & XRNOTIFY_REPLY) != XRNOTIFY_REPLY) {
				this->send(
					new XRNOTIFYEVENT(
						(const XRNOTIFYEVENT &)*event,
						notifyev->getFlags() | XRNOTIFY_REPLY
					)
				);
			}
		} else if ((event->getType() & XREVENT_BUFFER) == XREVENT_BUFFER) {
			PXRBUFFEREVENT bufferev = (PXRBUFFEREVENT)event;

			switch (bufferev->getCode()) {
			case XRNETDATA_CODE_PRIMARYBUFFER:
				this->setSelection(XA_PRIMARY, bufferev->getAsString());
				break;
			case XRNETDATA_CODE_SECONDARYBUFFER:
				this->setSelection(XA_SECONDARY, bufferev->getAsString());
				break;
			case XRNETDATA_CODE_CLIPBOARDBUFFER:
				this->setSelection(this->XA_CLIPBOARD, bufferev->getAsString());
				break;
			}
//			printf("xremote: buffer %d received (%s)\n", bufferev->getCode(), bufferev->getAsString().c_str());
		}
//	} else {
//		printf("xremote: unknown message (%d)\n", event->getType());
	}
	return true;
}

bool XRCLIENT::onSend(PXREVENT event) {
	XRLOCKER locker(&this->lock);

	return true;
}


bool XRCLIENT::processXEvent(const XEvent &xev) {
	XRLOCKER locker(&this->lock);

	// process grabbing events (pointer+keyboard+clipboard copy)
	if (this->isGrabbing()) {
		XRSCREEN screen(this->getCurrentScreen());
		int cx = screen.getWidth() / 2;
		int cy = screen.getHeight() / 2;
		int dx, dy;
		unsigned char code = 0;

		switch (xev.type) {
		case MotionNotify:
			dx = xev.xmotion.x_root - cx;
			dy = xev.xmotion.y_root - cy;
			if (dx != 0 || dy != 0) {
				this->send(
					new XRPTREVENT(
						XREVENT_PTR,
						this->localPort,
						this->remoteAddress,
						this->remotePort,
						dx,
						dy
					)
				);
				screen.setPointer(cx, cy);
			}
			break;
		case ButtonPress:
			dx = xev.xbutton.x_root - cx;
			dy = xev.xbutton.y_root - cy;
			this->send(
				new XRPTREVENT(
					XREVENT_PTR | XREVENT_DOWN,
					this->localPort,
					this->remoteAddress,
					this->remotePort,
					dx,
					dy,
					xev.xbutton.button
				)
			);
			if (dx != 0 || dy != 0) {
				screen.setPointer(cx, cy);
			}
			break;
		case ButtonRelease:
			dx = xev.xbutton.x_root - cx;
			dy = xev.xbutton.y_root - cy;
			this->send(
				new XRPTREVENT(
					XREVENT_PTR | XREVENT_UP,
					this->localPort,
					this->remoteAddress,
					this->remotePort,
					dx,
					dy,
					xev.xbutton.button
				)
			);
			if (dx != 0 || dy != 0) {
				screen.setPointer(cx, cy);
			}
			break;

		case KeyPress:
			this->send(
				new XRKBDEVENT(
					XREVENT_KBD | XREVENT_DOWN,
					this->localPort,
					this->remoteAddress,
					this->remotePort,
					xev.xkey.keycode
				)
			);
			break;
		case KeyRelease:
			this->send(
				new XRKBDEVENT(
					XREVENT_KBD | XREVENT_UP,
					this->localPort,
					this->remoteAddress,
					this->remotePort,
					xev.xkey.keycode
				)
			);
			break;

		case SelectionNotify:
			if (xev.xselection.selection == XA_PRIMARY) {
				code = XRNETDATA_CODE_PRIMARYBUFFER;
			} else if (xev.xselection.selection == XA_SECONDARY) {
				code = XRNETDATA_CODE_SECONDARYBUFFER;
			} else if (xev.xselection.selection == this->XA_CLIPBOARD) {
				code = XRNETDATA_CODE_CLIPBOARDBUFFER;
			}
			if (code > 0) {
				this->send(
					new XRBUFFEREVENT(
						this->localPort,
						this->remoteAddress,
						this->remotePort,
						code,
						this->getSelection(xev.xselection.selection, xev.xselection.property)
					)
				);

				// clear clipboard
				this->setSelection(xev.xselection.selection, "");
			}
			break;
		}
	}

	// process local clipboard events
	switch (xev.type) {
	case SelectionRequest:
		this->sendSelectionNotify(xev);
		break;

	case SelectionClear:
		this->clearSelection(xev.xselectionclear.selection);
		break;
	}
	return this->sendAll();
}


bool XRCLIENT::begin(int y, int flags) {
	XRLOCKER locker(&this->lock);

	if (!this->isGrabbing()) {
		// start grab inputs
		if (!this->grabInput()) {
			return false;
		}
	
		// set the pointer in center
		if (!this->centerPointerOnScreen()) {
			return false;
		}
	
		// ask for clipboard transfert
		this->askSelection(XA_PRIMARY);
		this->askSelection(XA_SECONDARY);
		this->askSelection(this->XA_CLIPBOARD);
	}
	return true;
}

bool XRCLIENT::end(int y, int flags) {
	XRLOCKER locker(&this->lock);

	if (this->isGrabbing()) {
		// stop grabbing inputs
		if (!this->ungrabInput()) {
			return false;
		}

		// restore pointer position
		if ((flags & XRNOTIFY_LEFT) == XRNOTIFY_LEFT) {
			XRSCREEN lastScreen(this->getScreen(this->getScreenCount() - 1));
	
			return lastScreen.setPointer(lastScreen.getWidth() - 2, y);
		} else if ((flags & XRNOTIFY_RIGHT) == XRNOTIFY_RIGHT) {
			XRSCREEN firstScreen(this->getScreen(0));
	
			return firstScreen.setPointer(1, y);
		}
	}
	return true;
}
