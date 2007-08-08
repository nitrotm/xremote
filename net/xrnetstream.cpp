/**
 * Xremote - user packet streamer
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "../xremote.h"


typedef struct {
	unsigned char  id;		// buffer id
	unsigned short seq;		// buffer sequence
	unsigned char  size;		// packet size
} XRNETPACKETHEADER;

typedef struct {
	unsigned int size;		// buffer size
	unsigned int checksum;	// buffer checksum
	unsigned int count;		// buffer packet count (including header)
} XRNETBUFFERHEADER;


XRNETDATAREADER::XRNETDATAREADER() {
}

XRNETDATAREADER::~XRNETDATAREADER() {
}



XRNETDATAWRITER::XRNETDATAWRITER(PXRNETBASE net) : net(net), uid(0) {
}

XRNETDATAWRITER::~XRNETDATAWRITER() {
}


bool XRNETDATAWRITER::send(const XRNETPACKETMETA &meta, const XRNETBUFFER &buffer) {
	unsigned int bufferSize = buffer.getSize();
	int			 rawMaxSize = this->net->getRawSize();
	int			 rawSize = rawMaxSize - sizeof(XRNETPACKETHEADER);

//	printf("rawSize=%d, maxSize=%d\n", rawSize, rawMaxSize);
	if (rawSize < (int)sizeof(XRNETBUFFERHEADER)) {
		printf("ERR: packet size is too small\n");
		return false;
	}

	// setup buffer/packet header
	XRNETBUFFER			packet(rawMaxSize);
	XRNETPACKETHEADER	packetHeader;
	XRNETBUFFERHEADER	bufferHeader;

	memset(&packetHeader, 0, sizeof(XRNETPACKETHEADER));
	packetHeader.id = uid++;
	packetHeader.seq = 0;
	packetHeader.size = sizeof(XRNETBUFFERHEADER);

	memset(&bufferHeader, 0, sizeof(XRNETBUFFERHEADER));
	bufferHeader.size = bufferSize;
	bufferHeader.checksum = 0;
	if (bufferSize % rawSize == 0) {
		bufferHeader.count = bufferSize / rawSize;
	} else {
		bufferHeader.count = (bufferSize - bufferSize % rawSize) / rawSize;
		bufferHeader.count++;
	}

	// send the first packet : buffer header
	packet.setData(0, sizeof(XRNETPACKETHEADER), &packetHeader);
	packet.setData(sizeof(XRNETPACKETHEADER), sizeof(XRNETBUFFERHEADER), &bufferHeader);
//	printf("1\n");
	if (!this->net->send(XRNETPACKET(meta, packet))) {
		printf("ERR: unable to send packet\n");
		return false;
	}
//	printf("2\n");

	// break down the data packet into raw packet
	unsigned int offset = 0;

	while (offset < bufferSize) {
		// create a packet to hold this piece of the buffer
		int size = bufferSize - offset;

		if (size > rawSize) {
			size = rawSize;
		}
		packetHeader.seq++;
		packetHeader.size = size;
		packet.setData(0, sizeof(XRNETPACKETHEADER), &packetHeader);

		// copy data to packet
		packet.setData(sizeof(XRNETPACKETHEADER), size, buffer.getPtr(offset));
		offset += size;

		// enqueue packet
		if (!this->net->send(XRNETPACKET(meta, packet))) {
			printf("ERR: unable to send packet\n");
			return false;
		}
//		printf("3\n");
	}

	// flush all packets
	return this->net->sendAll();
}



XRNETSTREAMLISTENER::XRNETSTREAMLISTENER(PXRNETDATAREADER reader) : XRNETLISTENER(), reader(reader) {
}

XRNETSTREAMLISTENER::~XRNETSTREAMLISTENER() {
}

int XRNETSTREAMLISTENER::getHeaderSize() const {
	return 0;
}

bool XRNETSTREAMLISTENER::onReceivePacket(const XRNETPACKET &packet) {
	// append packet to the corresponding list based on source host/port, buffer id and packet sequence
	const XRNETBUFFER &	buf = packet.getBuffer();
	XRNETPACKETHEADER	packetHeader;

	buf.getData(0, sizeof(XRNETPACKETHEADER), &packetHeader);
	printf("IFO: packet received from %d:%d (id=%d,seq=%d,size=%d)\n", packet.getRemoteAddress(), packet.getRemotePort(), packetHeader.id, packetHeader.seq, packetHeader.size);
//	this->packets.push_back(packet);

	// merge the packet buffer fully received into data buffers

	return XRNETLISTENER::onReceivePacket(packet);
}

bool XRNETSTREAMLISTENER::onSendPacket(XRNETPACKET &packet) {
	// do nothing here
	return XRNETLISTENER::onSendPacket(packet);
}



class MYREADER : XRNETDATAREADER {
private:
public:
	MYREADER() : XRNETDATAREADER() {
	}
	virtual ~MYREADER() {
	}


	virtual bool onReceive(const XRNETPACKETMETA &meta, const XRNETBUFFER &buffer) {
		printf("IFO: buffer received from ...\n");
		return true;
	}
};


int main(int argc, char *argv[]) {
	in_port_t localPort;
	in_port_t remotePort;
	
	if (argc > 1) {
		localPort = 10001;
		remotePort = 10000;
	} else {
		localPort = 10000;
		remotePort = 10001;
	}

	// create reader
	MYREADER					reader;

	printf("IFO: reader created\n");

	// setup packet filters chain
	XRNETCRYPTLISTENER		crypt("abc");
	XRNETCHECKSUMLISTENER	chksum(true);
	XRNETSTREAMLISTENER 		stream((PXRNETDATAREADER)&reader);

	crypt.setUpper(&chksum);
	chksum.setLower(&crypt);
	chksum.setUpper(&stream);
	stream.setLower(&chksum);

	printf("IFO: filters created\n");

	// setup udp socket & writer
	XRNETUDP					net(&crypt, &stream, 32, "localhost", localPort, "localhost", remotePort);

	printf("IFO: socket created\n");

	// create writer
	XRNETDATAWRITER			writer(&net);

	printf("IFO: writer created\n");

	// test ....
	struct hostent *local = gethostbyname("localhost");

	if (local != NULL) {
		in_addr_t addr;

		memcpy(&addr, local->h_addr, sizeof(in_addr_t));
		printf("IFO: local=%d:%d\n", addr, localPort);

		if (net.createSocket()) {
			printf("IFO: socket created\n");

			for (int i = 0; i < 1000; i++) {
				if (i > 10) {
					XRNETPACKETMETA meta(addr, localPort, addr, remotePort);
					XRNETBUFFER buffer(49);

//					printf("IFO: writing buffer...\n");
					writer.send(meta, buffer);
//					printf("IFO: buffer written.\n");
				}

//				printf("IFO: receiving packets...\n");
				net.receiveAll(1000000);
//				printf("IFO: packets receveid.\n");
			}
		} else {
			printf("ERR: cannot create socket\n");
		}
	} else {
		printf("ERR: local=??\n");
	}
	return 0;
}
