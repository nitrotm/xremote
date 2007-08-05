/**
 * Xremote - client/server commons
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "../xremote.h"


XRNETRAWPACKET::XRNETRAWPACKET(in_addr_t localAddress, in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, unsigned char *buffer) :
	timestamp(time(NULL)),
	localAddress(localAddress),
	localPort(localPort),
	remoteAddress(remoteAddress),
	remotePort(remotePort) {
	memcpy(this->buffer, buffer, XREMOTE_PACKET_SIZE);
}

XRNETRAWPACKET::XRNETRAWPACKET(const XRNETRAWPACKET &ref) :
	timestamp(ref.timestamp),
	localAddress(ref.localAddress),
	localPort(ref.localPort),
	remoteAddress(ref.remoteAddress),
	remotePort(ref.remotePort) {
	memcpy(this->buffer, ref.buffer, XREMOTE_PACKET_SIZE);

}

XRNETRAWPACKET::~XRNETRAWPACKET() {
}

unsigned char * XRNETRAWPACKET::getBuffer() {
	return this->buffer;
}

int XRNETRAWPACKET::getSize() {
	return XREMOTE_PACKET_SIZE;
}

const time_t & XRNETRAWPACKET::getTimestamp() {
	return this->timestamp;
}

const in_addr_t & XRNETRAWPACKET::getLocalAddress() {
	return this->localAddress;
}

const in_port_t & XRNETRAWPACKET::getLocalPort() {
	return this->localPort;
}

const in_addr_t & XRNETRAWPACKET::getRemoteAddress() {
	return this->remoteAddress;
}

const in_port_t & XRNETRAWPACKET::getRemotePort() {
	return this->remotePort;
}



XRNETLISTENER::XRNETLISTENER(XRNETLISTENER *listener) : listener(listener) {
}

XRNETLISTENER::~XRNETLISTENER() {
}

bool XRNETLISTENER::onReceivePacket(PXRNETRAWPACKET packet) {
	if (this->listener != NULL) {
		return this->listener->onReceivePacket(packet);
	}
	return true;
}

PXRNETRAWPACKET XRNETLISTENER::onSendPacket(PXRNETRAWPACKET packet) {
	if (this->listener != NULL) {
		return this->listener->onSendPacket(packet);
	}
	return packet;
}




XRNETBASE::XRNETBASE(PXRNETLISTENER listener) : listener(listener) {
}

XRNETBASE::XRNETBASE(const XRNETBASE &ref) : listener(ref.listener) {
}

XRNETBASE::~XRNETBASE() {
	while (!this->sendBuffer.empty()) {
		delete this->sendBuffer.front();
		this->sendBuffer.pop_front();
	}
	this->listener = NULL;
}


bool XRNETBASE::receiveAll(long timeout) {
	XRLOCKER locker(&this->lock);

	// receive all packet pending
	PXRNETRAWPACKET packet = NULL;
	int code = this->receiveOne(&packet, timeout);

	while (code > 0) {
		if (this->listener != NULL) {
			this->listener->onReceivePacket(packet);
		}
		delete packet;

		code = this->receiveOne(&packet, 0);
	}
	if (code >= 0) {
		return true;
	}
	return false;
}


bool XRNETBASE::send(PXRNETRAWPACKET packet) {
	XRLOCKER locker(&this->lock);

	this->sendBuffer.push_back(packet);
	return true;
}

bool XRNETBASE::sendNow(PXRNETRAWPACKET packet) {
	XRLOCKER locker(&this->lock);

	this->sendBuffer.push_back(packet);
	return this->sendAll();
}

bool XRNETBASE::sendAll(long timeout) {
	XRLOCKER locker(&this->lock);

	// send all packet pending
	while (!this->sendBuffer.empty()) {
		PXRNETRAWPACKET packet = this->sendBuffer.front();

		if (this->listener != NULL) {
			packet = this->listener->onSendPacket(packet);
			if (packet != NULL) {
				int code = this->sendOne(packet, timeout);

				if (code < 0) {
					return false;
				}
				delete packet;
			}
		} else {
			int code = this->sendOne(packet, timeout);

			if (code < 0) {
				return false;
			}
			delete packet;
		}
		this->sendBuffer.pop_front();
	}
	return true;
}
