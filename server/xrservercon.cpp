/**
 * Xremote - server-side connection wrapper
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "../xremote.h"
#include "xrservercon.h"


XRSERVERCON::XRSERVERCON() : remoteAddress(INADDR_NONE), remotePort(0), alive(time(NULL)), endY(0), endFlags(0) {
}

XRSERVERCON::XRSERVERCON(in_addr_t remoteAddress, in_port_t remotePort) : remoteAddress(remoteAddress), remotePort(remotePort), alive(time(NULL)), endY(0), endFlags(0) {
}

XRSERVERCON::XRSERVERCON(const XRSERVERCON &ref) : remoteAddress(ref.remoteAddress), remotePort(ref.remotePort), alive(ref.alive), endY(ref.endY), endFlags(ref.endFlags) {
}

XRSERVERCON::~XRSERVERCON() {
}


in_addr_t XRSERVERCON::getRemoteAddress() const {
	return this->remoteAddress;
}

in_port_t XRSERVERCON::getRemotePort() const {
	return this->remotePort;
}

string XRSERVERCON::getRemoteHost() const {
	char buffer[32];
	int a = (this->remoteAddress >>  0) & 0xFF;
	int b = (this->remoteAddress >>  8) & 0xFF;
	int c = (this->remoteAddress >> 16) & 0xFF;
	int d = (this->remoteAddress >> 24) & 0xFF;

	sprintf(buffer, "%d.%d.%d.%d:%d", a, b, c, d, this->remotePort);
	return buffer;
}

time_t XRSERVERCON::getAlive() const {
	return this->alive;
}

void XRSERVERCON::setAlive() {
	this->alive = time(NULL);
}

void	 XRSERVERCON::setEnd(int y, int flags) {
	this->endY = y;
	this->endFlags = flags;
}

int XRSERVERCON::getEndY() const {
	return this->endY;
}

int XRSERVERCON::getEndFlags() const {
	return this->endFlags;
}
