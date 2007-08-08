/**
 * Xremote - client/server low-level common api
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "../xremote.h"


XRNETBUFFER::XRNETBUFFER() : size(0), buffer(NULL) {
}

XRNETBUFFER::XRNETBUFFER(const int size) : size(size) {
	this->buffer = new unsigned char[size];
	memset(this->buffer, 0, size);
}

XRNETBUFFER::XRNETBUFFER(const int size, const void *buffer) : size(size) {
	this->buffer = new unsigned char[size];
	memcpy(this->buffer, buffer, size);
}

XRNETBUFFER::XRNETBUFFER(const XRNETBUFFER &ref) : size(ref.size) {
	this->buffer = new unsigned char[ref.size];
	memcpy(this->buffer, ref.buffer, ref.size);
}

XRNETBUFFER::~XRNETBUFFER() {
	if (this->buffer != NULL) {
		delete [] this->buffer;
	}
}

const XRNETBUFFER & XRNETBUFFER::operator =(const XRNETBUFFER &ref) {
	if (this->buffer != NULL) {
		delete [] this->buffer;
	}
	this->size = ref.size;
	this->buffer = new unsigned char[ref.size];
	memcpy(this->buffer, ref.buffer, ref.size);
	return *this;
}

const unsigned char & XRNETBUFFER::operator [](int offset) const {
	return this->buffer[offset];
}

unsigned char * XRNETBUFFER::getPtr() const {
	return this->buffer;
}

unsigned char * XRNETBUFFER::getPtr(const int offset) const {
	return &this->buffer[offset];
}

int XRNETBUFFER::getSize() const {
	return this->size;
}

void XRNETBUFFER::getData(void *buffer) const {
	memcpy(buffer, this->buffer, this->size);
}

void XRNETBUFFER::getData(const int size, void *buffer) const {
	if (size <= this->size) {
		memcpy(buffer, this->buffer, size);
	} else {
		memcpy(buffer, this->buffer, this->size);
	}
}

void XRNETBUFFER::getData(const int offset, const int size, void *buffer) const {
	if (offset < this->size) {
		if ((offset + size) <= this->size) {
			memcpy(buffer, &this->buffer[offset], size);
		} else {
			memcpy(buffer, &this->buffer[offset], this->size - offset);
		}
	}
}

void XRNETBUFFER::setData(const void *buffer) {
	memcpy(this->buffer, buffer, this->size);
}

void XRNETBUFFER::setData(const int size, const void *buffer) {
	if (size <= this->size) {
		memcpy(this->buffer, buffer, size);
	} else {
		memcpy(this->buffer, buffer, this->size);
	}
}

void XRNETBUFFER::setData(const int offset, const int size, const void *buffer) {
	if (offset < this->size) {
		if ((offset + size) <= this->size) {
			memcpy(&this->buffer[offset], buffer, size);
		} else {
			memcpy(&this->buffer[offset], buffer, this->size - offset);
		}
	}
}



XRNETPACKETMETA::XRNETPACKETMETA(const in_addr_t &localAddress, const in_port_t localPort, const in_addr_t &remoteAddress, const in_port_t remotePort) :
	timestamp(time(NULL)),
	localAddress(localAddress),
	localPort(localPort),
	remoteAddress(remoteAddress),
	remotePort(remotePort) {
}

XRNETPACKETMETA::XRNETPACKETMETA(const XRNETPACKETMETA &ref) :
	timestamp(ref.timestamp),
	localAddress(ref.localAddress),
	localPort(ref.localPort),
	remoteAddress(ref.remoteAddress),
	remotePort(ref.remotePort) {
}

XRNETPACKETMETA::~XRNETPACKETMETA() {
}

const time_t & XRNETPACKETMETA::getTimestamp() const {
	return this->timestamp;
}

const in_addr_t & XRNETPACKETMETA::getLocalAddress() const {
	return this->localAddress;
}

const in_port_t & XRNETPACKETMETA::getLocalPort() const {
	return this->localPort;
}

const in_addr_t & XRNETPACKETMETA::getRemoteAddress() const {
	return this->remoteAddress;
}

const in_port_t & XRNETPACKETMETA::getRemotePort() const {
	return this->remotePort;
}



XRNETPACKET::XRNETPACKET(const in_addr_t &localAddress, const in_port_t localPort, const in_addr_t &remoteAddress, const in_port_t remotePort, const XRNETBUFFER &buffer) :
	XRNETPACKETMETA(localAddress, localPort, remoteAddress, remotePort),
	buffer(buffer) {
}

XRNETPACKET::XRNETPACKET(const XRNETPACKETMETA &meta, const XRNETBUFFER &buffer) :
	XRNETPACKETMETA(meta),
	buffer(buffer) {
}

XRNETPACKET::XRNETPACKET(const XRNETPACKET &ref) :
	XRNETPACKETMETA((const XRNETPACKETMETA &)ref),
	buffer(ref.buffer) {
}

XRNETPACKET::~XRNETPACKET() {
}

const XRNETBUFFER & XRNETPACKET::getBuffer() const {
	return this->buffer;
}

void XRNETPACKET::setBuffer(const XRNETBUFFER &buffer) {
	this->buffer = buffer;
}



XRNETLISTENER::XRNETLISTENER() : lower(NULL), upper(NULL) {
}

XRNETLISTENER::~XRNETLISTENER() {
}

PXRNETLISTENER XRNETLISTENER::getLower() const {
	return this->lower;
}

PXRNETLISTENER XRNETLISTENER::getUpper() const {
	return this->upper;
}

void XRNETLISTENER::setLower(PXRNETLISTENER lower) {
	this->lower = lower;
}

void XRNETLISTENER::setUpper(PXRNETLISTENER upper) {
	this->upper = upper;
}

bool XRNETLISTENER::onReceivePacket(const XRNETPACKET &packet) {
	if (this->upper != NULL) {
		return this->upper->onReceivePacket(packet);
	}
	return true;
}

bool XRNETLISTENER::onSendPacket(XRNETPACKET &packet) {
	if (this->lower != NULL) {
		return this->lower->onSendPacket(packet);
	}
	return true;
}



XRNETBASE::XRNETBASE(PXRNETLISTENER lower, PXRNETLISTENER upper) : lower(lower), upper(upper) {
}

XRNETBASE::XRNETBASE(const XRNETBASE &ref) : lower(ref.lower), upper(ref.upper) {
}

XRNETBASE::~XRNETBASE() {
	this->sendBuffer.clear();
}


bool XRNETBASE::receiveAll(const long timeout) {
	XRLOCKER locker(&this->lock);

	// receive all packet pending
	XRNETPACKET	*packet = NULL;
	int			code = this->receiveOne(&packet, timeout);

	while (code > 0) {
		if (this->lower != NULL) {
			this->lower->onReceivePacket(*packet);
		}
		delete packet;
		code = this->receiveOne(&packet, 0);
	}
	if (code >= 0) {
		return true;
	}
	return false;
}


bool XRNETBASE::send(const XRNETPACKET &packet) {
	XRLOCKER locker(&this->lock);

	this->sendBuffer.push_back(packet);
	return true;
}

bool XRNETBASE::sendNow(const XRNETPACKET &packet) {
	XRLOCKER locker(&this->lock);

	this->sendBuffer.push_back(packet);
	return this->sendAll();
}

bool XRNETBASE::sendAll(const long timeout) {
	XRLOCKER locker(&this->lock);

	// send all packet pending
	while (!this->sendBuffer.empty()) {
		XRNETPACKET packet = this->sendBuffer.front();

//		printf("aa\n");
		if (this->upper != NULL) {
//			printf("b1\n");
			if (this->upper->onSendPacket(packet)) {
				int code = this->sendOne(packet, timeout);

				if (code < 0) {
					return false;
				}
			}
//			printf("b2\n");
		} else {
//			printf("b3\n");

			int code = this->sendOne(packet, timeout);

			if (code < 0) {
				return false;
			}
		}
		this->sendBuffer.pop_front();
//		printf("cc\n");
	}
	return true;
}
