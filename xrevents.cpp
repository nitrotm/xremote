/**
 * Xremote - event wrappers
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "xremote.h"


XREVENT::XREVENT(int type, in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort) : type(type), localPort(localPort), remoteAddress(remoteAddress), remotePort(remotePort) {
}

XREVENT::XREVENT(const XREVENT &ref) : type(ref.type), localPort(ref.localPort), remoteAddress(ref.remoteAddress), remotePort(ref.remotePort) {
}

XREVENT::XREVENT(PXRNETEVENT nev, in_port_t localPort, in_addr_t remoteAddress) : type(nev->type), localPort(localPort), remoteAddress(remoteAddress), remotePort(nev->localPort) {
}

XREVENT::~XREVENT() {
}


in_port_t XREVENT::getLocalPort() const {
	return this->localPort;
}

in_addr_t XREVENT::getRemoteAddress() const {
	return this->remoteAddress;
}

in_port_t XREVENT::getRemotePort() const {
	return this->remotePort;
}

string XREVENT::getRemoteHost() const {
	char buffer[32];
	int a = (this->remoteAddress >>  0) & 0xFF;
	int b = (this->remoteAddress >>  8) & 0xFF;
	int c = (this->remoteAddress >> 16) & 0xFF;
	int d = (this->remoteAddress >> 24) & 0xFF;

	sprintf(buffer, "%d.%d.%d.%d:%d", a, b, c, d, this->remotePort);
	return buffer;
}

unsigned char XREVENT::getType() const {
	return this->type;
}


void XREVENT::convert(PXRNETEVENT nev) const {
	nev->type = this->type;
	nev->r1 = 0;
	nev->localPort = this->localPort;
	nev->r2 = 0;
}



XRPTREVENT::XRPTREVENT(int type, in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, int x, int y) : XREVENT(type, localPort, remoteAddress, remotePort), x(x), y(y), button(0) {
}

XRPTREVENT::XRPTREVENT(int type, in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, int x, int y, int button) : XREVENT(type, localPort, remoteAddress, remotePort), x(x), y(y), button(button) {
}

XRPTREVENT::XRPTREVENT(const XRPTREVENT &ref) : XREVENT((const XREVENT &)ref), x(ref.x), y(ref.y), button(ref.button) {
}

XRPTREVENT::XRPTREVENT(PXRNETEVENT nev, in_port_t localPort, in_addr_t remoteAddress) : XREVENT(nev, localPort, remoteAddress) {
	PXRNETPTREVENT nptrev = (PXRNETPTREVENT)nev;

	this->x = nptrev->x;
	this->y = nptrev->y;
	this->button = nptrev->button;
}

XRPTREVENT::~XRPTREVENT() {
}


short XRPTREVENT::getX() const {
	return this->x;
}

short XRPTREVENT::getY() const {
	return this->y;
}

unsigned char XRPTREVENT::getButton() const {
	return this->button;
}


void XRPTREVENT::convert(PXRNETEVENT nev) const {
	PXRNETPTREVENT nptrev = (PXRNETPTREVENT)nev;

	nptrev->type = this->type;
	nptrev->button = this->button;
	nptrev->localPort = this->localPort;
	nptrev->x = this->x;
	nptrev->y = this->y;
}



XRKBDEVENT::XRKBDEVENT(int type, in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, int keycode) : XREVENT(type, localPort, remoteAddress, remotePort), keycode(keycode) {
}

XRKBDEVENT::XRKBDEVENT(const XRKBDEVENT &ref) : XREVENT((const XREVENT &)ref), keycode(ref.keycode) {
}

XRKBDEVENT::XRKBDEVENT(PXRNETEVENT nev, in_port_t localPort, in_addr_t remoteAddress) : XREVENT(nev, localPort, remoteAddress) {
	PXRNETKBDEVENT nkbdev = (PXRNETKBDEVENT)nev;

	this->keycode = nkbdev->keycode;
}

XRKBDEVENT::~XRKBDEVENT() {
}


unsigned int XRKBDEVENT::getKeyCode() const {
	return this->keycode;
}


void XRKBDEVENT::convert(PXRNETEVENT nev) const {
	PXRNETKBDEVENT nkbdev = (PXRNETKBDEVENT)nev;

	nkbdev->type = this->type;
	nkbdev->r1 = 0;
	nkbdev->localPort = this->localPort;
	nkbdev->keycode = this->keycode;
}



XRNOTIFYEVENT::XRNOTIFYEVENT(int type, unsigned short y, in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, int flags) : XREVENT(type, localPort, remoteAddress, remotePort), y(y), flags(flags) {
}

XRNOTIFYEVENT::XRNOTIFYEVENT(const XRNOTIFYEVENT &ref) : XREVENT((const XREVENT &)ref), y(ref.y), flags(ref.flags) {
}

XRNOTIFYEVENT::XRNOTIFYEVENT(const XRNOTIFYEVENT &ref, int flags) : XREVENT((const XREVENT &)ref), y(ref.y), flags(flags) {
}

XRNOTIFYEVENT::XRNOTIFYEVENT(PXRNETEVENT nev, in_port_t localPort, in_addr_t remoteAddress) : XREVENT(nev, localPort, remoteAddress) {
	PXRNETNOTIFYEVENT nnotifyev = (PXRNETNOTIFYEVENT)nev;

	this->y = nnotifyev->y;
	this->flags = nnotifyev->flags;
}

XRNOTIFYEVENT::~XRNOTIFYEVENT() {
}


unsigned short XRNOTIFYEVENT::getY() const {
	return this->y;
}

unsigned char XRNOTIFYEVENT::getFlags() const {
	return this->flags;
}


void XRNOTIFYEVENT::convert(PXRNETEVENT nev) const {
	PXRNETNOTIFYEVENT nnotifyev = (PXRNETNOTIFYEVENT)nev;

	nnotifyev->type = this->type;
	nnotifyev->flags = this->flags;
	nnotifyev->localPort = this->localPort;
	nnotifyev->r1 = 0;
	nnotifyev->y = this->y;
}



XRBUFFEREVENT::XRBUFFEREVENT(in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, unsigned char code, unsigned int size) : XREVENT(XREVENT_BUFFER, localPort, remoteAddress, remotePort), code(code), size(size) {
	this->count = (unsigned short)(this->size - this->size % sizeof(XRNETEVENT)) / sizeof(XRNETEVENT);
	if (this->size > 0) {
		this->count++;
	}
}

XRBUFFEREVENT::XRBUFFEREVENT(in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, unsigned char code, const string &str) : XREVENT(XREVENT_BUFFER, localPort, remoteAddress, remotePort), code(code), size(str.length()) {
	this->count = (unsigned short)(this->size - this->size % sizeof(XRNETEVENT)) / sizeof(XRNETEVENT);
	if (this->size > 0) {
		this->count++;
	}

	// build chunks
	unsigned int offset = 0;

	for (unsigned short i = 0; i < this->count; i++) {
		XRBUFFERCHUNK chunk;
		unsigned char chunksize = sizeof(XRNETEVENT);

		if ((offset + chunksize) > this->size) {
			chunksize = this->size - offset;
		}
		chunk.size = chunksize;
		chunk.sequence = i + 1;
		memcpy(chunk.data, str.c_str() + offset, chunksize);
		this->chunks.push_back(chunk);

		offset += chunksize;
	}
}

XRBUFFEREVENT::XRBUFFEREVENT(const XRBUFFEREVENT &ref) : XREVENT((const XREVENT &)ref), code(ref.code), size(ref.size), chunks(ref.chunks), count(ref.count) {
}

XRBUFFEREVENT::XRBUFFEREVENT(PXRNETEVENT nev, in_port_t localPort, in_addr_t remoteAddress) : XREVENT(nev, localPort, remoteAddress) {
	PXRNETBUFFEREVENT nbufferev = (PXRNETBUFFEREVENT)nev;

	this->code = nbufferev->code;
	this->size = nbufferev->size;
	this->count = (this->size - this->size % sizeof(XRNETEVENT)) / sizeof(XRNETEVENT);
	if (this->size > 0) {
		this->count++;
	}
}

XRBUFFEREVENT::~XRBUFFEREVENT() {
}


bool XRBUFFEREVENT::isComplete() const {
	return (this->count == this->chunks.size());
}

unsigned char XRBUFFEREVENT::getCode() const {
	return this->code;
}

unsigned int XRBUFFEREVENT::getSize() const {
	return this->size;
}


string XRBUFFEREVENT::getAsString() const {
	char *buffer = new char[this->size + 1];
	unsigned int offset = 0;
	unsigned int i = 0;

	while (offset < this->size && i < this->chunks.size()) {
		memcpy(buffer + offset, this->chunks[i].data, this->chunks[i].size);
		offset += this->chunks[i].size;
		i++;
	}
	buffer[offset] = 0;

	// return string
	string str(buffer);

	delete [] buffer;
	return str;
}


void XRBUFFEREVENT::setChunk(const XRBUFFERCHUNK &chunk) {
	this->chunks.push_back(chunk);
	sort(this->chunks.begin(), this->chunks.end());
}

const vector<XRBUFFERCHUNK> & XRBUFFEREVENT::getChunks() const {
	return chunks;
}


void XRBUFFEREVENT::convert(PXRNETEVENT nev) const {
	PXRNETBUFFEREVENT nbufferev = (PXRNETBUFFEREVENT)nev;

	nbufferev->type = this->type;
	nbufferev->code = this->code;
	nbufferev->localPort = this->localPort;
	nbufferev->size = this->size;
}
