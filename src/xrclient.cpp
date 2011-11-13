/**
 * Xremote - client-side
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "xremote.h"
#include "xrscreen.h"
#include "xrdisplay.h"
#include "xrwindow.h"
#include "xrnet.h"
#include "xrclient.h"


XRCLIENT::XRCLIENT(int packetSize, XRNETFILTER *first, const string &localHost, in_port_t localPort, const string &remoteHost, in_port_t remotePort, const string &displayName) : XRNETUDP(packetSize, first, localHost, localPort, remoteHost, remotePort), XRWINDOW(displayName), allows(false), alive(time(NULL)), lastCheck(time(NULL)) {
}

XRCLIENT::~XRCLIENT() {
}


bool XRCLIENT::main() {
	XRLOCKER locker(&this->lock);
	bool code = true;

	// create socket
	if (!this->createSocket()) {
		if (this->isVerbose(XREMOTE_LOG_FAIL)) {
			printf("xremote: error in createSocket()\n");
		}
		code = false;
	}

	// create window
	if (!this->createWindow(0, 0, 1, 1)) {
		if (this->isVerbose(XREMOTE_LOG_FAIL)) {
			printf("xremote: error in createWindow()\n");
		}
		code = false;
	}

	// create cursor
	if (!this->createCursor()) {
		if (this->isVerbose(XREMOTE_LOG_FAIL)) {
			printf("xremote: error in createCursor()\n");
		}
		code = false;
	}

	// event loop
	this->allows = true;
	while (this->allows && code) {
		// receive all event from network
		if (!this->receiveAll()) {
			if (this->isVerbose(XREMOTE_LOG_FAIL)) {
				printf("xremote: error in receiveAll()\n");
			}
			code = false;
		}

		// fetch next X events
		if (!this->pollXEvents()) {
			if (this->isVerbose(XREMOTE_LOG_FAIL)) {
				printf("xremote: error in pollXEvents()\n");
			}
			code = false;
		}

		// send all event to network
		if (!this->sendAll()) {
			if (this->isVerbose(XREMOTE_LOG_FAIL)) {
				printf("xremote: error in sendAll()\n");
			}
			code = false;
		}

		// send alive messages
		if (this->isGrabbing()) {
			if ((time(NULL) - this->lastCheck) >= XREMOTE_CHECK_FREQ) {
				this->lastCheck = time(NULL);

				if ((time(NULL) - this->alive) >= XREMOTE_DEAD_CHECK) {
					if (!this->release()) {
						if (this->isVerbose(XREMOTE_LOG_FAIL)) {
							printf("xremote: error in release()\n");
						}
						code = false;
					}
					this->centerPointerOnScreen();

					XRNETNOTIFYEVENT event = {
						this->meta.getLocalPort(),
						XREVENT_RELEASE,
						XRNOTIFY_NONE,
						0
					};

					this->sendEvent(this->meta, &event);
				} else if ((time(NULL) - this->alive) >= XREMOTE_ALIVE_CHECK) {
					XRNETNOTIFYEVENT event = {
						this->meta.getLocalPort(),
						XREVENT_ALIVE,
						XRNOTIFY_NONE,
						0
					};

					this->sendEvent(this->meta, &event);
				}
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
				XRNETNOTIFYEVENT event = {
					this->meta.getLocalPort(),
					XREVENT_ACQUIRE,
					flags,
					y
				};

				this->sendEvent(this->meta, &event);
			}
		}
	}
	this->allows = false;

	// stop grabbing inputs
	if (!this->ungrabInput()) {
		if (this->isVerbose(XREMOTE_LOG_FAIL)) {
			printf("xremote: error in ungrabInput()\n");
		}
		code = false;
	}

	// destroy cursor
	if (!this->destroyCursor()) {
		if (this->isVerbose(XREMOTE_LOG_FAIL)) {
			printf("xremote: error in destroyCursor()\n");
		}
		code = false;
	}

	// destroy window
	if (!this->destroyWindow()) {
		if (this->isVerbose(XREMOTE_LOG_FAIL)) {
			printf("xremote: error in destroyWindow()\n");
		}
		code = false;
	}

	// destroy socket
	if (!this->destroySocket()) {
		if (this->isVerbose(XREMOTE_LOG_FAIL)) {
			printf("xremote: error in destroySocket()\n");
		}
		code = false;
	}
	return code;
}

void XRCLIENT::abort() {
	this->allows = false;
}


bool XRCLIENT::sendEvent(const XRNETPACKETMETA &meta, XRNETEVENT *event) {
	XRNETBUFFER buffer(sizeof(XRNETEVENT), event);

	return this->send(meta, buffer);
}

bool XRCLIENT::sendEvent(const XRNETPACKETMETA &meta, XRNETNOTIFYEVENT *event) {
	if (this->isVerbose(XREMOTE_LOG_PACKET)) {
		printf("send[%s]: port=%d, type=notify(%d), flags=%d, y=%d\n", meta.getRemoteHost().c_str(), event->port, event->type, event->flags, event->y);
	}
	return this->sendEvent(meta, (XRNETEVENT*)event);
}

bool XRCLIENT::sendEvent(const XRNETPACKETMETA &meta, XRNETPTREVENT *event) {
	if (this->isVerbose(XREMOTE_LOG_PACKET)) {
		printf("send[%s]: port=%d, type=ptr(%d), button=%d, x=%d, y=%d\n", meta.getRemoteHost().c_str(), event->port, event->type, event->button, event->x, event->y);
	}
	return this->sendEvent(meta, (XRNETEVENT*)event);
}

bool XRCLIENT::sendEvent(const XRNETPACKETMETA &meta, XRNETKBDEVENT *event) {
	if (this->isVerbose(XREMOTE_LOG_PACKET)) {
		printf("send[%s]: port=%d, type=kbd(%d), keycode=%d\n", meta.getRemoteHost().c_str(), event->port, event->type, event->keycode);
	}
	return this->sendEvent(meta, (XRNETEVENT*)event);
}
/*
bool XRCLIENT::sendEvent(const XRNETPACKETMETA &meta, XRNETBUFFEREVENT *event, const string &str) {
	XRNETBUFFER buffer(sizeof(XRNETEVENT) + buffer.length());

	buffer.set(0, sizeof(XRNETEVENT), event);
	buffer.set(sizeof(XRNETEVENT), buffer.length(), str.c_str());
	return this->send(meta, buffer);
}
*/

