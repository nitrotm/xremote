/**
 * Xremote - client/server udp socket
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRNETBASE_H_INCLUDE_
#define _XRNETBASE_H_INCLUDE_


#define XREMOTE_PACKET_SIZE 32


/**
 * Raw network packet structure (XREMOTE_PACKET_SIZE bytes of data)
 *
 */
class XRNETRAWPACKET {
protected:
	time_t			timestamp;
	in_addr_t		localAddress;
	in_port_t		localPort;
	in_addr_t		remoteAddress;
	in_port_t		remotePort;
	unsigned char	buffer[XREMOTE_PACKET_SIZE];


public:
	XRNETRAWPACKET(in_addr_t localAddress, in_port_t localPort, in_addr_t remoteAddress, in_port_t remotePort, unsigned char *buffer);
	XRNETRAWPACKET(const XRNETRAWPACKET &ref);
	virtual ~XRNETRAWPACKET();

	virtual unsigned char * getBuffer();
	virtual int getSize();

	const time_t & getTimestamp();

	const in_addr_t & getLocalAddress();
	const in_port_t & getLocalPort();

	const in_addr_t & getRemoteAddress();
	const in_port_t & getRemotePort();
};
typedef XRNETRAWPACKET *PXRNETRAWPACKET;


/**
 * XRemote network packet empty listener
 *
 */
class XRNETLISTENER {
protected:
	XRNETLISTENER *listener;


public:
	XRNETLISTENER(XRNETLISTENER *listener);
	virtual ~XRNETLISTENER();


	virtual bool onReceivePacket(PXRNETRAWPACKET packet);
	virtual PXRNETRAWPACKET onSendPacket(PXRNETRAWPACKET packet);
};
typedef XRNETLISTENER *PXRNETLISTENER;


/**
 * XRemote client/server network raw interface
 *
 */
class XRNETBASE {
private:
	list<PXRNETRAWPACKET>	sendBuffer;
	PXRNETLISTENER			listener;


protected:
	XRLOCK lock;

	virtual int receiveOne(PXRNETRAWPACKET *packet, long timeout) = 0;
	virtual int sendOne(PXRNETRAWPACKET packet, long timeout) = 0;


public:
	XRNETBASE(PXRNETLISTENER listener);
	XRNETBASE(const XRNETBASE &ref);
	virtual ~XRNETBASE();


	virtual bool createSocket() = 0;
	virtual bool destroySocket() = 0;

	virtual bool receiveAll(long timeout = XREMOTE_READ_TIMEOUT);

	virtual bool send(PXRNETRAWPACKET packet);
	virtual bool sendNow(PXRNETRAWPACKET packet);
	virtual bool sendAll(long timeout = XREMOTE_WRITE_TIMEOUT);
};


#endif //_XRNETBASE_H_INCLUDE_
