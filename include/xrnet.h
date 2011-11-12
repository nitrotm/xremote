/**
 * Xremote - client/server low-level common api
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free to use for any purpose ;)
 */
#ifndef _XRNETBASE_H_INCLUDE_
#define _XRNETBASE_H_INCLUDE_


/**
 * Raw network buffer
 *
 */
class XRNETBUFFER {
protected:
	int           size;
	unsigned char *buffer;	


public:
	XRNETBUFFER();
	XRNETBUFFER(int size);
	XRNETBUFFER(int size, const void *buffer);
	XRNETBUFFER(const string &str);
	XRNETBUFFER(const XRNETBUFFER &ref);
	virtual ~XRNETBUFFER();

	int length() const;

	const XRNETBUFFER & operator =(const XRNETBUFFER &ref);
	operator unsigned char *() const;
	operator unsigned char *();

	void get(XRNETBUFFER &buffer) const;
	void get(int offset, XRNETBUFFER &buffer) const;
	void get(int offset, int size, XRNETBUFFER &buffer, int bufferOffset) const;
	void get(int offset, int size, void *buffer) const;

	void set(const XRNETBUFFER &buffer);
	void set(int offset, const XRNETBUFFER &buffer);
	void set(int offset, int size, const XRNETBUFFER &buffer, int bufferOffset);
	void set(int offset, int size, const void *buffer);

	string getString() const;
	unsigned int checksum() const;
};


/**
 * Raw network packet structure
 *
 */
class XRNETPACKETMETA {
protected:
	time_t    timestamp;
	in_addr_t localAddress;
	in_port_t localPort;
	in_addr_t remoteAddress;
	in_port_t remotePort;


public:
	XRNETPACKETMETA();
	XRNETPACKETMETA(const in_addr_t &localAddress, const in_port_t localPort, const in_addr_t &remoteAddress, const in_port_t remotePort);
	XRNETPACKETMETA(const XRNETPACKETMETA &ref);
	~XRNETPACKETMETA();

	const time_t & getTimestamp() const;

	string getLocalHost() const;
	const in_addr_t & getLocalAddress() const;
	const in_port_t & getLocalPort() const;

	string getRemoteHost() const;
	const in_addr_t & getRemoteAddress() const;
	const in_port_t & getRemotePort() const;
};


/**
 * Raw network packet structure
 *
 */
class XRNETPACKET : public XRNETPACKETMETA {
protected:
	XRNETBUFFER	buffer;


public:
	XRNETPACKET(const in_addr_t &localAddress, const in_port_t localPort, const in_addr_t &remoteAddress, const in_port_t remotePort, const XRNETBUFFER &buffer);
	XRNETPACKET(const XRNETPACKETMETA &meta, const XRNETBUFFER &buffer);
	XRNETPACKET(const XRNETPACKET &ref);
	~XRNETPACKET();

	int length() const;

	const XRNETBUFFER & getBuffer() const;
	XRNETBUFFER & getBuffer();
	void setBuffer(const XRNETBUFFER &buffer);
};


/**
 * Buffer queue entry
 *
 */
class XRNETBUFFERQUEUE : public XRNETBUFFER {
protected:
	XRNETPACKETMETA meta;
	unsigned int    id;
	unsigned int    nextSeq;
	unsigned int    checksum;
	unsigned int    offset;


public:
	XRNETBUFFERQUEUE();
	XRNETBUFFERQUEUE(XRNETPACKET *packet);
	XRNETBUFFERQUEUE(const XRNETBUFFERQUEUE &ref);
	virtual ~XRNETBUFFERQUEUE();

	const XRNETBUFFERQUEUE & operator =(const XRNETBUFFERQUEUE &ref);

	const XRNETPACKETMETA & getMeta() const;

	bool add(XRNETPACKET *packet);
	bool complete() const;
};


class XRNETFILTER;


/**
 * XRemote client/server network raw interface
 *
 */
class XRNET {
private:
	int                 packetSize;
	XRNETFILTER*        first;
	XRNETFILTER*        last;
	unsigned int        sendId;
	list<XRNETPACKET*>  sendQueue;
	map<unsigned char, XRNETBUFFERQUEUE>
	                    recvQueue;


protected:
	XRLOCK lock;

	virtual int getPacketSize() const;
	virtual int getRawSize() const;

	virtual bool send(XRNETPACKET *packet);
	virtual bool sendNow(XRNETPACKET *packet);
	virtual bool sendAll(long timeout = XREMOTE_WRITE_TIMEOUT);
	virtual bool receive(XRNETPACKET *packet);

	virtual bool createSocket() = 0;
	virtual bool destroySocket() = 0;

	virtual int read(XRNETPACKET **packet, long timeout) = 0;
	virtual int write(const XRNETPACKET *packet, long timeout) = 0;

	virtual bool onReceive(const XRNETPACKETMETA &meta, const XRNETBUFFER &buffer) = 0;


public:
	XRNET(int packetSize, XRNETFILTER *first);
	virtual ~XRNET();

	virtual bool send(const XRNETPACKETMETA &meta, const XRNETBUFFER &str);
	virtual bool receiveAll(long timeout = XREMOTE_READ_TIMEOUT);
};


/**
 * XRemote client/server network udp interface
 *
 */
class XRNETUDP : public XRNET {
private:
	int s;


protected:
	XRNETPACKETMETA meta;
	string          localHost;
	string          remoteHost;

	virtual bool createSocket();
	virtual bool destroySocket();

	virtual int read(XRNETPACKET **packet, long timeout);
	virtual int write(const XRNETPACKET *packet, long timeout);

	static in_addr_t resolveHost(const string &host, const in_addr_t &defaultAddress);
	

public:
	XRNETUDP(int packetSize, XRNETFILTER *first, const string &localHost, const in_port_t localPort, const string &remoteHost = "", const in_port_t remotePort = 0);
	virtual ~XRNETUDP();
};


/**
 * XRemote network packet filter
 *
 */
class XRNETFILTER {
protected:
	XRNETFILTER* lower;
	XRNETFILTER* upper;


public:
	XRNETFILTER(XRNETFILTER *child = NULL);
	virtual ~XRNETFILTER();

	XRNETFILTER* getLower() const;
	XRNETFILTER* getUpper() const;

	virtual int getHeaderSize() const = 0;
	virtual bool onReceivePacket(XRNET *net, XRNETPACKET *packet) = 0;
	virtual bool onSendPacket(XRNET *net, XRNETPACKET *packet) = 0;
};


/**
 * XRemote network packet checksum filter
 *
 */
class XRNETCHECKSUMFILTER : public XRNETFILTER {
public:
	XRNETCHECKSUMFILTER(XRNETFILTER *child = NULL);
	virtual ~XRNETCHECKSUMFILTER();

	virtual int getHeaderSize() const;
	virtual bool onReceivePacket(XRNET *net, XRNETPACKET *packet);
	virtual bool onSendPacket(XRNET *net, XRNETPACKET *packet);
};


/**
 * XRemote network packet aes encryption listener
 *
 */
class XRNETCRYPTFILTER : public XRNETFILTER {
protected:
	unsigned char key[32];
	void          *ctx;


public:
	XRNETCRYPTFILTER(const string &password, XRNETFILTER *child = NULL);
	virtual ~XRNETCRYPTFILTER();

	virtual int getHeaderSize() const;
	virtual bool onReceivePacket(XRNET *net, XRNETPACKET *packet);
	virtual bool onSendPacket(XRNET *net, XRNETPACKET *packet);
};


#endif //_XRNETBASE_H_INCLUDE_
