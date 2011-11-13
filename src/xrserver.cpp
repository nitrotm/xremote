/**
 * Xremote - server-side
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
#include "xrserver.h"


XRSERVER::XRSERVER(int packetSize, XRNETFILTER *first, const string &localHost, in_port_t localPort, const string &displayName) : XRNETUDP(packetSize, first, localHost, localPort), XRWINDOW(displayName), debug(false), allows(false) {
}

XRSERVER::~XRSERVER() {
}


bool XRSERVER::main(bool debug) {
	XRLOCKER locker(&this->lock);
	bool code = true;

	this->debug = debug;

	// check if XTest exists
	int eventBase = 0;
	int errorBase = 0;
	int majorVersion = 0;
	int minorVersion = 0;

	if (XTestQueryExtension(this->display, &eventBase, &errorBase, &majorVersion, &minorVersion) != True) {
		printf("xremote: error in XTestQueryExtension()\n");
		code = false;
	}

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

	// start grab inputs
	if (!this->grabInput()) {
		printf("xremote: error in grabInput()\n");
		return false;
	}

	// main loop
	this->allows = true;
	while (this->allows && code) {
		// receive network events
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
		if (!this->isGrabbing()) {
			for (map<string, XRCLIENTREF>::iterator it = this->clients.begin(); it != this->clients.end(); ++it) {
				if ((time(NULL) - it->second.lastCheck) >= XREMOTE_CHECK_FREQ) {
					it->second.lastCheck = time(NULL);

					if ((time(NULL) - it->second.alive) >= XREMOTE_DEAD_CHECK) {
						this->clients.erase(it);

						XRNETNOTIFYEVENT event = {
							this->meta.getLocalPort(),
							XREVENT_RELEASE,
							XRNOTIFY_NONE,
							0
						};
	
						this->sendEvent(it->second.meta, &event);
					} else if ((time(NULL) - it->second.alive) >= XREMOTE_ALIVE_CHECK) {
						XRNETNOTIFYEVENT event = {
							this->meta.getLocalPort(),
							XREVENT_ALIVE,
							XRNOTIFY_NONE,
							0
						};

						this->sendEvent(it->second.meta, &event);
					}
				}
			}
		}
		if (this->clients.size() == 0) {
			this->release();
		}
	}
	this->allows = false;

	// reset buttons down
	for (map<int, bool>::iterator it = this->buttons.begin(); it != this->buttons.end(); ++it) {
		if (it->second) {
			XRNETPTREVENT event = {
				XREVENT_PTR_UP,
				it->first,
				0,
				0
			};
	
			this->processButtonEvent(&event);
		}
	}

	// reset keys down
	for (map<unsigned int, bool>::iterator it = this->keys.begin(); it != this->keys.end(); ++it) {
		if (it->second) {
			XRNETKBDEVENT event = {
				XREVENT_KBD_UP,
				it->first
			};

			this->processKbdEvent(&event);
		}
	}

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

void XRSERVER::abort() {
	this->allows = false;
}


bool XRSERVER::sendEvent(const XRNETPACKETMETA &meta, XRNETEVENT *event) {
	XRNETBUFFER buffer(sizeof(XRNETEVENT), event);

	return this->send(meta, buffer);
}

bool XRSERVER::sendEvent(const XRNETPACKETMETA &meta, XRNETNOTIFYEVENT *event) {
	if (this->debug) {
		printf("send[%s]: port=%d, type=notify(%d), flags=%d, y=%d\n", meta.getRemoteHost().c_str(), event->port, event->type, event->flags, event->y);
	}
	return this->sendEvent(meta, (XRNETEVENT*)event);
}

bool XRSERVER::sendEvent(const XRNETPACKETMETA &meta, XRNETPTREVENT *event) {
	if (this->debug) {
		printf("send[%s]: port=%d, type=ptr(%d), button=%d, x=%d, y=%d\n", meta.getRemoteHost().c_str(), event->port, event->type, event->button, event->x, event->y);
	}
	return this->sendEvent(meta, (XRNETEVENT*)event);
}

bool XRSERVER::sendEvent(const XRNETPACKETMETA &meta, XRNETKBDEVENT *event) {
	if (this->debug) {
		printf("send[%s]: port=%d, type=kbd(%d), keycode=%d\n", meta.getRemoteHost().c_str(), event->port, event->type, event->keycode);
	}
	return this->sendEvent(meta, (XRNETEVENT*)event);
}
/*
bool XRSERVER::sendEvent(const XRNETPACKETMETA &meta, XRNETBUFFEREVENT *event, const string &str) {
	XRNETBUFFER buffer(sizeof(XRNETEVENT) + buffer.length());

	buffer.set(0, sizeof(XRNETEVENT), event);
	buffer.set(sizeof(XRNETEVENT), buffer.length(), str.c_str());
	return this->send(meta, buffer);
}
*/

