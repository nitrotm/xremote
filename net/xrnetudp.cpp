/**
 * Xremote - client/server low-level udp socket
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "../xremote.h"


XRNETUDP::XRNETUDP(PXRNETLISTENER lower, PXRNETLISTENER upper, const int packetSize, const string &localHost, const in_port_t localPort) :
	XRNETBASE(lower, upper),
	packetSize(packetSize),
	localHost(localHost),
	localAddress(INADDR_NONE),
	localPort(localPort),
	remoteAddress(INADDR_NONE),
	remotePort(0),
	s(-1) {
}

XRNETUDP::XRNETUDP(PXRNETLISTENER lower, PXRNETLISTENER upper, const int packetSize, const string &localHost, const in_port_t localPort, const string &remoteHost, const in_port_t remotePort) :
	XRNETBASE(lower, upper),
	packetSize(packetSize),
	localHost(localHost),
	localAddress(INADDR_NONE),
	localPort(localPort),
	remoteHost(remoteHost),
	remoteAddress(INADDR_NONE),
	remotePort(remotePort),
	s(-1) {
}

XRNETUDP::XRNETUDP(const XRNETUDP &ref) :
	XRNETBASE((const XRNETBASE &)ref),
	localHost(ref.localHost),
	localAddress(INADDR_NONE),
	localPort(ref.localPort),
	remoteHost(ref.remoteHost),
	remoteAddress(INADDR_NONE),
	remotePort(ref.remotePort),
	s(-1) {
}

XRNETUDP::~XRNETUDP() {
	this->destroySocket();
}


int XRNETUDP::getRawSize() const {
	return this->packetSize - this->getRawSize(this->lower);
}

bool XRNETUDP::createSocket() {
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

bool XRNETUDP::destroySocket() {
	if (this->s >= 0) {
		close(this->s);
		this->s = -1;
	}
	this->localAddress = INADDR_NONE;
	this->remoteAddress = INADDR_NONE;
	return true;
}


int XRNETUDP::receiveOne(XRNETPACKET **packet, const long timeout) {
	*packet = NULL;
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
		// receive network packet
		struct sockaddr_in	sin;
		socklen_t			sinLength = sizeof(sin);
		int					rb;
		XRNETBUFFER			buffer(this->packetSize);

		memset(&sin, 0, sizeof(sin));
		rb = recvfrom(this->s, buffer.getPtr(), this->packetSize, 0, (struct sockaddr *)&sin, &sinLength);
		if (rb != this->packetSize) {
			printf("rb=%d, from=%s !\n", rb, inet_ntoa(sin.sin_addr));
			return -2;
		}

//		printf("some data are available ;) wooowwww\n");

		// push packet on queue
		*packet = new XRNETPACKET(this->localAddress, this->localPort, sin.sin_addr.s_addr, sin.sin_port, buffer);
		return 1;
	}
	printf("no data are available :(\n");
	return 0;
}


int XRNETUDP::sendOne(const XRNETPACKET &packet, const long timeout) {
	if (this->s < 0) {
		return -1;
	}
	if (packet.getBuffer().getSize() != this->packetSize) {
		return -10;
	}

	// send network packet
	fd_set set;
	struct timeval t;

	FD_ZERO(&set);
	FD_SET(this->s, &set);
	t.tv_sec = 0;
	t.tv_usec = timeout;
	if (select(this->s + 1, NULL, &set, NULL, &t) > 0 && FD_ISSET(this->s, &set)) {
		struct sockaddr_in sin;
		int sb;

//		printf("sending to %d:%d\n", packet.getRemoteAddress(), packet.getRemotePort());

		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = packet.getRemoteAddress();
		sin.sin_port = htons(packet.getRemotePort());
		sb = sendto(this->s, packet.getBuffer().getPtr(), this->packetSize, 0, (struct sockaddr *)&sin, sizeof(sin));
		if (sb != this->packetSize) {
			printf("sb=%d, to=%s !\n", sb, inet_ntoa(sin.sin_addr));
			return -3;
		}
		return 1;
	}
	printf("cannot send to %d:%d\n", packet.getRemoteAddress(), packet.getRemotePort());
	return 0;
}

int XRNETUDP::getRawSize(PXRNETLISTENER listener) const {
	if (listener != NULL) {
		if (listener->getUpper() != NULL) {
			return listener->getHeaderSize() + this->getRawSize(listener->getUpper());
		}
		return listener->getHeaderSize();
	}
	return 0;
}
