/**
 * Xremote - server-side
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "../xremote.h"
#include "xrservercon.h"
#include "xrserver.h"


XRSERVER::XRSERVER(const string &localHost, in_port_t localPort, const string &displayName) : XRNET(localHost, localPort, displayName), allows(false), bufferCount(0) {
}

XRSERVER::XRSERVER(const XRSERVER &ref) : XRNET((const XRNET &)ref), allows(false), bufferCount(0) {
}

XRSERVER::~XRSERVER() {
}


bool XRSERVER::main() {
	XRLOCKER locker(&this->lock);
	bool		 code = true;

	if (!this->isSupported()) {
		printf("xremote: error in isSupported()\n");
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
//		return false;
	}

	// main loop
//	printf("xremote: server main loop begin\n");
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
			for (size_t i = 0; i < this->connections.size(); i++) {
				if ((time(NULL) - this->connections[i].getAlive()) >= XREMOTE_DEAD_CHECK) {
					this->send(
						new XRNOTIFYEVENT(
							XREVENT_END,
							0,
							this->localPort,
							this->connections[i].getRemoteAddress(),
							this->connections[i].getRemotePort(),
							0
						)
					);
					this->unregisterConnection(this->connections[i].getRemoteAddress(), this->connections[i].getRemotePort());
//					this->connections.erase(this->connections.begin() + i);
					break;
				} else if ((time(NULL) - this->connections[i].getAlive()) >= XREMOTE_ALIVE_CHECK) {
					this->send(
						new XRNOTIFYEVENT(
							XREVENT_ALIVE,
							0,
							this->localPort,
							this->connections[i].getRemoteAddress(),
							this->connections[i].getRemotePort(),
							0
						)
					);
				}
			}
		}
		if (this->connections.size() == 0) {
			this->end(0, 0);
		}
	}
	this->allows = false;
//	printf("xremote: server main loop end\n");

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

void XRSERVER::shutdown() {
	XRLOCKER locker(&this->lock);

	this->allows = false;
}


bool XRSERVER::onReceive(PXREVENT event) {
	XRLOCKER locker(&this->lock);

	if (this->hasConnection(event->getRemoteAddress(), event->getRemotePort())) {
		this->setConnectionAlive(event->getRemoteAddress(), event->getRemotePort());

		if ((event->getType() & XREVENT_BEGIN) == XREVENT_BEGIN) {
			PXRNOTIFYEVENT notifyev = (PXRNOTIFYEVENT)event;

			if ((notifyev->getFlags() & XRNOTIFY_REPLY) != XRNOTIFY_REPLY) {
				this->send(
					new XRNOTIFYEVENT(
						(const XRNOTIFYEVENT &)*event,
						notifyev->getFlags() | XRNOTIFY_REPLY
					)
				);
			}
		} else if ((event->getType() & XREVENT_END) == XREVENT_END) {
			PXRNOTIFYEVENT notifyev = (PXRNOTIFYEVENT)event;

			if ((notifyev->getFlags() & XRNOTIFY_REPLY) != XRNOTIFY_REPLY) {
				this->send(
					new XRNOTIFYEVENT(
						(const XRNOTIFYEVENT &)*event,
						notifyev->getFlags() | XRNOTIFY_REPLY
					)
				);
			} else {
				this->unregisterConnection(event->getRemoteAddress(), event->getRemotePort());
			}
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
		} else {
			if (!this->processEvent(event)) {
				printf("xremote: error in processEvent()\n");
				return false;
			}
		}
	} else if ((event->getType() & XREVENT_BEGIN) == XREVENT_BEGIN) {
		PXRNOTIFYEVENT notifyev = (PXRNOTIFYEVENT)event;

		if (!this->begin(notifyev->getY(), notifyev->getFlags())) {
			printf("xremote: error in begin()\n");
		}
		this->send(
			new XRNOTIFYEVENT(
				(const XRNOTIFYEVENT &)*event,
				notifyev->getFlags() | XRNOTIFY_REPLY
			)
		);
		this->registerConnection(event->getRemoteAddress(), event->getRemotePort());
	}
	return true;
}

bool XRSERVER::onSend(PXREVENT event) {
	XRLOCKER locker(&this->lock);

	if (this->hasConnection(event->getRemoteAddress(), event->getRemotePort())) {
		if ((event->getType() & XREVENT_BEGIN) == XREVENT_BEGIN) {
			return true;
		} else if ((event->getType() & XREVENT_END) == XREVENT_END) {
			return true;
		} else if ((event->getType() & XREVENT_ALIVE) == XREVENT_ALIVE) {
			return true;
		} else if ((event->getType() & XREVENT_BUFFER) == XREVENT_BUFFER) {
			return true;
		}
	} else if ((event->getType() & XREVENT_BEGIN) == XREVENT_BEGIN) {
		return true;
	}
	printf("xremote: unknown message (to %s)\n", event->getRemoteHost().c_str());
	return false;
}


bool XRSERVER::processEvent(PXREVENT event) {
	XRLOCKER locker(&this->lock);

	// fetch up/down state
	bool up = false;
	bool down = false;

	if ((event->getType() & XREVENT_UP) == XREVENT_UP) {
		up = true;
	}
	if ((event->getType() & XREVENT_DOWN) == XREVENT_DOWN) {
		down = true;
	}

	// dispatch by message type
	if ((event->getType() & XREVENT_PTR) == XREVENT_PTR) {
		PXRPTREVENT ptrev = (PXRPTREVENT)event;

		if (!this->processPtrEvent(ptrev)) {
			printf("xremote: error in processPtrEvent()\n");
			return false;
		}
		if (up) {
			for (size_t i = 0; i < this->buttons.size(); i++) {
				if (this->buttons[i] == ptrev->getButton()) {
					this->buttons.erase(this->buttons.begin() + i);
					break;
				}
			}
			return this->processButtonEvent(ptrev, false);
		} else if (down) {
			bool found = false;

			for (size_t i = 0; i < this->buttons.size(); i++) {
				if (this->buttons[i] == ptrev->getButton()) {
					found = true;
					break;
				}
			}
			if (!found) {
				this->buttons.push_back(ptrev->getButton());
			}
			return this->processButtonEvent(ptrev, true);
		}
		return true;
	} else if ((event->getType() & XREVENT_KBD) == XREVENT_KBD) {
		PXRKBDEVENT kbdev = (PXRKBDEVENT)event;

		if (up) {
			for (size_t i = 0; i < this->keys.size(); i++) {
				if (this->keys[i] == kbdev->getKeyCode()) {
					this->keys.erase(this->keys.begin() + i);
					break;
				}
			}
			return this->processKbdEvent(kbdev, false);
		} else if (down) {
			bool found = false;

			for (size_t i = 0; i < this->keys.size(); i++) {
				if (this->keys[i] == kbdev->getKeyCode()) {
					found = true;
					break;
				}
			}
			if (!found) {
				this->keys.push_back(kbdev->getKeyCode());
			}
			return this->processKbdEvent(kbdev, true);
		}
	}
	printf("xremote: unknown message (from %s)\n", event->getRemoteHost().c_str());
	return false;
}

bool XRSERVER::sendEnd(int y, int flags) {
	XRLOCKER locker(&this->lock);

	this->bufferCount = 0;
	for (size_t i = 0; i < this->connections.size(); i++) {
		this->connections[i].setEnd(y, flags);
	}
	this->askSelection(XA_PRIMARY);
	this->askSelection(XA_SECONDARY);
	this->askSelection(this->XA_CLIPBOARD);
	return true;
}

bool XRSERVER::processXEvent(const XEvent &xev) {
	XRLOCKER locker(&this->lock);

	// process clipboard events
	unsigned char code = 0;

	switch (xev.type) {
	case SelectionNotify:
		if (xev.xselection.selection == XA_PRIMARY) {
			code = XRNETDATA_CODE_PRIMARYBUFFER;
		} else if (xev.xselection.selection == XA_SECONDARY) {
			code = XRNETDATA_CODE_SECONDARYBUFFER;
		} else if (xev.xselection.selection == this->XA_CLIPBOARD) {
			code = XRNETDATA_CODE_CLIPBOARDBUFFER;
		}
		if (code > 0) {
			string text = this->getSelection(xev.xselection.selection, xev.xselection.property);

			this->bufferCount++;
			for (size_t i = 0; i < this->connections.size(); i++) {
				this->send(
					new XRBUFFEREVENT(
						this->localPort,
						this->connections[i].getRemoteAddress(),
						this->connections[i].getRemotePort(),
						code,
						text
					)
				);
			}

			// clear clipboard
			this->setSelection(xev.xselection.selection, "");
		}
		if (this->bufferCount == 3) {
			for (size_t i = 0; i < this->connections.size(); i++) {
				this->send(
					new XRNOTIFYEVENT(
						XREVENT_END,
						this->connections[i].getEndY(),
						this->localPort,
						this->connections[i].getRemoteAddress(),
						this->connections[i].getRemotePort(),
						this->connections[i].getEndFlags()
					)
				);
			}
		}
		break;

	case SelectionRequest:
		this->sendSelectionNotify(xev);
		break;

	case SelectionClear:
		this->clearSelection(xev.xselectionclear.selection);
		break;
	}
	return this->sendAll();
}


bool XRSERVER::setConnectionAlive(in_addr_t address, in_port_t port) {
	XRLOCKER locker(&this->lock);

	for (size_t i = 0; i < this->connections.size(); i++) {
		if (this->connections[i].getRemoteAddress() == address &&
			this->connections[i].getRemotePort() == port) {
			this->connections[i].setAlive();
			return true;
		}
	}
	return false;
}

void XRSERVER::registerConnection(in_addr_t address, in_port_t port) {
	XRLOCKER locker(&this->lock);

	if (!this->hasConnection(address, port)) {
		this->connections.push_back(XRSERVERCON(address, port));
	}
}

void XRSERVER::unregisterConnection(in_addr_t address, in_port_t port) {
	XRLOCKER locker(&this->lock);

	for (size_t i = 0; i < this->connections.size(); i++) {
		if (this->connections[i].getRemoteAddress() == address &&
			this->connections[i].getRemotePort() == port) {
			this->connections.erase(this->connections.begin() + i);
			break;
		}
	}
}

bool XRSERVER::hasConnection(in_addr_t address, in_port_t port) {
	XRLOCKER locker(&this->lock);

	for (size_t i = 0; i < this->connections.size(); i++) {
		if (this->connections[i].getRemoteAddress() == address &&
			this->connections[i].getRemotePort() == port) {
			return true;
		}
	}
	return false;
}


bool XRSERVER::begin(int y, int flags) {
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

bool XRSERVER::end(int y, int flags) {
	XRLOCKER locker(&this->lock);

	// reset buttons down
	for (size_t i = 0; i < this->buttons.size(); i++) {
		XRPTREVENT pev(
			XREVENT_PTR | XREVENT_UP,
			0,
			0,
			0,
			0,
			0,
			this->buttons[i]
		);
		this->processButtonEvent(&pev, false);
	}
	this->buttons.clear();

	// reset keys down
	for (size_t i = 0; i < this->keys.size(); i++) {
		XRKBDEVENT kev(
			XREVENT_KBD | XREVENT_UP,
			0,
			0,
			0,
			this->keys[i]
		);
		this->processKbdEvent(&kev, false);
	}
	this->keys.clear();

	// start grab inputs
	if (!this->grabInput()) {
		return false;
	}
	return true;
}
