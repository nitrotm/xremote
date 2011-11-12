/**
 * Xremote - client/server low-level common api
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#include "xremote.h"
#include "xrnet.h"

#include "sha2.h"
#include "aes.h"


typedef struct {
	unsigned char  id;   // buffer id
	unsigned short seq;  // buffer sequence
	unsigned char  size; // packet size
} XRNETPACKETHEADER;

typedef struct {
	unsigned int size;     // buffer size
	unsigned int checksum; // buffer checksum
} XRNETBUFFERHEADER;



XRNETBUFFER::XRNETBUFFER() : size(0), buffer(NULL) {
}

XRNETBUFFER::XRNETBUFFER(int size) : size(size), buffer(NULL) {
	if (size > 0) {
		this->buffer = new unsigned char[size];
		memset(this->buffer, 0, size);
	}
}

XRNETBUFFER::XRNETBUFFER(int size, const void *buffer) : size(size), buffer(NULL) {
	if (size > 0) {
		this->buffer = new unsigned char[size];
		memcpy(this->buffer, buffer, size);
	}
}

XRNETBUFFER::XRNETBUFFER(const string &str) : size(str.length()), buffer(NULL) {
	if (str.length() > 0) {
		this->buffer = new unsigned char[str.length()];
		memcpy(this->buffer, str.c_str(), str.length());
	}
}

XRNETBUFFER::XRNETBUFFER(const XRNETBUFFER &ref) : size(ref.size), buffer(NULL) {
	if (ref.size > 0) {
		this->buffer = new unsigned char[ref.size];
		memcpy(this->buffer, ref.buffer, ref.size);
	}
}

XRNETBUFFER::~XRNETBUFFER() {
	if (this->buffer != NULL) {
		delete [] this->buffer;
	}
}

int XRNETBUFFER::length() const {
	return this->size;
}

XRNETBUFFER::operator unsigned char *() const {
	return this->buffer;
}

XRNETBUFFER::operator unsigned char *() {
	return this->buffer;
}

const XRNETBUFFER & XRNETBUFFER::operator =(const XRNETBUFFER &ref) {
	if (this->buffer != NULL) {
		delete [] this->buffer;
		this->buffer = NULL;
	}
	this->size = ref.size;
	if (ref.size > 0) {
		this->buffer = new unsigned char[ref.size];
		memcpy(this->buffer, ref.buffer, ref.size);
	}
	return *this;
}

void XRNETBUFFER::get(XRNETBUFFER &buffer) const {
	this->get(0, buffer.size, buffer, 0);
}

void XRNETBUFFER::get(int offset, XRNETBUFFER &buffer) const {
	this->get(offset, buffer.size, buffer, 0);
}

void XRNETBUFFER::get(int offset, int size, XRNETBUFFER &buffer, int bufferOffset) const {
/*	if (bufferOffset >= buffer.size) {
		printf("ERR: target offset is too big\n");
	}
	if ((buffer.size - bufferOffset) < size) {
		printf("ERR: target buffer is too small\n");
	}*/
	this->get(offset, buffer.size, buffer + bufferOffset);
}

void XRNETBUFFER::get(int offset, int size, void *buffer) const {
/*	if (offset >= this->size) {
		printf("ERR: source offset is too big\n");
	}
	if ((this->size - offset) < size) {
		printf("ERR: source buffer is too small\n");
	}*/
	memcpy(buffer, this->buffer + offset, size);
}

void XRNETBUFFER::set(const XRNETBUFFER &buffer) {
	this->set(0, buffer.size, buffer, 0);
}

void XRNETBUFFER::set(int offset, const XRNETBUFFER &buffer) {
	this->set(offset, buffer.size, buffer, 0);
}

void XRNETBUFFER::set(int offset, int size, const XRNETBUFFER &buffer, int bufferOffset) {
/*	if (bufferOffset >= buffer.size) {
		printf("ERR: target offset is too big\n");
	}
	if ((buffer.size - bufferOffset) < size) {
		printf("ERR: target buffer is too small\n");
	}*/
	this->set(offset, size, buffer.buffer + bufferOffset);
}

