/**
 * Xremote - client/server udp socket
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRNET_H_INCLUDE_
#define _XRNET_H_INCLUDE_


// Net data code
#define XRNETDATA_CODE_EVENT				0x00
#define XRNETDATA_CODE_PRIMARYBUFFER		0x01
#define XRNETDATA_CODE_SECONDARYBUFFER	0x02
#define XRNETDATA_CODE_CLIPBOARDBUFFER	0x03


/**
 * Network encrypted event packet structure (16 bytes)
 *
 */
typedef struct {
	unsigned char	buffer[8 + sizeof(XRNETEVENT)];
} XRNETPACKET, *PXRNETPACKET;

/**
 * Network event packet data structure (16 bytes)
 *
 */
typedef struct {
	unsigned int		chksum;
	unsigned char	code;
	unsigned char	size;
	unsigned short	sequence;
	unsigned char	buffer[sizeof(XRNETEVENT)];
} XRNETDATA, *PXRNETDATA;


/**
 * XRemote client/server udp socket
 *
 */
class XRNET : public XRWINDOW {
private:
	unsigned char		key[32];
	bool					encrypt;
	int					s;
	PXRBUFFEREVENT		primaryBufferEvent;
	PXRBUFFEREVENT		secondaryBufferEvent;
	PXRBUFFEREVENT		clipboardBufferEvent;
	list<PXREVENT>		sendEvents;
	list<PXREVENT>		recvEvents;


	int receiveEvent(long timeout);
	PXREVENT popReceiveEvent();
	void pushReceiveEvent(PXREVENT event);
	void freeReceiveEvents();

	int sendEvent(long timeout);
	int sendDataPacket(long timeout, PXREVENT event, PXRNETDATA data);
	PXREVENT popSendEvent();
	void pushSendEvent(PXREVENT event);
	void freeSendEvents();


protected:
	XRLOCK		lock;
	string		localHost;
	in_addr_t	localAddress;
	in_port_t	localPort;
	string		remoteHost;
	in_addr_t	remoteAddress;
	in_port_t	remotePort;


	bool createSocket();
	bool destroySocket();

	bool receiveAll(long timeout = XREMOTE_READ_TIMEOUT);
	virtual bool onReceive(PXREVENT event) = 0;

	void send(PXREVENT event);
	bool sendNow(PXREVENT event);
	bool sendAll(long timeout = XREMOTE_WRITE_TIMEOUT);
	virtual bool onSend(PXREVENT event) = 0;

	string ipToString(in_addr_t address);
	string hostToString(in_addr_t address, in_port_t port);


public:
	XRNET(const string &localHost, in_port_t localPort, const string &displayName);
	XRNET(const string &localHost, in_port_t localPort, const string &remoteHost, in_port_t remotePort, const string &displayName);
	XRNET(const XRNET &ref);
	virtual ~XRNET();


	void setEncryptionKey(const string &password);

	virtual bool main() = 0;
	virtual void shutdown() = 0;
};


#endif //_XRNET_H_INCLUDE_