bool XRCLIENT::onReceive(const XRNETPACKETMETA &meta, const XRNETBUFFER &buffer) {
	XRLOCKER locker(&this->lock);

	// fetch header
	XRNETEVENT header;

	buffer.get(0, sizeof(XRNETEVENT), &header);

	if (this->isVerbose(XREMOTE_LOG_PACKET)) {
		switch (header.type) {
		case XREVENT_ACQUIRE:
		case XREVENT_RELEASE:
		case XREVENT_ALIVE:
			{
				XRNETNOTIFYEVENT *notifyev = (XRNETNOTIFYEVENT*)&header;
	
				printf("recv[%s]: port=%d, type=notify(%d), flags=%d, y=%d\n", meta.getRemoteHost().c_str(), notifyev->port, notifyev->type, notifyev->flags, notifyev->y);
			}
			break;
		}
	}

	// check server address
	if (meta.getRemoteAddress() != this->meta.getRemoteAddress() || header.port != this->meta.getRemotePort()) {
		if (this->isVerbose(XREMOTE_LOG_DROP)) {
			printf("xremote: invalid server address (%s, %d)\n", meta.getRemoteHost().c_str(), header.port);
		}
		return false;
	}
	this->alive = time(NULL);

	// dispatch message
	switch (header.type) {
	case XREVENT_ACQUIRE:
		{
			XRNETNOTIFYEVENT *notifyev = (XRNETNOTIFYEVENT*)&header;

			if ((notifyev->flags & XRNOTIFY_REPLY) == XRNOTIFY_REPLY) {
				if (!this->acquire(notifyev->y, notifyev->flags)) {
					this->release(notifyev->y, notifyev->flags);

					XRNETNOTIFYEVENT event = {
						this->meta.getLocalPort(),
						XREVENT_RELEASE,
						XRNOTIFY_NONE,
						notifyev->y
					};

					this->sendEvent(this->meta, &event);
				}
			} else {
				this->release(notifyev->y, notifyev->flags);
			}
		}
		break;

	case XREVENT_RELEASE:
		{
			XRNETNOTIFYEVENT *notifyev = (XRNETNOTIFYEVENT*)&header;

			if ((notifyev->flags & XRNOTIFY_REPLY) != XRNOTIFY_REPLY) {
				XRNETNOTIFYEVENT event = {
					this->meta.getLocalPort(),
					notifyev->type,
					notifyev->flags | XRNOTIFY_REPLY,
					notifyev->y
				};

				this->sendEvent(this->meta, &event);
			}
			this->release(notifyev->y, notifyev->flags);
		}
		break;

	case XREVENT_ALIVE:
		{
			XRNETNOTIFYEVENT *notifyev = (XRNETNOTIFYEVENT*)&header;

			if ((notifyev->flags & XRNOTIFY_REPLY) != XRNOTIFY_REPLY) {
				XRNETNOTIFYEVENT event = {
					this->meta.getLocalPort(),
					notifyev->type,
					notifyev->flags | XRNOTIFY_REPLY,
					notifyev->y
				};

				this->sendEvent(this->meta, &event);
			}
		}
		break;

/*	case XREVENT_BUFFER:
		if (this->isGrabbing()) {
			XRNETBUFFEREVENT *bufferev = (XRNETBUFFEREVENT*)&header;
			XRNETBUFFER data(bufferev->size);

			buffer.get(sizeof(XRNETBUFFEREVENT), bufferev->size, data);
			switch (bufferev->code) {
			case XRBUFFER_PRIMARYBUFFER:
				this->setSelection(XA_PRIMARY, data.getString());
				break;
			case XRBUFFER_SECONDARYBUFFER:
				this->setSelection(XA_SECONDARY, data.getString());
				break;
			case XRBUFFER_CLIPBOARDBUFFER:
				this->setSelection(this->getClipboardAtom(), data.getString());
				break;
			default:
				printf("xremote: unknown buffer (%d)\n", bufferev->code);
				break;
			}
		} else {
			printf("xremote: cannot process buffer event (not grabbed)!\n");
		}
		break;*/

	default:
		if (this->isVerbose(XREMOTE_LOG_DROP)) {
			printf("xremote: unknown message (%d)\n", header.type);
		}
		break;
	}
	return true;
}

