/**
 * Xremote - client/server low-level common api
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRNETBASE_H_INCLUDE_
#define _XRNETBASE_H_INCLUDE_


class XRNETBASE;
typedef XRNETBASE *PXRNETBASE;

class XRNETLISTENER;
typedef XRNETLISTENER *PXRNETLISTENER;


/**
 * Raw network buffer
 *
 */
class XRNETBUFFER {
protected:
	int				size;
	unsigned char	*buffer;	


public:
	XRNETBUFFER();
	XRNETBUFFER(const int size);
	XRNETBUFFER(const int size, const void *buffer);
	XRNETBUFFER(const XRNETBUFFER &ref);
	virtual ~XRNETBUFFER();

	const XRNETBUFFER & operator =(const XRNETBUFFER &ref);
	const unsigned char & operator [](int offset) const;

	unsigned char * getPtr() const;
	unsigned char * getPtr(const int offset) const;
	int getSize() const;

	void getData(void *buffer) const;
	void getData(const int size, void *buffer) const;
	void getData(const int offset, const int size, void *buffer) const;

	void setData(const void *buffer);
	void setData(const int size, const void *buffer);
	void setData(const int offset, const int size, const void *buffer);
};


/**
 * Raw network packet structure
 *
 */
class XRNETPACKETMETA {
protected:
	time_t		timestamp;
	in_addr_t	localAddress;
	in_port_t	localPort;
	in_addr_t	remoteAddress;
	in_port_t	remotePort;


public:
	XRNETPACKETMETA(const in_addr_t &localAddress, const in_port_t localPort, const in_addr_t &remoteAddress, const in_port_t remotePort);
	XRNETPACKETMETA(const XRNETPACKETMETA &ref);
	virtual ~XRNETPACKETMETA();

	const time_t & getTimestamp() const;

	const in_addr_t & getLocalAddress() const;
	const in_port_t & getLocalPort() const;

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
	virtual ~XRNETPACKET();

	const XRNETBUFFER & getBuffer() const;
	void setBuffer(const XRNETBUFFER &buffer);
};


/**
 * XRemote network packet empty listener
 *
 */
class XRNETLISTENER {
protected:
	PXRNETLISTENER lower;
	PXRNETLISTENER upper;


public:
	XRNETLISTENER();
	virtual ~XRNETLISTENER();

	PXRNETLISTENER getLower() const;
	PXRNETLISTENER getUpper() const;
	void setLower(PXRNETLISTENER lower);
	void setUpper(PXRNETLISTENER upper);

	virtual int getHeaderSize() const = 0;
	virtual bool onReceivePacket(const XRNETPACKET &packet);
	virtual bool onSendPacket(XRNETPACKET &packet);
};


/**
 * XRemote client/server network raw interface
 *
 */
class XRNETBASE {
private:
	XRLOCK				lock;
	list<XRNETPACKET>	sendBuffer;


protected:
	PXRNETLISTENER	lower;
	PXRNETLISTENER	upper;

	virtual int receiveOne(XRNETPACKET **packet, const long timeout) = 0;
	virtual int sendOne(const XRNETPACKET &packet, const long timeout) = 0;


public:
	XRNETBASE(PXRNETLISTENER lower, PXRNETLISTENER upper);
	XRNETBASE(const XRNETBASE &ref);
	virtual ~XRNETBASE();

	virtual int getRawSize() const = 0;

	virtual bool createSocket() = 0;
	virtual bool destroySocket() = 0;

	virtual bool receiveAll(const long timeout = XREMOTE_READ_TIMEOUT);

	virtual bool send(const XRNETPACKET &packet);
	virtual bool sendNow(const XRNETPACKET &packet);
	virtual bool sendAll(const long timeout = XREMOTE_WRITE_TIMEOUT);
};


#endif //_XRNETBASE_H_INCLUDE_
