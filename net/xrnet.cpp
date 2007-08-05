/**
 * Xremote - client/server commons
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "../xremote.h"
#include "../thirdparty/sha2.h"
#include "../thirdparty/aes.h"


XRNET::XRNET(const string &localHost, in_port_t localPort, const string &displayName) : XRWINDOW(displayName), encrypt(false), s(-1), primaryBufferEvent(NULL), secondaryBufferEvent(NULL), clipboardBufferEvent(NULL), lock(), localHost(localHost), localAddress(INADDR_NONE), localPort(localPort), remoteAddress(INADDR_NONE), remotePort(0) {
	memset(this->key, 0, sizeof(this->key));
}

XRNET::XRNET(const string &localHost, in_port_t localPort, const string &remoteHost, in_port_t remotePort, const string &displayName) : XRWINDOW(displayName), encrypt(false), s(-1), primaryBufferEvent(NULL), secondaryBufferEvent(NULL), clipboardBufferEvent(NULL), lock(), localHost(localHost), localAddress(INADDR_NONE), localPort(localPort), remoteHost(remoteHost), remoteAddress(INADDR_NONE), remotePort(remotePort) {
	memset(this->key, 0, sizeof(this->key));
}

XRNET::XRNET(const XRNET &ref) : XRWINDOW((const XRWINDOW &)ref), encrypt(false), s(-1), primaryBufferEvent(NULL), secondaryBufferEvent(NULL), clipboardBufferEvent(NULL), lock(), localHost(ref.localHost), localAddress(INADDR_NONE), localPort(ref.localPort), remoteHost(ref.remoteHost), remoteAddress(INADDR_NONE), remotePort(ref.remotePort) {
	memcpy(this->key, ref.key, sizeof(this->key));
}

XRNET::~XRNET() {
	this->destroySocket();
}


void XRNET::setEncryptionKey(const string &password) {
	XRLOCKER locker(&this->lock);

	if (password.length() > 0) {
		sha256_context ctx;

		memset(&ctx, 0, sizeof(sha256_context));
		sha256_starts(&ctx);
		sha256_update(&ctx, (uint8*)password.c_str(), password.length());
		sha256_finish(&ctx, this->key);
		this->encrypt = true;
	} else {
		this->encrypt = false;
	}
}


bool XRNET::createSocket() {
	XRLOCKER locker(&this->lock);

	if (!this->destroySocket()) {
		return false;
	}

	// resolve local address
	struct hostent *local = gethostbyname(this->localHost.c_str());

	if (local != NULL) {
		memcpy(&this->localAddress, local->h_addr, sizeof(this->localAddress));
	} else {
		this->localAddress = INADDR_ANY;
	}

	// resolve remote address
	if (this->remoteHost.length() > 0) {
		struct hostent *remote = gethostbyname(this->remoteHost.c_str());
	
		if (remote == NULL) {
			return false;
		}
		memcpy(&this->remoteAddress, remote->h_addr, sizeof(this->remoteAddress));
	} else {
		this->remoteAddress = INADDR_NONE;
	}

	// create socket
	this->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (this->s < 0) {
		return false;
	}

	// bind the socket
	struct sockaddr_in sin;

	sin.sin_family = AF_INET;
	sin.sin_port = htons(this->localPort);
	sin.sin_addr.s_addr = this->localAddress;
	if (bind(this->s, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
		return false;
	}

	// set in asynchronous mode
	int flags = fcntl(this->s, F_GETFL, 0);

	if (flags < 0) {
		return false;
	}
	if (fcntl(this->s, F_SETFL, flags | O_NONBLOCK) != 0) {
		return false;
	}
	return true;
}

bool XRNET::destroySocket() {
	XRLOCKER locker(&this->lock);

	this->freeReceiveEvents();
	this->freeSendEvents();
	if (this->primaryBufferEvent != NULL) {
		delete primaryBufferEvent;
		this->primaryBufferEvent = NULL;
	}
	if (this->secondaryBufferEvent != NULL) {
		delete secondaryBufferEvent;
		this->secondaryBufferEvent = NULL;
	}
	if (this->clipboardBufferEvent != NULL) {
		delete clipboardBufferEvent;
		this->clipboardBufferEvent = NULL;
	}
	if (this->s >= 0) {
		close(this->s);
	}
	this->s = -1;
	this->localAddress = INADDR_NONE;
	this->remoteAddress = INADDR_NONE;
	return true;
}


bool XRNET::receiveAll(long timeout) {
	XRLOCKER locker(&this->lock);

	// receive all events pending
	int code = this->receiveEvent(timeout);

	while (code > 0) {
		code = this->receiveEvent(0);
	}

	// dispatch events
	PXREVENT event = this->popReceiveEvent();

	while (event != NULL) {
		if (!this->onReceive(event)) {
			code = -10;
		}
		delete event;
		event = this->popReceiveEvent();
	}
	if (code >= 0) {
		return true;
	}
	return false;
}


void XRNET::send(PXREVENT event) {
	XRLOCKER locker(&this->lock);

	this->pushSendEvent(event);
}

bool XRNET::sendNow(PXREVENT event) {
	XRLOCKER locker(&this->lock);

	this->send(event);
	return this->sendAll();
}

bool XRNET::sendAll(long timeout) {
	XRLOCKER locker(&this->lock);

	// send all event in the queue
	int code = this->sendEvent(timeout);

	while (code > 0) {
		code = this->sendEvent(0);
	}
	if (code >= 0) {
		return true;
	}
	return false;
}


string XRNET::ipToString(in_addr_t address) {
	char buffer[32];
	int a = (address >>  0) & 0xFF;
	int b = (address >>  8) & 0xFF;
	int c = (address >> 16) & 0xFF;
	int d = (address >> 24) & 0xFF;

	sprintf(buffer, "%d.%d.%d.%d", a, b, c, d);
	return buffer;
}

string XRNET::hostToString(in_addr_t address, in_port_t port) {
	char buffer[32];
	int a = (address >>  0) & 0xFF;
	int b = (address >>  8) & 0xFF;
	int c = (address >> 16) & 0xFF;
	int d = (address >> 24) & 0xFF;

	sprintf(buffer, "%d.%d.%d.%d:%d", a, b, c, d, port);
	return buffer;
}


int XRNET::receiveEvent(long timeout) {
	XRLOCKER locker(&this->lock);

	if (this->s < 0) {
		return -1;
	}

	// check if some data are pending
	fd_set set;
	struct timeval t;

	FD_ZERO(&set);
	FD_SET(this->s, &set);
	t.tv_sec = 0;
	t.tv_usec = timeout;
	if (select(this->s + 1, &set, NULL, NULL, &t) > 0 && FD_ISSET(this->s, &set)) {
		// receive network event
		struct sockaddr_in sin;
		socklen_t	sinLength = sizeof(sin);
		int			rb;
		XRNETPACKET packet;

		memset(&sin, 0, sizeof(sin));
		rb = recvfrom(this->s, &packet, sizeof(XRNETPACKET), 0, (struct sockaddr *)&sin, &sinLength);
		if (rb != sizeof(XRNETPACKET)) {
			printf("rb=%d, from=%s !\n", rb, inet_ntoa(sin.sin_addr));
			return -2;
		}

		// decrypt packet
		XRNETDATA data;

		if (this->encrypt) {
			aes_context ctx;

			memset(&ctx, 0, sizeof(aes_context));
			aes_set_key(&ctx, this->key, sizeof(this->key) * 8);
			aes_decrypt(&ctx, (uint8*)&packet, (uint8*)&data);
		} else {
			memcpy(&data, &packet, sizeof(XRNETPACKET));
		}

		// compute checksum
		unsigned int	 chksum = 11;
		int			 half = sizeof(data.buffer) / 2;

		for (int i = 0; i < half; i++) {
			chksum += 11 * chksum + (data.buffer[i] ^ data.buffer[half + i]);
		}
		if (chksum != data.chksum) {
			printf("chksum failed (%08X != %08X) !\n", chksum, data.chksum);
			return 2;
		}

		XRNETEVENT nev;

		switch (data.code) {
		case XRNETDATA_CODE_EVENT:
			// convert native event
			memcpy(&nev, data.buffer, sizeof(XRNETEVENT));
			if ((nev.type & XREVENT_PTR) == XREVENT_PTR) {
				this->pushReceiveEvent(
					new XRPTREVENT(
						&nev,
						this->localPort,
						sin.sin_addr.s_addr
					)
				);
			} else if ((nev.type & XREVENT_KBD) == XREVENT_KBD) {
				this->pushReceiveEvent(
					new XRKBDEVENT(
						&nev,
						this->localPort,
						sin.sin_addr.s_addr
					)
				);
			} else if ((nev.type & XREVENT_BEGIN) == XREVENT_BEGIN) {
				this->pushReceiveEvent(
					new XRNOTIFYEVENT(
						&nev,
						this->localPort,
						sin.sin_addr.s_addr
					)
				);
			} else if ((nev.type & XREVENT_END) == XREVENT_END) {
				this->pushReceiveEvent(
					new XRNOTIFYEVENT(
						&nev,
						this->localPort,
						sin.sin_addr.s_addr
					)
				);
			} else if ((nev.type & XREVENT_ALIVE) == XREVENT_ALIVE) {
				this->pushReceiveEvent(
					new XRNOTIFYEVENT(
						&nev,
						this->localPort,
						sin.sin_addr.s_addr
					)
				);
			} else if ((nev.type & XREVENT_BUFFER) == XREVENT_BUFFER) {
				PXRBUFFEREVENT pbufferev = new XRBUFFEREVENT(
					&nev,
					this->localPort,
					sin.sin_addr.s_addr
				);

				switch (pbufferev->getCode()) {
				case XRNETDATA_CODE_PRIMARYBUFFER:
					if (this->primaryBufferEvent != NULL) {
						printf("previous primaryBufferEvent detected : %d [b] lost !\n", this->primaryBufferEvent->getSize());
						delete this->primaryBufferEvent;
					}
					this->primaryBufferEvent = pbufferev;
					if (this->primaryBufferEvent->isComplete()) {
						this->pushReceiveEvent(
							this->primaryBufferEvent
						);
						this->primaryBufferEvent = NULL;
					}
					break;

				case XRNETDATA_CODE_SECONDARYBUFFER:
					if (this->secondaryBufferEvent != NULL) {
						printf("previous secondaryBufferEvent detected : %d [b] lost !\n", this->secondaryBufferEvent->getSize());
						delete this->secondaryBufferEvent;
					}
					this->secondaryBufferEvent = pbufferev;
					if (this->secondaryBufferEvent->isComplete()) {
						this->pushReceiveEvent(
							this->secondaryBufferEvent
						);
						this->secondaryBufferEvent = NULL;
					}
					break;

				case XRNETDATA_CODE_CLIPBOARDBUFFER:
					if (this->clipboardBufferEvent != NULL) {
						printf("previous clipboardBufferEvent detected : %d [b] lost !\n", this->clipboardBufferEvent->getSize());
						delete this->clipboardBufferEvent;
					}
					this->clipboardBufferEvent = pbufferev;
					if (this->clipboardBufferEvent->isComplete()) {
						this->pushReceiveEvent(
							this->clipboardBufferEvent
						);
						this->clipboardBufferEvent = NULL;
					}
					break;

				default:
					printf("unsupported buffer event : unknown code (%d) !\n", pbufferev->getCode());
					delete pbufferev;
					return -3;
				}
			} else {
				printf("unsupported event (%d) !\n", nev.type);
				return -4;
			}
			break;

		case XRNETDATA_CODE_PRIMARYBUFFER:
			if (this->primaryBufferEvent == NULL) {
				printf("error receiving primaryBufferEvent packet : no previous declaration !\n");
				return -5;
			}
			this->primaryBufferEvent->setChunk(
				XRBUFFERCHUNK(
					data.size,
					data.sequence,
					data.buffer
				)
			);
			if (this->primaryBufferEvent->isComplete()) {
				this->pushReceiveEvent(
					this->primaryBufferEvent
				);
				this->primaryBufferEvent = NULL;
			}
			break;

		case XRNETDATA_CODE_SECONDARYBUFFER:
			if (this->secondaryBufferEvent == NULL) {
				printf("error receiving secondaryBufferEvent packet : no previous declaration !\n");
				return -5;
			}
			this->secondaryBufferEvent->setChunk(
				XRBUFFERCHUNK(
					data.size,
					data.sequence,
					data.buffer
				)
			);
			if (this->secondaryBufferEvent->isComplete()) {
				this->pushReceiveEvent(
					this->secondaryBufferEvent
				);
				this->secondaryBufferEvent = NULL;
			}
			break;

		case XRNETDATA_CODE_CLIPBOARDBUFFER:
			if (this->clipboardBufferEvent == NULL) {
				printf("error receiving clipboardBufferEvent packet : no previous declaration !\n");
				return -5;
			}
			this->clipboardBufferEvent->setChunk(
				XRBUFFERCHUNK(
					data.size,
					data.sequence,
					data.buffer
				)
			);
			if (this->clipboardBufferEvent->isComplete()) {
				this->pushReceiveEvent(
					this->clipboardBufferEvent
				);
				this->clipboardBufferEvent = NULL;
			}
			break;

		default:
			printf("error receiving packet : unknown code (%d) !\n", data.code);
			return -6;
		}
		return 1;
	}
	return 0;
}

PXREVENT XRNET::popReceiveEvent() {
	XRLOCKER locker(&this->lock);

	if (this->recvEvents.empty()) {
		return NULL;
	}
	PXREVENT event = this->recvEvents.front();

	this->recvEvents.pop_front();
	return event;
}

void XRNET::pushReceiveEvent(PXREVENT event) {
	XRLOCKER locker(&this->lock);

	this->recvEvents.push_back(event);
}

void XRNET::freeReceiveEvents() {
	XRLOCKER locker(&this->lock);
	PXREVENT event = this->popReceiveEvent();

	while(event != NULL) {
		delete event;
		event = this->popReceiveEvent();
	}
}


int XRNET::sendEvent(long timeout) {
	XRLOCKER locker(&this->lock);
	PXREVENT event = this->popSendEvent();

	if (event != NULL) {
		if (this->s < 0) {
			delete event;
			return -1;
		}
		if (!this->onSend(event)) {
			delete event;
			return 0;
		}

		// setup & send network event
		XRNETEVENT	nev;
		XRNETDATA	data;

		event->convert(&nev);
		data.code = XRNETDATA_CODE_EVENT;
		data.size = 8;
		data.sequence = 0;
		memcpy(data.buffer, &nev, sizeof(XRNETEVENT));
		if (this->sendDataPacket(timeout, event, &data) < 0) {
			delete event;
			return -2;
		}

		// send buffer chunks
		if ((event->getType() & XREVENT_BUFFER) == XREVENT_BUFFER) {
			PXRBUFFEREVENT pbufferev = (PXRBUFFEREVENT)event;
			const vector<XRBUFFERCHUNK> &chunks = pbufferev->getChunks();

			for (unsigned int i = 0; i < chunks.size(); i++) {
				data.code = pbufferev->getCode();
				data.size = chunks[i].size;
				data.sequence = chunks[i].sequence;
				memcpy(data.buffer, chunks[i].data, sizeof(XRNETEVENT));
				if (this->sendDataPacket(timeout, event, &data) < 0) {
					delete event;
					return -2;
				}
			}
		}
		delete event;
		return 1;
	}
	return 0;
}

int XRNET::sendDataPacket(long timeout, PXREVENT event, PXRNETDATA data) {
	// compute checksum
	int			 half = sizeof(data->buffer) / 2;
	unsigned int	 chksum = 11;

	for (int i = 0; i < half; i++) {
		chksum += 11 * chksum + (data->buffer[i] ^ data->buffer[half + i]);
	}
	data->chksum = chksum;

	// encrypt packet
	XRNETPACKET packet;

	if (this->encrypt) {
		aes_context ctx;

		memset(&ctx, 0, sizeof(aes_context));
		aes_set_key(&ctx, this->key, sizeof(this->key) * 8);
		aes_encrypt(&ctx, (uint8*)data, (uint8*)&packet);
	} else {
		memcpy(&packet, data, sizeof(XRNETPACKET));
	}

	// send network event
	fd_set set;
	struct timeval t;

	FD_ZERO(&set);
	FD_SET(this->s, &set);
	t.tv_sec = 0;
	t.tv_usec = timeout;
	if (select(this->s + 1, NULL, &set, NULL, &t) > 0 && FD_ISSET(this->s, &set)) {
		struct sockaddr_in sin;
		int sb;

		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(event->getRemotePort());
		sin.sin_addr.s_addr = event->getRemoteAddress();
		sb = sendto(this->s, &packet, sizeof(XRNETPACKET), 0, (struct sockaddr *)&sin, sizeof(sin));
		if (sb != sizeof(XRNETPACKET)) {
			printf("sb=%d, to=%s !\n", sb, inet_ntoa(sin.sin_addr));
			return -2;
		}
		return 1;
	}
	return -3;
}

PXREVENT XRNET::popSendEvent() {
	XRLOCKER locker(&this->lock);

	if (this->sendEvents.empty()) {
		return NULL;
	}
	PXREVENT event = this->sendEvents.front();

	this->sendEvents.pop_front();
	return event;
}

void XRNET::pushSendEvent(PXREVENT event) {
	XRLOCKER locker(&this->lock);

	this->sendEvents.push_back(event);
}

void XRNET::freeSendEvents() {
	XRLOCKER locker(&this->lock);
	PXREVENT event = this->popSendEvent();

	while(event != NULL) {
		delete event;
		event = this->popSendEvent();
	}
}