void XRNETBUFFER::set(int offset, int size, const void *buffer) {
/*	if (offset >= this->size) {
		printf("ERR: source offset is too big\n");
	}
	if ((this->size - offset) < size) {
		printf("ERR: source buffer is too small\n");
	}*/
	memcpy(this->buffer + offset, buffer, size);
}

string XRNETBUFFER::getString() const {
	char *buffer = new char[this->size + 1];

	memcpy(buffer, this->buffer, this->size);
	buffer[this->size] = 0;

	string str(buffer);

	delete [] buffer;
	return str;
}

unsigned int XRNETBUFFER::checksum() const {
	unsigned int sum = 11;
	int half = (this->size - this->size % 2) / 2;

	for (int i = 0; i < half; i++) {
		sum += 11 * sum + (this->buffer[i] ^ this->buffer[half + i]);
	}
	if (this->size % 2 != 0) {
		sum += 11 * sum + this->buffer[this->size - 1];
	}
	return sum;
}



XRNETPACKETMETA::XRNETPACKETMETA() :
	timestamp(0),
	localAddress(INADDR_NONE),
	localPort(0),
	remoteAddress(INADDR_NONE),
	remotePort(0) {
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

string XRNETPACKETMETA::getLocalHost() const {
	char buffer[25];
	int a = (this->localAddress >>  0) & 0xFF;
	int b = (this->localAddress >>  8) & 0xFF;
	int c = (this->localAddress >> 16) & 0xFF;
	int d = (this->localAddress >> 24) & 0xFF;

	sprintf(buffer, "%d.%d.%d.%d:%d", a, b, c, d, this->localPort);
	return buffer;
}

const in_addr_t & XRNETPACKETMETA::getLocalAddress() const {
	return this->localAddress;
}

const in_port_t & XRNETPACKETMETA::getLocalPort() const {
	return this->localPort;
}

string XRNETPACKETMETA::getRemoteHost() const {
	char buffer[25];
	int a = (this->remoteAddress >>  0) & 0xFF;
	int b = (this->remoteAddress >>  8) & 0xFF;
	int c = (this->remoteAddress >> 16) & 0xFF;
	int d = (this->remoteAddress >> 24) & 0xFF;

	sprintf(buffer, "%d.%d.%d.%d:%d", a, b, c, d, this->remotePort);
	return buffer;
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

int XRNETPACKET::length() const {
	return this->buffer.length();
}

const XRNETBUFFER & XRNETPACKET::getBuffer() const {
	return this->buffer;
}

XRNETBUFFER & XRNETPACKET::getBuffer() {
	return this->buffer;
}

void XRNETPACKET::setBuffer(const XRNETBUFFER &buffer) {
	this->buffer = buffer;
}



XRNETBUFFERQUEUE::XRNETBUFFERQUEUE() : XRNETBUFFER(), meta(), id(0), nextSeq(0), checksum(0), offset(0) {
}
XRNETBUFFERQUEUE::XRNETBUFFERQUEUE(XRNETPACKET *packet) : XRNETBUFFER(), meta(*packet), id(0), nextSeq(0), checksum(0), offset(0) {
	const XRNETBUFFER &buffer = packet->getBuffer();
	XRNETPACKETHEADER packetHeader;
	XRNETBUFFERHEADER bufferHeader;

	buffer.get(0, sizeof(XRNETPACKETHEADER), &packetHeader);
	buffer.get(sizeof(XRNETPACKETHEADER), sizeof(XRNETBUFFERHEADER), &bufferHeader);

	this->id = packetHeader.id;
	this->nextSeq = 1;
	this->checksum = bufferHeader.checksum;
	this->size = bufferHeader.size;
	if (this->size > 0) {
		this->buffer = new unsigned char[this->size];
		memset(this->buffer, 0, this->size);
	}

/*	if (buffer.length() % rawSize == 0) {
		bufferHeader.count = buffer.length() / rawSize;
	} else {
		bufferHeader.count = (buffer.length() - buffer.length() % rawSize) / rawSize;
		bufferHeader.count++;
	}*/
}
XRNETBUFFERQUEUE::XRNETBUFFERQUEUE(const XRNETBUFFERQUEUE &ref) : XRNETBUFFER(ref), meta(ref.meta), id(ref.id), nextSeq(ref.nextSeq), checksum(ref.checksum), offset(ref.offset) {
}
XRNETBUFFERQUEUE::~XRNETBUFFERQUEUE() {
}

const XRNETBUFFERQUEUE & XRNETBUFFERQUEUE::operator =(const XRNETBUFFERQUEUE &ref) {
	if (this->buffer != NULL) {
		delete [] this->buffer;
		this->buffer = NULL;
	}
	this->meta = ref.meta;
	this->id = ref.id;
	this->nextSeq = ref.nextSeq;
	this->checksum = ref.checksum;
	this->offset = ref.offset;
	this->size = ref.size;
	if (ref.size > 0) {
		this->buffer = new unsigned char[ref.size];
		memcpy(this->buffer, ref.buffer, ref.size);
	}
	return *this;
}

const XRNETPACKETMETA & XRNETBUFFERQUEUE::getMeta() const {
	return this->meta;
}

bool XRNETBUFFERQUEUE::add(XRNETPACKET *packet) {
	const XRNETBUFFER &buffer = packet->getBuffer();
	XRNETPACKETHEADER header;

	buffer.get(0, sizeof(XRNETPACKETHEADER), &header);

	if (this->offset >= this->size) {
		return false;
	}
	if ((this->size - this->offset) < header.size) {
		return false;
	}
	memcpy(this->buffer + this->offset, buffer + sizeof(XRNETPACKETHEADER), header.size);
	this->offset += header.size;
	this->nextSeq = (header.seq + 1) % 65536;
	return true;
}

bool XRNETBUFFERQUEUE::complete() const {
	return (this->offset >= this->size);
}



XRNET::XRNET(int packetSize, XRNETFILTER *first) : packetSize(packetSize), first(first), last(NULL), sendId(0) {
	while (first != NULL) {
		this->last = first;
		first = first->getLower();
	}
}

XRNET::~XRNET() {
	while (!this->sendQueue.empty()) {
		delete this->sendQueue.front();
		this->sendQueue.pop_front();
	}
}

int XRNET::getPacketSize() const {
	return this->packetSize;
}

int XRNET::getRawSize() const {
	XRNETFILTER *filter = this->first;
	int size = this->packetSize;

	while (filter != NULL && filter != this->last) {
		size -= filter->getHeaderSize();
		filter = filter->getLower();
	}
	return size;
}

bool XRNET::send(XRNETPACKET *packet) {
	XRLOCKER locker(&this->lock);
	XRNETFILTER *filter = this->first;

	while (filter != NULL) {
		if (!filter->onSendPacket(this, packet)) {
			delete packet;
			return false;
		}
		if (filter == this->last) {
			break;
		}
		filter = filter->getLower();
	}
	this->sendQueue.push_back(packet);
	return true;
}

bool XRNET::sendNow(XRNETPACKET *packet) {
	if (this->send(packet)) {
		return this->sendAll();
	}
	return false;
}

bool XRNET::sendAll(long timeout) {
	XRLOCKER locker(&this->lock);

	// send all packet pending
	int error = 0;

	while (!this->sendQueue.empty()) {
		XRNETPACKET *packet = this->sendQueue.front();
		int code = this->write(packet, timeout);

		if (code == 0) {
			break;
		}
		if (code < 0) {
			error++;
		}

		delete packet;
		this->sendQueue.pop_front();
	}
	return (error == 0);
}

bool XRNET::receive(XRNETPACKET *packet) {
	// filter packet
	XRNETFILTER *filter = this->last;

	while (filter != NULL) {
		if (!filter->onReceivePacket(this, packet)) {
			delete packet;
			continue;
		}
		if (filter == this->first) {
			break;
		}
		filter = filter->getUpper();
	}

	// decode header
	const XRNETBUFFER &buffer = packet->getBuffer();
	XRNETPACKETHEADER packetHeader;

	buffer.get(0, sizeof(XRNETPACKETHEADER), &packetHeader);
//	printf("recv: id=%d, seq=%d, size=%d\n", packetHeader.id, packetHeader.seq, packetHeader.size);

	// create buffer queue?
	map<unsigned char, XRNETBUFFERQUEUE>::iterator recvIt = this->recvQueue.find(packetHeader.id);

	if (packetHeader.seq == 0) {
		this->recvQueue[packetHeader.id] = XRNETBUFFERQUEUE(packet);
		return true;
	}
	if (recvIt == this->recvQueue.end()) {
		printf("ERR: lost buffer sequence: %d.%d\n", packetHeader.id, packetHeader.seq);
		return false;
	}

	// update buffer queue
	XRNETBUFFERQUEUE &queue = this->recvQueue[packetHeader.id];

	if (!queue.add(packet)) {
		this->recvQueue.erase(recvIt);

		printf("ERR: corrupted buffer sequence: %d.%d\n", packetHeader.id);
		return false;
	}
	if (queue.complete()) {
		this->onReceive(queue.getMeta(), queue);
		this->recvQueue.erase(recvIt);
	}
	return true;
}

bool XRNET::send(const XRNETPACKETMETA &meta, const XRNETBUFFER &data) {
	int rawSize = this->getRawSize() - sizeof(XRNETPACKETHEADER);

	if (rawSize < (int)sizeof(XRNETBUFFERHEADER)) {
		printf("ERR: packet size is too small\n");
		return false;
	}

	// setup buffer/packet header
	XRNETBUFFER			buffer(this->getRawSize());
	XRNETPACKETHEADER	packetHeader;
	XRNETBUFFERHEADER	bufferHeader;

	memset(&packetHeader, 0, sizeof(XRNETPACKETHEADER));
	packetHeader.id = (this->sendId++ % 256);
	packetHeader.seq = 0;
	packetHeader.size = sizeof(XRNETBUFFERHEADER);

	memset(&bufferHeader, 0, sizeof(XRNETBUFFERHEADER));
	bufferHeader.size = data.length();
	bufferHeader.checksum = data.checksum();

	// send the first packet : buffer header
	buffer.set(0, sizeof(XRNETPACKETHEADER), &packetHeader);
	buffer.set(sizeof(XRNETPACKETHEADER), sizeof(XRNETBUFFERHEADER), &bufferHeader);
	if (!this->send(new XRNETPACKET(meta, buffer))) {
		return false;
	}

	// break down the data packet into raw packet
	unsigned int offset = 0;
	int size = data.length() - offset;

	while (size > 0) {
		if (size > rawSize) {
			size = rawSize;
		}

		// create a packet to hold this piece of the buffer
		packetHeader.seq++;
		packetHeader.size = size;
		buffer.set(0, sizeof(XRNETPACKETHEADER), &packetHeader);
		buffer.set(sizeof(XRNETPACKETHEADER), size, data, offset);

		// enqueue packet
		if (!this->send(new XRNETPACKET(meta, buffer))) {
			return false;
		}

		offset += size;
		size = data.length() - offset;
	}

	// flush all packets
	return this->sendAll();
}

bool XRNET::receiveAll(long timeout) {
	XRLOCKER locker(&this->lock);

	// receive all packet pending
	XRNETPACKET	*packet = NULL;
	int code = this->read(&packet, timeout);

	while (code > 0) {
		this->receive(packet);
		delete packet;
		packet = NULL;

		code = this->read(&packet, timeout);
	}
	return (code >= 0);
}



XRNETFILTER::XRNETFILTER(XRNETFILTER *child) : lower(NULL), upper(NULL) {
	if (child != NULL) {
		this->lower = child;
		this->lower->upper = this;
	}
}

XRNETFILTER::~XRNETFILTER() {
}

XRNETFILTER* XRNETFILTER::getLower() const {
	return this->lower;
}

XRNETFILTER* XRNETFILTER::getUpper() const {
	return this->upper;
}



XRNETCHECKSUMFILTER::XRNETCHECKSUMFILTER(XRNETFILTER *child) : XRNETFILTER(child) {
}

XRNETCHECKSUMFILTER::~XRNETCHECKSUMFILTER() {
}

int XRNETCHECKSUMFILTER::getHeaderSize() const {
	return sizeof(unsigned int);
}

bool XRNETCHECKSUMFILTER::onReceivePacket(XRNET *net, XRNETPACKET *packet) {
	const XRNETBUFFER & buffer = packet->getBuffer();

	if (buffer.length() < this->getHeaderSize()) {
		printf("ERR: packet size is too small\n");
		return false;
	}

	// fetch check sum
	unsigned int remoteSum = 0;

	buffer.get(0, sizeof(unsigned int), &remoteSum);

	// fetch buffer
	XRNETBUFFER outBuffer(buffer.length() - sizeof(unsigned int));

	buffer.get(sizeof(unsigned int), outBuffer);

	// check local/remote sum equality
	unsigned int localSum = outBuffer.checksum();

	if (remoteSum != localSum) {
		printf("ERR: chksum failed (%08X != %08X) !\n", remoteSum, localSum);
		return false;
	}

	// replace packet buffer
	packet->setBuffer(outBuffer);
	return true;
}

bool XRNETCHECKSUMFILTER::onSendPacket(XRNET *net, XRNETPACKET *packet) {
	const XRNETBUFFER &buffer = packet->getBuffer();

	// append check sum
	unsigned int sum = buffer.checksum();
	XRNETBUFFER outBuffer(buffer.length() + sizeof(unsigned int));

	outBuffer.set(0, sizeof(unsigned int), &sum);
	outBuffer.set(sizeof(unsigned int), buffer);

	packet->setBuffer(outBuffer);
	return true;
}



XRNETCRYPTFILTER::XRNETCRYPTFILTER(const string &password, XRNETFILTER *child) : XRNETFILTER(child), ctx(NULL) {
	sha256_context ctx;

	memset(&ctx, 0, sizeof(sha256_context));
	sha256_starts(&ctx);
	sha256_update(&ctx, (uint8*)password.c_str(), password.length());
	sha256_finish(&ctx, this->key);

	this->ctx = new aes_context;
	memset(this->ctx, 0, sizeof(aes_context));
	aes_set_key((aes_context*)this->ctx, this->key, sizeof(this->key) * 8);
}

XRNETCRYPTFILTER::~XRNETCRYPTFILTER() {
	if (this->ctx != NULL) {
		delete (aes_context*)this->ctx;
	}
}

int XRNETCRYPTFILTER::getHeaderSize() const {
	return 0;
}

bool XRNETCRYPTFILTER::onReceivePacket(XRNET *net, XRNETPACKET *packet) {
	XRNETBUFFER &buffer = packet->getBuffer();

	if (buffer.length() % 16 != 0) {
		printf("ERR: packet size is not rounded to 16 bytes (%d bytes)\n", buffer.length());
		return false;
	}

	// decrypt buffer
	uint8 in[16];
	uint8 out[16];

	for (int i = 0; i < buffer.length(); i += 16) {
		buffer.get(i, 16, in);
		aes_decrypt((aes_context*)this->ctx, in, out);
		buffer.set(i, 16, out);
	}
	return true;
}

bool XRNETCRYPTFILTER::onSendPacket(XRNET *net, XRNETPACKET *packet) {
	XRNETBUFFER &buffer = packet->getBuffer();

	if (buffer.length() % 16 != 0) {
		printf("ERR: packet size is not rounded to 16 bytes (%d bytes)\n", buffer.length());
		return false;
	}

	// encrypt buffer
	uint8 in[16];
	uint8 out[16];

	for (int i = 0; i < buffer.length(); i += 16) {
		buffer.get(i, 16, in);
		aes_encrypt((aes_context*)this->ctx, in, out);
		buffer.set(i, 16, out);
	}
	return true;
}


/*
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
*/