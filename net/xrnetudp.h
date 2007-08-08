/**
 * Xremote - client/server low-level udp socket
 *
 * \author Antony Ducommun (nitro.tm@gmail.com)
 *
 * license : free of use for any purpose ;)
 */
#ifndef _XRNETUDP_H_INCLUDE_
#define _XRNETUDP_H_INCLUDE_


/**
 * XRemote client/server network udp interface
 *
 */
class XRNETUDP : public XRNETBASE {
private:
	int			packetSize;
	string		localHost;
	in_addr_t	localAddress;
	in_port_t	localPort;
	string		remoteHost;
	in_addr_t	remoteAddress;
	in_port_t	remotePort;
	int			s;


protected:
	virtual int receiveOne(XRNETPACKET **packet, const long timeout);
	virtual int sendOne(const XRNETPACKET &packet, const long timeout);

	virtual int getRawSize(PXRNETLISTENER listener) const;


public:
	XRNETUDP(PXRNETLISTENER lower, PXRNETLISTENER upper, const int packetSize, const string &localHost, const in_port_t localPort);
	XRNETUDP(PXRNETLISTENER lower, PXRNETLISTENER upper, const int packetSize, const string &localHost, const in_port_t localPort, const string &remoteHost, const in_port_t remotePort);
	XRNETUDP(const XRNETUDP &ref);
	virtual ~XRNETUDP();

	virtual int getRawSize() const;

	virtual bool createSocket();
	virtual bool destroySocket();
};


#endif //_XRNETUDP_H_INCLUDE_
