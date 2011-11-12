/**
 * Xremote - client/server low-level udp socket
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "xremote.h"
#include "xrnet.h"


XRNETUDP::XRNETUDP(int packetSize, XRNETFILTER *first, const string &localHost, const in_port_t localPort, const string &remoteHost, const in_port_t remotePort) :
	XRNET(packetSize, first),
	meta(
		XRNETUDP::resolveHost(localHost, INADDR_ANY),
		localPort,
		XRNETUDP::resolveHost(remoteHost, INADDR_NONE),
		remotePort
	),
	localHost(localHost),
	remoteHost(remoteHost),
	s(-1) {
}


XRNETUDP::~XRNETUDP() {
	this->destroySocket();
}

bool XRNETUDP::createSocket() {
	if (!this->destroySocket()) {
		return false;
	}

	// create socket
	this->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (this->s < 0) {
		return false;
	}

	// bind the socket
	struct sockaddr_in sin;

	sin.sin_family = AF_INET;
	sin.sin_port = htons(this->meta.getLocalPort());
	sin.sin_addr.s_addr = this->meta.getLocalAddress();
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
	return true;
}

int XRNETUDP::read(XRNETPACKET **packet, long timeout) {
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
	if (select(this->s + 1, &set, NULL, NULL, &t) <= 0 || !FD_ISSET(this->s, &set)) {
		return 0;
	}

	// receive network packet
	socklen_t sinLength = sizeof(struct sockaddr_in);
	struct sockaddr_in sin;
	XRNETBUFFER buffer(this->getPacketSize());
	int rb = 0;

	memset(&sin, 0, sizeof(sin));
	rb = recvfrom(this->s, buffer, this->getPacketSize(), 0, (struct sockaddr *)&sin, &sinLength);
	if (rb != this->getPacketSize()) {
		printf("ERR: size=%d, rb=%d, from=%s !\n", this->getPacketSize(), rb, inet_ntoa(sin.sin_addr));
		return -2;
	}

	// push packet on queue
	*packet = new XRNETPACKET(this->meta.getLocalAddress(), this->meta.getLocalPort(), sin.sin_addr.s_addr, sin.sin_port, buffer);
	return 1;
}

int XRNETUDP::write(const XRNETPACKET *packet, long timeout) {
	if (this->s < 0) {
		return -1;
	}

	// check network availability
	fd_set set;
	struct timeval t;

	FD_ZERO(&set);
	FD_SET(this->s, &set);
	t.tv_sec = 0;
	t.tv_usec = timeout;
	if (select(this->s + 1, NULL, &set, NULL, &t) <= 0 || !FD_ISSET(this->s, &set)) {
		return 0;
	}

	// check packet
	if (packet->length() != this->getPacketSize()) {
		printf("ERR: invalid packet: size=%d!\n", packet->length());
		return -10;
	}

	// send network packet
	struct sockaddr_in sin;
	int sb = 0;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = packet->getRemoteAddress();
	sin.sin_port = htons(packet->getRemotePort());
	sb = sendto(this->s, packet->getBuffer(), this->getPacketSize(), 0, (struct sockaddr *)&sin, sizeof(sin));
	if (sb != this->getPacketSize()) {
		printf("ERR: size=%d, sb=%d, to=%s !\n", this->getPacketSize(), sb, inet_ntoa(sin.sin_addr));
		return -3;
	}
	return 1;
}

in_addr_t XRNETUDP::resolveHost(const string &host, const in_addr_t &defaultAddress) {
	// resolve local address
	struct hostent *local = gethostbyname(host.c_str());
	
	if (local != NULL) {
		in_addr_t address;

		memcpy(&address, local->h_addr, sizeof(address));
		return address;
	}
	return defaultAddress;
}