bool XRSERVER::onReceive(const XRNETPACKETMETA &meta, const XRNETBUFFER &buffer) {
	XRLOCKER locker(&this->lock);

	// fetch header
	XRNETEVENT header;

	buffer.get(0, sizeof(XRNETEVENT), &header);

	if (this->debug) {
		switch (header.type) {
		case XREVENT_ACQUIRE:
		case XREVENT_RELEASE:
		case XREVENT_ALIVE:
			{
				XRNETNOTIFYEVENT *notifyev = (XRNETNOTIFYEVENT*)&header;
	
				printf("recv[%s]: port=%d, type=notify(%d), flags=%d, y=%d\n", meta.getRemoteHost().c_str(), notifyev->port, notifyev->type, notifyev->flags, notifyev->y);
			}
			break;
	
		case XREVENT_PTR_MOVE:
		case XREVENT_PTR_DOWN:
		case XREVENT_PTR_UP:
			{
				XRNETPTREVENT *ptrev = (XRNETPTREVENT*)&header;
	
				printf("recv[%s]: port=%d, type=ptr(%d), button=%d, x=%d, y=%d\n", meta.getRemoteHost().c_str(), ptrev->port, ptrev->type, ptrev->button, ptrev->x, ptrev->y);
			}
			break;
	
		case XREVENT_KBD_DOWN:
		case XREVENT_KBD_UP:
			{
				XRNETKBDEVENT *kbdev = (XRNETKBDEVENT*)&header;

				printf("recv[%s]: port=%d, type=kbd(%d), keycode=%d\n", meta.getRemoteHost().c_str(), kbdev->port, kbdev->type, kbdev->keycode);
			}
			break;
		}
	}

	// find client
	XRNETPACKETMETA clientMeta = XRNETPACKETMETA(
		meta.getLocalAddress(),
		meta.getLocalPort(),
		meta.getRemoteAddress(),
		header.port
	);
	map<string, XRCLIENTREF>::iterator it = this->clients.find(clientMeta.getRemoteHost());

	if (it != this->clients.end()) {
		it->second.alive = time(NULL);
	}

	// dispatch message
	switch (header.type) {
	case XREVENT_ACQUIRE:
		{
			XRNETNOTIFYEVENT *notifyev = (XRNETNOTIFYEVENT*)&header;

			if ((notifyev->flags & XRNOTIFY_REPLY) != XRNOTIFY_REPLY) {
				XRNETNOTIFYEVENT event = {
					this->meta.getLocalPort(),
					notifyev->type,
					notifyev->flags | XRNOTIFY_REPLY,
					notifyev->y
				};

				if (it == this->clients.end()) {
					this->clients[clientMeta.getRemoteHost()].meta = clientMeta;
					if (!this->acquire(notifyev->y, notifyev->flags)) {
						printf("xremote: error in acquire()\n");
					}
				}
				this->sendEvent(clientMeta, &event);
			}
		}
		break;

	case XREVENT_RELEASE:
		if (it != this->clients.end()) {
			XRNETNOTIFYEVENT *notifyev = (XRNETNOTIFYEVENT*)&header;

			if ((notifyev->flags & XRNOTIFY_REPLY) != XRNOTIFY_REPLY) {
				XRNETNOTIFYEVENT event = {
					this->meta.getLocalPort(),
					notifyev->type,
					notifyev->flags | XRNOTIFY_REPLY,
					notifyev->y
				};

				this->sendEvent(clientMeta, &event);
			} else {
				this->clients.erase(it);
			}
		}
		break;

	case XREVENT_ALIVE:
		if (it != this->clients.end()) {
			XRNETNOTIFYEVENT *notifyev = (XRNETNOTIFYEVENT*)&header;

			if ((notifyev->flags & XRNOTIFY_REPLY) != XRNOTIFY_REPLY) {
				XRNETNOTIFYEVENT event = {
					this->meta.getLocalPort(),
					notifyev->type,
					notifyev->flags | XRNOTIFY_REPLY,
					notifyev->y
				};

				this->sendEvent(clientMeta, &event);
			}
		}
		break;

	case XREVENT_PTR_MOVE:
		if (it != this->clients.end()) {
			XRNETPTREVENT *ptrev = (XRNETPTREVENT*)&header;

			if (!this->processPtrEvent(ptrev)) {
				printf("xremote: error in processPtrEvent()\n");
			}
		}
		break;

	case XREVENT_PTR_DOWN:
		if (it != this->clients.end()) {
			XRNETPTREVENT *ptrev = (XRNETPTREVENT*)&header;

			if (!this->processPtrEvent(ptrev)) {
				printf("xremote: error in processPtrEvent()\n");
			}

			if (this->processButtonEvent(ptrev)) {
				this->buttons[ptrev->button] = true;
			} else {
				printf("xremote: error in processButtonEvent()\n");
			}
		}
		break;

	case XREVENT_PTR_UP:
		if (it != this->clients.end()) {
			XRNETPTREVENT *ptrev = (XRNETPTREVENT*)&header;

			if (!this->processPtrEvent(ptrev)) {
				printf("xremote: error in processPtrEvent()\n");
			}

			if (this->processButtonEvent(ptrev)) {
				this->buttons[ptrev->button] = false;
			} else {
				printf("xremote: error in processButtonEvent()\n");
			}
		}
		break;

	case XREVENT_KBD_DOWN:
		if (it != this->clients.end()) {
			XRNETKBDEVENT *kbdev = (XRNETKBDEVENT*)&header;

			if (this->processKbdEvent(kbdev)) {
				this->keys[kbdev->keycode] = true;
			} else {
				printf("xremote: error in processKbdEvent()\n");
			}
		}
		break;

	case XREVENT_KBD_UP:
		if (it != this->clients.end()) {
			XRNETKBDEVENT *kbdev = (XRNETKBDEVENT*)&header;

			if (this->processKbdEvent(kbdev)) {
				this->keys[kbdev->keycode] = false;
			} else {
				printf("xremote: error in processKbdEvent()\n");
			}
		}
		break;

/*	case XREVENT_BUFFER:
		if (it != this->clients.end()) {
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
		}
		break;*/

	default:
		printf("xremote: unknown message (%d)\n", header.type);
		break;
	}
	return true;
}