bool XRCLIENT::processXEvent(const XEvent &xev) {
	XRLOCKER locker(&this->lock);

	// process grabbing events (pointer+keyboard+clipboard copy)
	if (this->isGrabbing()) {
		XRSCREEN screen(this->getCurrentScreen());
		int cx = screen.getWidth() / 2;
		int cy = screen.getHeight() / 2;
		int code = 0;

		switch (xev.type) {
		case MotionNotify:
			{
				int dx = xev.xbutton.x_root - cx;
				int dy = xev.xbutton.y_root - cy;
				XRNETPTREVENT event = {
					this->meta.getLocalPort(),
					XREVENT_PTR_MOVE,
					0,
					dx,
					dy
				};

				this->sendEvent(this->meta, &event);
				if (dx != 0 || dy != 0) {
					screen.setPointer(cx, cy);
				}
			}
			break;
		case ButtonPress:
			{
				int dx = xev.xbutton.x_root - cx;
				int dy = xev.xbutton.y_root - cy;
				XRNETPTREVENT event = {
					this->meta.getLocalPort(),
					XREVENT_PTR_DOWN,
					xev.xbutton.button,
					dx,
					dy
				};

				this->sendEvent(this->meta, &event);
				if (dx != 0 || dy != 0) {
					screen.setPointer(cx, cy);
				}
			}
			break;
		case ButtonRelease:
			{
				int dx = xev.xbutton.x_root - cx;
				int dy = xev.xbutton.y_root - cy;
				XRNETPTREVENT event = {
					this->meta.getLocalPort(),
					XREVENT_PTR_UP,
					xev.xbutton.button,
					dx,
					dy
				};

				this->sendEvent(this->meta, &event);
				if (dx != 0 || dy != 0) {
					screen.setPointer(cx, cy);
				}
			}
			break;

		case KeyPress:
			{
				XRNETKBDEVENT event = {
					this->meta.getLocalPort(),
					XREVENT_KBD_DOWN,
					xev.xkey.keycode
				};

				this->sendEvent(this->meta, &event);
			}
			break;
		case KeyRelease:
			{
				XRNETKBDEVENT event = {
					this->meta.getLocalPort(),
					XREVENT_KBD_UP,
					xev.xkey.keycode
				};

				this->sendEvent(this->meta, &event);
			}
			break;

/*		case SelectionNotify:
			{
				string selection = this->getSelection(xev.xselection.selection, xev.xselection.property);
				int code = XRBUFFER_NONE;

				if (xev.xselection.selection == XA_PRIMARY) {
					code = XRBUFFER_PRIMARYBUFFER;
				} else if (xev.xselection.selection == XA_SECONDARY) {
					code = XRBUFFER_SECONDARYBUFFER;
				} else if (xev.xselection.selection == this->getClipboardAtom()) {
					code = XRBUFFER_CLIPBOARDBUFFER;
				} else {
					break;
				}

				XRNETBUFFEREVENT event = {
					this->meta.getLocalPort(),
					XREVENT_BUFFER,
					code,
					selection.length()
				};
	
				this->sendEvent(this->meta, &event, selection);
				this->setSelection(xev.xselection.selection, "");
			}
			break;*/
		}
	}

	// process local clipboard events
/*	switch (xev.type) {
	case SelectionRequest:
		this->sendSelectionNotify(xev);
		break;

	case SelectionClear:
		this->clearSelection(xev.xselectionclear.selection);
		break;
	}*/
	return true;
}


bool XRCLIENT::acquire(int y, int flags) {
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
		this->askSelection(this->getClipboardAtom());
	}
	return true;
}

bool XRCLIENT::release(int y, int flags) {
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