bool XRSERVER::processXEvent(const XEvent &xev) {
	XRLOCKER locker(&this->lock);

	// process clipboard events
/*	switch (xev.type) {
	case SelectionNotify:
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

			for (map<string, XRCLIENTREF>::iterator it = this->clients.begin(); it != this->clients.end(); ++it) {
				this->sendEvent(it->second.meta, &event, selection);
			}
			this->setSelection(xev.xselection.selection, "");
		}
		break;

	case SelectionRequest:
		this->sendSelectionNotify(xev);
		break;

	case SelectionClear:
		this->clearSelection(xev.xselectionclear.selection);
		break;
	}*/
	return this->sendAll();
}

bool XRSERVER::processPtrEvent(XRNETPTREVENT *event) {
	XTestFakeRelativeMotionEvent(this->display, event->x, event->y, CurrentTime);
	this->flush();

	// dispatch pointer remotely?
	XRSCREEN screen(this->getCurrentScreen());
	Window wnd;
	int x = 0;
	int y = 0;
	int w = screen.getWidth();
	int h = screen.getHeight();

	if (screen.getPointer(&wnd, &x, &y)) {
		if ((x + event->x) < 0) {
			// move to previous screen if any
			x += event->x;
			y += event->y;
			if (y < 0) {
				y = 0;
			} else if (y >= h) {
				y = h - 1;
			}
			if (screen.getIndex() > 0) {
				screen = this->getScreen(screen.getIndex() - 1);
				return screen.setPointer(screen.getWidth() - x, y);
			}

			for (map<string, XRCLIENTREF>::iterator it = this->clients.begin(); it != this->clients.end(); ++it) {
				XRNETNOTIFYEVENT event = {
					this->meta.getLocalPort(),
					XREVENT_RELEASE,
					XRNOTIFY_LEFT,
					y
				};

				this->sendEvent(it->second.meta, &event);
			}
		} else if ((x + event->x) >= w) {
			// move to next screen if any
			x += event->x;
			y += event->y;
			if (y < 0) {
				y = 0;
			} else if (y >= h) {
				y = h - 1;
			}
			if (screen.getIndex() < (this->getScreenCount() - 1)) {
				screen = this->getScreen(screen.getIndex() + 1);
				return screen.setPointer(x - w, y);
			}

			for (map<string, XRCLIENTREF>::iterator it = this->clients.begin(); it != this->clients.end(); ++it) {
				XRNETNOTIFYEVENT event = {
					this->meta.getLocalPort(),
					XREVENT_RELEASE,
					XRNOTIFY_RIGHT,
					y
				};

				this->sendEvent(it->second.meta, &event);
			}
		}
	}
	return true;
}

bool XRSERVER::processButtonEvent(XRNETPTREVENT *event) {
	switch (event->type) {
	case XREVENT_PTR_DOWN:
		XTestFakeButtonEvent(this->display, event->button, True, CurrentTime);
		break;

	case XREVENT_PTR_UP:
		XTestFakeButtonEvent(this->display, event->button, False, CurrentTime);
		break;
	}
	this->flush();
	return true;
}

bool XRSERVER::processKbdEvent(XRNETKBDEVENT *event) {
	switch (event->type) {
	case XREVENT_KBD_DOWN:
		XTestFakeKeyEvent(this->display, event->keycode, True, CurrentTime);
		break;

	case XREVENT_KBD_UP:
		XTestFakeKeyEvent(this->display, event->keycode, False, CurrentTime);
		break;
	}
	this->flush();
	return true;
}


bool XRSERVER::acquire(int y, int flags) {
	XRLOCKER locker(&this->lock);
	
	// stop grabbing inputs
	if (!this->ungrabInput()) {
		return false;
	}

	// set pointer position
	if ((flags & XRNOTIFY_LEFT) == XRNOTIFY_LEFT) {
		XRSCREEN lastScreen(this->getScreen(this->getScreenCount() - 1));

		return lastScreen.setPointer(lastScreen.getWidth() - 2, y);
	} else if ((flags & XRNOTIFY_RIGHT) == XRNOTIFY_RIGHT) {
		XRSCREEN firstScreen(this->getScreen(0));

		return firstScreen.setPointer(1, y);
	}
	return true;
}

bool XRSERVER::release(int y, int flags) {
	XRLOCKER locker(&this->lock);

	// grab selection
	this->askSelection(XA_PRIMARY);
	this->askSelection(XA_SECONDARY);
	this->askSelection(this->getClipboardAtom());

	// reset buttons down
	for (map<int, bool>::iterator it = this->buttons.begin(); it != this->buttons.end(); ++it) {
		if (it->second) {
			XRNETPTREVENT event = {
				XREVENT_PTR_UP,
				it->first,
				0,
				0
			};
	
			this->processButtonEvent(&event);
		}
	}

	// reset keys down
	for (map<unsigned int, bool>::iterator it = this->keys.begin(); it != this->keys.end(); ++it) {
		if (it->second) {
			XRNETKBDEVENT event = {
				XREVENT_KBD_UP,
				it->first
			};

			this->processKbdEvent(&event);
		}
	}

	// start grab inputs
	if (!this->grabInput()) {
		return false;
	}
	return true;
}
